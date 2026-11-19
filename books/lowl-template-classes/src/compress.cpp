// MIT License - (c) Anthony Matarazzo 2026
//
// compress.cpp - RLE, LZ77, XOR delta and block adaptive Huffman, plus the
// framed container that makes a compressed stream self describing.
//
// Every coder here is self terminating or length prefixed so it can also be
// called on its own, without the container.
#include "compress.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <queue>

namespace lowl {

namespace {

// Token bytes used by the byte oriented coders. A literal that collides with a
// marker is escaped, which is the part the book left out.
constexpr std::uint8_t kMatch = 0xFE;
constexpr std::uint8_t kEscape = 0xFF;

void write_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>(v >> (i * 8)));
}

std::uint32_t read_u32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

// ---------------------------------------------------------------- bit stream

class BitWriter {
public:
    void put(std::uint32_t code, unsigned bits) {
        for (unsigned i = bits; i-- > 0;) {
            cur_ = static_cast<std::uint8_t>((cur_ << 1) | ((code >> i) & 1u));
            if (++filled_ == 8) {
                out_.push_back(cur_);
                cur_ = 0;
                filled_ = 0;
            }
        }
    }
    // Appends the padded tail to dst instead of returning out_, which keeps
    // gcc 12 from mis-reading the copy as a free of an interior pointer.
    void finish(std::vector<std::uint8_t>& dst) {
        if (filled_ != 0) {
            out_.push_back(static_cast<std::uint8_t>(cur_ << (8 - filled_)));
            cur_ = 0;
            filled_ = 0;
        }
        dst.insert(dst.end(), out_.begin(), out_.end());
        out_.clear();
    }

private:
    std::vector<std::uint8_t> out_;
    std::uint8_t cur_ = 0;
    unsigned filled_ = 0;
};

class BitReader {
public:
    BitReader(const std::uint8_t* data, std::size_t len) : data_(data), len_(len) {}
    int get() {
        if (pos_ >= len_ * 8) return -1;
        const int bit = (data_[pos_ >> 3] >> (7 - (pos_ & 7))) & 1;
        ++pos_;
        return bit;
    }

private:
    const std::uint8_t* data_;
    std::size_t len_;
    std::size_t pos_ = 0;
};

// ---------------------------------------------------------------- Huffman

constexpr int kAlphabet = 256;

struct HuffTable {
    std::array<std::uint32_t, kAlphabet> code{};
    std::array<std::uint8_t, kAlphabet> len{};
    // Binary decode tree: a node is a leaf when child[0] == -1, and then
    // symbol holds the byte. Node 0 is the root.
    struct TreeNode {
        int child[2] = {-1, -1};
        int symbol = -1;
    };
    std::vector<TreeNode> tree;

    bool is_leaf(std::size_t n) const { return tree[n].child[0] < 0; }
};

// Builds a canonical Huffman code for the symbol frequencies seen so far. Every
// symbol keeps a floor weight of one so that a symbol which shows up for the
// first time in the next block still has a code and no escape mechanism is
// needed. Both sides call this with identical frequencies, so the codes agree.
void build_huffman(const std::array<std::uint64_t, kAlphabet>& freq, HuffTable& t) {
    struct Item {
        std::uint64_t weight;
        int node;
        bool operator<(const Item& o) const {
            // Deterministic order: lighter first, then lower node index.
            if (weight != o.weight) return weight > o.weight;
            return node > o.node;
        }
    };
    std::vector<std::uint64_t> weight;
    std::vector<int> parent;
    weight.reserve(2 * kAlphabet);
    parent.reserve(2 * kAlphabet);
    std::priority_queue<Item> pq;
    for (int s = 0; s < kAlphabet; ++s) {
        const std::uint64_t w = std::max<std::uint64_t>(freq[static_cast<std::size_t>(s)], 1);
        weight.push_back(w);
        parent.push_back(-1);
        pq.push(Item{w, s});
    }
    while (pq.size() > 1) {
        const Item a = pq.top();
        pq.pop();
        const Item b = pq.top();
        pq.pop();
        const int node = static_cast<int>(weight.size());
        weight.push_back(a.weight + b.weight);
        parent.push_back(-1);
        parent[static_cast<std::size_t>(a.node)] = node;
        parent[static_cast<std::size_t>(b.node)] = node;
        pq.push(Item{a.weight + b.weight, node});
    }

    // Raw depths, then a length limit guard. With a floor of one no symbol can
    // be deeper than 255, and a 32 bit code word holds that comfortably.
    std::array<unsigned, kAlphabet> depth{};
    for (int s = 0; s < kAlphabet; ++s) {
        unsigned d = 0;
        for (int n = s; parent[static_cast<std::size_t>(n)] >= 0;
             n = parent[static_cast<std::size_t>(n)])
            ++d;
        depth[static_cast<std::size_t>(s)] = std::max(d, 1u);
    }
    unsigned max_depth = 0;
    for (unsigned d : depth) max_depth = std::max(max_depth, d);
    if (max_depth > 32) {
        for (int s = 0; s < kAlphabet; ++s) depth[static_cast<std::size_t>(s)] = 8;
        max_depth = 8;
    }

    // Canonical assignment: shorter codes first, ties by symbol value.
    std::array<int, kAlphabet> order{};
    for (int s = 0; s < kAlphabet; ++s) order[static_cast<std::size_t>(s)] = s;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        if (depth[static_cast<std::size_t>(a)] != depth[static_cast<std::size_t>(b)])
            return depth[static_cast<std::size_t>(a)] < depth[static_cast<std::size_t>(b)];
        return a < b;
    });

    t.tree.clear();
    t.tree.push_back(HuffTable::TreeNode{});
    std::uint32_t code = 0;
    unsigned cur_len = depth[static_cast<std::size_t>(order[0])];
    for (int s : order) {
        const unsigned d = depth[static_cast<std::size_t>(s)];
        code <<= (d - cur_len);
        cur_len = d;
        t.code[static_cast<std::size_t>(s)] = code;
        t.len[static_cast<std::size_t>(s)] = static_cast<std::uint8_t>(d);
        // Insert into the decode tree.
        int node = 0;
        for (unsigned bit = d; bit-- > 0;) {
            const int b = static_cast<int>((code >> bit) & 1u);
            int next = t.tree[static_cast<std::size_t>(node)].child[b];
            if (next < 0) {
                next = static_cast<int>(t.tree.size());
                t.tree.push_back(HuffTable::TreeNode{});
                t.tree[static_cast<std::size_t>(node)].child[b] = next;
            }
            node = next;
        }
        t.tree[static_cast<std::size_t>(node)].symbol = s;
        ++code;
    }
}

} // namespace

const char* algorithm_name(Algorithm a) {
    switch (a) {
        case Algorithm::NONE: return "NONE";
        case Algorithm::LZ77: return "LZ77";
        case Algorithm::HUFFMAN: return "HUFFMAN";
        case Algorithm::RLE: return "RLE";
        case Algorithm::XOR: return "XOR";
        case Algorithm::DELTA: return "DELTA";
        case Algorithm::AUTO: return "AUTO";
    }
    return "UNKNOWN";
}

const char* level_name(Level l) {
    switch (l) {
        case Level::FASTEST: return "FASTEST";
        case Level::DEFAULT: return "DEFAULT";
        case Level::MAXIMUM: return "MAXIMUM";
        case Level::ULTRA: return "ULTRA";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------- RLE

std::vector<std::uint8_t> rle_compress(const std::vector<std::uint8_t>& in, CompressStats* st) {
    std::vector<std::uint8_t> out;
    out.reserve(in.size());
    std::size_t i = 0;
    std::size_t literals = 0;
    std::size_t runs = 0;
    while (i < in.size()) {
        const std::uint8_t value = in[i];
        std::size_t count = 1;
        while (i + count < in.size() && in[i + count] == value && count < 255) ++count;
        // A lone marker byte has to be escaped, otherwise the decoder would
        // read it as a run header.
        if (count >= 3 || value == kMatch) {
            out.push_back(kMatch);
            out.push_back(value);
            out.push_back(static_cast<std::uint8_t>(count));
            ++runs;
        } else {
            for (std::size_t k = 0; k < count; ++k) out.push_back(value);
            literals += count;
        }
        i += count;
    }
    if (st) {
        st->input_bytes = in.size();
        st->output_bytes = out.size();
        st->literal_bytes = literals;
        st->match_count = runs;
    }
    return out;
}

std::vector<std::uint8_t> rle_decompress(const std::vector<std::uint8_t>& in) {
    std::vector<std::uint8_t> out;
    std::size_t i = 0;
    while (i < in.size()) {
        if (in[i] == kMatch && i + 2 < in.size()) {
            const std::uint8_t value = in[i + 1];
            const std::size_t count = in[i + 2];
            for (std::size_t k = 0; k < count; ++k) out.push_back(value);
            i += 3;
        } else {
            out.push_back(in[i]);
            ++i;
        }
    }
    return out;
}

// ---------------------------------------------------------------- LZ77

Lz77Config lz77_config_for(Level level) {
    Lz77Config cfg;
    switch (level) {
        case Level::FASTEST:
            cfg.window = 256;
            cfg.max_chain = 4;
            break;
        case Level::DEFAULT:
            cfg.window = 4096;
            cfg.max_chain = 64;
            break;
        case Level::MAXIMUM:
            cfg.window = 32768;
            cfg.max_chain = 256;
            break;
        case Level::ULTRA:
            cfg.window = 65535;
            cfg.max_chain = 1024;
            break;
    }
    return cfg;
}

namespace {

// Three byte rolling hash feeding the chain table.
inline std::size_t hash3(const std::uint8_t* p) {
    const std::uint32_t v = (static_cast<std::uint32_t>(p[0]) << 16) |
                            (static_cast<std::uint32_t>(p[1]) << 8) |
                            static_cast<std::uint32_t>(p[2]);
    return static_cast<std::size_t>((v * 2654435761u) >> 17) & 0x7FFFu;
}

void emit_literal(std::vector<std::uint8_t>& out, std::uint8_t b, std::size_t& literals) {
    if (b == kMatch || b == kEscape) out.push_back(kEscape);
    out.push_back(b);
    ++literals;
}

} // namespace

std::vector<std::uint8_t> lz77_compress(const std::vector<std::uint8_t>& in,
                                        const Lz77Config& cfg, CompressStats* st) {
    const std::size_t n = in.size();
    std::vector<std::uint8_t> out;
    out.reserve(n);
    std::size_t literals = 0;
    std::size_t matches = 0;

    const std::size_t window = std::min<std::size_t>(std::max<std::size_t>(cfg.window, 1), 65535);
    const std::size_t min_match = std::max<std::size_t>(cfg.min_match, 3);
    const std::size_t max_match = std::min<std::size_t>(std::max<std::size_t>(cfg.max_match, 3), 255);
    const std::size_t max_chain = std::max<std::size_t>(cfg.max_chain, 1);

    constexpr std::size_t kHashSize = 1u << 15;
    std::vector<std::int32_t> head(kHashSize, -1);
    std::vector<std::int32_t> prev(n, -1);

    auto insert = [&](std::size_t i) {
        if (i + 3 > n) return;
        const std::size_t h = hash3(&in[i]);
        prev[i] = head[h];
        head[h] = static_cast<std::int32_t>(i);
    };

    std::size_t i = 0;
    while (i < n) {
        std::size_t best_len = 0;
        std::size_t best_off = 0;
        if (i + 3 <= n) {
            const std::size_t limit = std::min(max_match, n - i);
            std::int32_t cand = head[hash3(&in[i])];
            std::size_t seen = 0;
            while (cand >= 0 && seen < max_chain) {
                const std::size_t j = static_cast<std::size_t>(cand);
                if (i - j > window) break;
                // Cheap reject before the byte loop.
                if (in[j + best_len] == in[i + best_len]) {
                    std::size_t len = 0;
                    while (len < limit && in[j + len] == in[i + len]) ++len;
                    if (len > best_len) {
                        best_len = len;
                        best_off = i - j;
                        if (len == limit) break;
                    }
                }
                cand = prev[j];
                ++seen;
            }
        }
        if (best_len >= min_match && best_off > 0) {
            out.push_back(kMatch);
            out.push_back(static_cast<std::uint8_t>(best_off >> 8));
            out.push_back(static_cast<std::uint8_t>(best_off & 0xFF));
            out.push_back(static_cast<std::uint8_t>(best_len));
            for (std::size_t k = 0; k < best_len; ++k) insert(i + k);
            i += best_len;
            ++matches;
        } else {
            emit_literal(out, in[i], literals);
            insert(i);
            ++i;
        }
    }
    if (st) {
        st->input_bytes = in.size();
        st->output_bytes = out.size();
        st->literal_bytes = literals;
        st->match_count = matches;
    }
    return out;
}

std::vector<std::uint8_t> lz77_decompress(const std::vector<std::uint8_t>& in) {
    std::vector<std::uint8_t> out;
    std::size_t i = 0;
    while (i < in.size()) {
        if (in[i] == kMatch && i + 3 < in.size()) {
            const std::size_t off = (static_cast<std::size_t>(in[i + 1]) << 8) | in[i + 2];
            const std::size_t len = in[i + 3];
            if (off > 0 && off <= out.size()) {
                const std::size_t start = out.size() - off;
                for (std::size_t k = 0; k < len; ++k) out.push_back(out[start + k]);
                i += 4;
                continue;
            }
            // Bad offset means a damaged stream: fall back to a literal so the
            // decoder still makes progress instead of looping.
            out.push_back(in[i]);
            ++i;
        } else if (in[i] == kEscape && i + 1 < in.size()) {
            out.push_back(in[i + 1]);
            i += 2;
        } else {
            out.push_back(in[i]);
            ++i;
        }
    }
    return out;
}

// ---------------------------------------------------------------- XOR delta

std::vector<std::uint8_t> xor_compress(const std::vector<std::uint8_t>& in, CompressStats* st) {
    std::vector<std::uint8_t> out(in.size(), 0);
    if (!in.empty()) {
        out[0] = in[0];
        for (std::size_t i = 1; i < in.size(); ++i)
            out[i] = static_cast<std::uint8_t>(in[i] ^ in[i - 1]);
    }
    if (st) {
        st->input_bytes = in.size();
        st->output_bytes = out.size();
        st->literal_bytes = in.size();
        st->match_count = 0;
    }
    return out;
}

std::vector<std::uint8_t> xor_decompress(const std::vector<std::uint8_t>& in) {
    std::vector<std::uint8_t> out(in.size(), 0);
    if (in.empty()) return out;
    out[0] = in[0];
    for (std::size_t i = 1; i < in.size(); ++i)
        out[i] = static_cast<std::uint8_t>(out[i - 1] ^ in[i]);
    return out;
}

// ---------------------------------------------------------------- Huffman

std::vector<std::uint8_t> huffman_compress(const std::vector<std::uint8_t>& in, CompressStats* st,
                                           const HuffmanConfig& cfg) {
    const std::size_t block = std::max<std::size_t>(cfg.block_size, 1);
    std::vector<std::uint8_t> out;
    write_u32(out, static_cast<std::uint32_t>(in.size()));
    write_u32(out, static_cast<std::uint32_t>(block));

    std::array<std::uint64_t, kAlphabet> freq{};
    BitWriter bw;
    std::size_t bits = 0;
    for (std::size_t start = 0; start < in.size(); start += block) {
        HuffTable table;
        build_huffman(freq, table);
        const std::size_t end = std::min(in.size(), start + block);
        for (std::size_t i = start; i < end; ++i) {
            const std::size_t s = in[i];
            bw.put(table.code[s], table.len[s]);
            bits += table.len[s];
            ++freq[s];
        }
    }
    bw.finish(out);
    if (st) {
        st->input_bytes = in.size();
        st->output_bytes = out.size();
        st->literal_bytes = bits / 8;
        st->match_count = in.size();
    }
    return out;
}

std::vector<std::uint8_t> huffman_decompress(const std::vector<std::uint8_t>& in) {
    if (in.size() < 8) return {};
    const std::uint32_t count = read_u32(in.data());
    const std::uint32_t block = std::max<std::uint32_t>(read_u32(in.data() + 4), 1);
    BitReader br(in.data() + 8, in.size() - 8);

    std::array<std::uint64_t, kAlphabet> freq{};
    std::vector<std::uint8_t> out;
    out.reserve(count);
    std::uint32_t done = 0;
    while (done < count) {
        HuffTable table;
        build_huffman(freq, table);
        const std::uint32_t todo = std::min(block, count - done);
        for (std::uint32_t k = 0; k < todo; ++k) {
            int node = 0;
            while (!table.is_leaf(static_cast<std::size_t>(node))) {
                const int bit = br.get();
                if (bit < 0) return out;   // truncated stream
                node = table.tree[static_cast<std::size_t>(node)].child[bit];
                if (node < 0) return out;
            }
            const int sym = table.tree[static_cast<std::size_t>(node)].symbol;
            out.push_back(static_cast<std::uint8_t>(sym));
            ++freq[static_cast<std::size_t>(sym)];
        }
        done += todo;
    }
    return out;
}

// ---------------------------------------------------------------- container

std::vector<std::uint8_t> compress(const std::vector<std::uint8_t>& in, Algorithm algo,
                                   CompressStats* st) {
    if (algo == Algorithm::AUTO) return compress_auto(in, nullptr, st);
    std::vector<std::uint8_t> payload;
    switch (algo) {
        case Algorithm::LZ77: payload = lz77_compress(in, Lz77Config(), st); break;
        case Algorithm::HUFFMAN: payload = huffman_compress(in, st); break;
        case Algorithm::RLE: payload = rle_compress(in, st); break;
        case Algorithm::XOR:
        case Algorithm::DELTA: payload = xor_compress(in, st); break;
        case Algorithm::NONE:
        case Algorithm::AUTO:
        default:
            payload = in;
            if (st) {
                st->input_bytes = in.size();
                st->output_bytes = payload.size();
                st->literal_bytes = in.size();
                st->match_count = 0;
            }
            break;
    }
    std::vector<std::uint8_t> out;
    out.reserve(payload.size() + 11);
    out.push_back('L');
    out.push_back('C');
    out.push_back(1);
    out.push_back(static_cast<std::uint8_t>(algo));
    write_u32(out, static_cast<std::uint32_t>(in.size()));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

std::vector<std::uint8_t> decompress(const std::vector<std::uint8_t>& in) {
    if (in.size() < 8 || in[0] != 'L' || in[1] != 'C' || in[2] != 1) return {};
    const Algorithm algo = static_cast<Algorithm>(in[3]);
    const std::uint32_t expect = read_u32(in.data() + 4);
    const std::vector<std::uint8_t> payload(in.begin() + 8, in.end());
    std::vector<std::uint8_t> out;
    switch (algo) {
        case Algorithm::LZ77: out = lz77_decompress(payload); break;
        case Algorithm::HUFFMAN: out = huffman_decompress(payload); break;
        case Algorithm::RLE: out = rle_decompress(payload); break;
        case Algorithm::XOR:
        case Algorithm::DELTA: out = xor_decompress(payload); break;
        case Algorithm::NONE: out = payload; break;
        case Algorithm::AUTO: return {};
    }
    // The stored length is the integrity check that catches a wrong coder.
    if (out.size() != expect) return {};
    return out;
}

std::vector<std::uint8_t> compress_auto(const std::vector<std::uint8_t>& in, Algorithm* chosen,
                                        CompressStats* st) {
    struct Trial {
        Algorithm algo;
        std::vector<std::uint8_t> payload;
        CompressStats stats;
    };
    std::vector<Trial> trials;
    trials.push_back(Trial{Algorithm::RLE, rle_compress(in, nullptr), CompressStats()});
    trials.push_back(Trial{Algorithm::LZ77, lz77_compress(in, Lz77Config(), nullptr), CompressStats()});
    trials.push_back(Trial{Algorithm::HUFFMAN, huffman_compress(in, nullptr), CompressStats()});
    trials.push_back(Trial{Algorithm::XOR, xor_compress(in, nullptr), CompressStats()});
    // Re-run the winner with stats so the numbers match the stream that ships.
    std::size_t best = 0;
    for (std::size_t i = 1; i < trials.size(); ++i)
        if (trials[i].payload.size() < trials[best].payload.size()) best = i;

    const Algorithm algo = trials[best].algo;
    std::vector<std::uint8_t> framed;
    switch (algo) {
        case Algorithm::RLE: framed = compress(in, Algorithm::RLE, st); break;
        case Algorithm::LZ77: framed = compress(in, Algorithm::LZ77, st); break;
        case Algorithm::HUFFMAN: framed = compress(in, Algorithm::HUFFMAN, st); break;
        default: framed = compress(in, Algorithm::XOR, st); break;
    }
    if (chosen) *chosen = algo;
    return framed;
}

// ---------------------------------------------------------------- hash

std::uint64_t directory_hash(const std::uint8_t* data, std::size_t len) {
    std::uint64_t h = 0xCBF29CE484222325ULL;
    for (std::size_t i = 0; i < len; ++i) {
        h ^= data[i];
        h *= 0x100000001B3ULL;
        h = (h << 17) | (h >> 47);
    }
    return h;
}

std::uint64_t directory_hash(const std::vector<std::uint8_t>& data) {
    return directory_hash(data.data(), data.size());
}

std::string hex_u64(std::uint64_t v) {
    static const char* digits = "0123456789abcdef";
    std::string out = "0x";
    for (int i = 15; i >= 0; --i) out.push_back(digits[(v >> (i * 4)) & 0xF]);
    return out;
}

} // namespace lowl
