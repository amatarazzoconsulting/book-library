// MIT License - (c) Anthony Matarazzo 2026
//
// audit_log.cpp - chained checksum audit log.

#include "audit_log.hpp"

#include "sha256.hpp"

#include <algorithm>
#include <array>
#include <sstream>

namespace srsp {
namespace audit {

namespace {

void put_u64(bytes& out, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xffu));
}

void put_str(bytes& out, const std::string& s) {
    put_u64(out, s.size());
    out.insert(out.end(), s.begin(), s.end());
}

void put_blob(bytes& out, const bytes& b) {
    put_u64(out, b.size());
    out.insert(out.end(), b.begin(), b.end());
}

std::string escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else out.push_back(c);
    }
    return out;
}

std::string unescape(const std::string& s) {
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            const char n = s[++i];
            if (n == 'n') out.push_back('\n');
            else if (n == 't') out.push_back('\t');
            else out.push_back(n);
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

bytes from_hex(const std::string& hex) {
    bytes out;
    if (hex.size() % 2 != 0) return out;
    auto val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        const int hi = val(hex[i]);
        const int lo = val(hex[i + 1]);
        if (hi < 0 || lo < 0) return {};
        out.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
    }
    return out;
}

}  // namespace

const char* to_string(severity s) {
    switch (s) {
        case severity::info: return "info";
        case severity::notice: return "notice";
        case severity::warning: return "warning";
        case severity::critical: return "critical";
    }
    return "unknown";
}

bool parse_severity(const std::string& name, severity& out) {
    if (name == "info") { out = severity::info; return true; }
    if (name == "notice") { out = severity::notice; return true; }
    if (name == "warning") { out = severity::warning; return true; }
    if (name == "critical") { out = severity::critical; return true; }
    return false;
}

const char* to_string(break_kind k) {
    switch (k) {
        case break_kind::none: return "none";
        case break_kind::bad_index: return "bad_index";
        case break_kind::broken_prev: return "broken_prev";
        case break_kind::broken_hash: return "broken_hash";
        case break_kind::bad_mac: return "bad_mac";
        case break_kind::truncated: return "truncated";
    }
    return "unknown";
}

audit_log::audit_log(options opt) : opt_(opt) {
    if (opt_.hash_len == 0) opt_.hash_len = 32;
    if (opt_.hash_len > 32) opt_.hash_len = 32;
}

void audit_log::set_key(const bytes& key) {
    key_ = key;
    opt_.keyed = !key.empty();
    // Recomputing existing hashes after a key install is intentional: the log
    // is re-sealed with the new authentication key.
    reseal();
}

void audit_log::tick(std::uint64_t amount) { clock_ += amount; }

bytes audit_log::compute_hash(const record& r) const {
    bytes buf;
    put_u64(buf, r.index);
    put_u64(buf, r.epoch);
    put_u64(buf, r.logical_time);
    put_u64(buf, static_cast<std::uint64_t>(r.level));
    put_str(buf, r.actor);
    put_str(buf, r.action);
    put_str(buf, r.detail);
    if (opt_.include_payload) put_blob(buf, r.payload);
    put_blob(buf, r.prev_hash);

    std::array<std::uint8_t, 32> d{};
    if (opt_.keyed && !key_.empty()) {
        d = hash::hmac(key_, buf);
    } else {
        d = hash::sha256::digest(buf.data(), buf.size());
    }
    return bytes(d.begin(), d.begin() + static_cast<std::ptrdiff_t>(opt_.hash_len));
}

std::optional<record> audit_log::append(const append_request& req) {
    if (opt_.max_records > 0 && records_.size() >= opt_.max_records) return std::nullopt;

    record r;
    r.index = records_.size();
    r.epoch = req.epoch;
    r.logical_time = clock_;
    r.level = req.level;
    r.actor = req.actor;
    r.action = req.action;
    r.detail = req.detail;
    r.payload = req.payload;
    r.prev_hash = head_hash();
    r.hash = compute_hash(r);

    records_.push_back(r);
    ++writes_;
    return r;
}

std::optional<record> audit_log::append(severity level, const std::string& actor,
                                        const std::string& action, const std::string& detail) {
    append_request req;
    req.level = level;
    req.actor = actor;
    req.action = action;
    req.detail = detail;
    req.epoch = 0;
    return append(req);
}

std::optional<record> audit_log::at(std::size_t index) const {
    if (index >= records_.size()) return std::nullopt;
    return records_[index];
}

bytes audit_log::head_hash() const {
    if (records_.empty()) return bytes(opt_.hash_len, 0x00);
    return records_.back().hash;
}

std::string audit_log::head_hex() const { return hash::to_hex(head_hash()); }

verify_report audit_log::verify() const {
    verify_report rep;
    bytes prev(opt_.hash_len, 0x00);
    for (std::size_t i = 0; i < records_.size(); ++i) {
        const record& r = records_[i];
        rep.checked = i + 1;
        if (r.index != i) {
            rep.ok = false;
            rep.bad_index = i;
            rep.kind = break_kind::bad_index;
            rep.reason = "record " + std::to_string(i) + " carries index " +
                         std::to_string(r.index);
            rep.head_hash = head_hash();
            rep.root_hash = merkle_root();
            return rep;
        }
        if (r.prev_hash != prev) {
            rep.ok = false;
            rep.bad_index = i;
            rep.kind = break_kind::broken_prev;
            rep.reason = "record " + std::to_string(i) + " prev_hash does not match its parent";
            rep.head_hash = head_hash();
            rep.root_hash = merkle_root();
            return rep;
        }
        const bytes expect = compute_hash(r);
        if (expect != r.hash) {
            rep.ok = false;
            rep.bad_index = i;
            rep.kind = opt_.keyed ? break_kind::bad_mac : break_kind::broken_hash;
            rep.reason = "record " + std::to_string(i) + " contents do not match its hash";
            rep.head_hash = head_hash();
            rep.root_hash = merkle_root();
            return rep;
        }
        prev = r.hash;
    }
    rep.ok = true;
    rep.checked = records_.size();
    rep.kind = break_kind::none;
    rep.reason = "chain intact";
    rep.head_hash = head_hash();
    rep.root_hash = merkle_root();
    return rep;
}

verify_report audit_log::verify(const bytes& expected_head) const {
    verify_report rep = verify();
    if (!rep.ok) return rep;
    if (expected_head != head_hash()) {
        rep.ok = false;
        rep.kind = break_kind::truncated;
        rep.reason = "head hash differs from the expected head, log was truncated or extended";
    }
    return rep;
}

bytes audit_log::merkle_root() const {
    if (records_.empty()) return bytes(opt_.hash_len, 0x00);
    std::vector<bytes> level;
    level.reserve(records_.size());
    for (const auto& r : records_) level.push_back(r.hash);
    while (level.size() > 1) {
        std::vector<bytes> next;
        for (std::size_t i = 0; i < level.size(); i += 2) {
            const bytes& a = level[i];
            const bytes& b = (i + 1 < level.size()) ? level[i + 1] : level[i];
            bytes buf = a;
            buf.insert(buf.end(), b.begin(), b.end());
            const auto d = hash::sha256::digest(buf.data(), buf.size());
            next.emplace_back(d.begin(), d.begin() + static_cast<std::ptrdiff_t>(opt_.hash_len));
        }
        level.swap(next);
    }
    return level.front();
}

std::vector<bytes> audit_log::inclusion_proof(std::size_t index) const {
    std::vector<bytes> proof;
    if (index >= records_.size()) return proof;
    std::vector<bytes> level;
    for (const auto& r : records_) level.push_back(r.hash);
    std::size_t pos = index;
    while (level.size() > 1) {
        std::vector<bytes> next;
        for (std::size_t i = 0; i < level.size(); i += 2) {
            const bytes& a = level[i];
            const bytes& b = (i + 1 < level.size()) ? level[i + 1] : level[i];
            bytes buf = a;
            buf.insert(buf.end(), b.begin(), b.end());
            const auto d = hash::sha256::digest(buf.data(), buf.size());
            next.emplace_back(d.begin(), d.begin() + static_cast<std::ptrdiff_t>(opt_.hash_len));
        }
        const std::size_t sibling = (pos % 2 == 0) ? pos + 1 : pos - 1;
        proof.push_back(sibling < level.size() ? level[sibling] : level[pos]);
        pos /= 2;
        level.swap(next);
    }
    return proof;
}

bool audit_log::verify_inclusion(const bytes& leaf, const std::vector<bytes>& proof,
                                 const bytes& root, std::size_t index, std::size_t count) {
    (void)count;
    bytes cur = leaf;
    std::size_t pos = index;
    for (const auto& sib : proof) {
        bytes buf;
        if (pos % 2 == 0) {
            buf = cur;
            buf.insert(buf.end(), sib.begin(), sib.end());
        } else {
            buf = sib;
            buf.insert(buf.end(), cur.begin(), cur.end());
        }
        const auto d = hash::sha256::digest(buf.data(), buf.size());
        cur.assign(d.begin(), d.begin() + static_cast<std::ptrdiff_t>(leaf.size()));
        pos /= 2;
    }
    return cur == root;
}

bool audit_log::tamper_detail(std::size_t index, const std::string& new_detail) {
    if (index >= records_.size()) return false;
    records_[index].detail = new_detail;
    return true;
}

bool audit_log::tamper_payload(std::size_t index, const bytes& new_payload) {
    if (index >= records_.size()) return false;
    records_[index].payload = new_payload;
    return true;
}

bool audit_log::drop_record(std::size_t index) {
    if (index >= records_.size()) return false;
    records_.erase(records_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

bool audit_log::swap_records(std::size_t a, std::size_t b) {
    if (a >= records_.size() || b >= records_.size() || a == b) return false;
    std::swap(records_[a], records_[b]);
    return true;
}

bool audit_log::truncate_to(std::size_t count) {
    if (count > records_.size()) return false;
    records_.resize(count);
    return true;
}

std::string audit_log::serialize() const {
    std::ostringstream os;
    os << "SRSPAUDIT 1\n";
    os << "count " << records_.size() << "\n";
    os << "keyed " << (opt_.keyed ? 1 : 0) << "\n";
    for (const auto& r : records_) {
        os << "R " << r.index << ' ' << r.epoch << ' ' << r.logical_time << ' '
           << static_cast<int>(r.level) << ' '
           << (r.actor.empty() ? std::string("-") : escape(r.actor)) << ' '
           << (r.action.empty() ? std::string("-") : escape(r.action)) << ' '
           << (r.detail.empty() ? std::string("-") : escape(r.detail)) << ' '
           << (r.payload.empty() ? std::string("-") : hash::to_hex(r.payload)) << ' '
           << hash::to_hex(r.prev_hash) << ' ' << hash::to_hex(r.hash) << "\n";
    }
    return os.str();
}

bool audit_log::deserialize(const std::string& text, audit_log& out, const bytes& key) {
    std::istringstream is(text);
    std::string magic;
    std::size_t count = 0;
    int keyed = 0;
    if (!(is >> magic) || magic != "SRSPAUDIT") return false;
    std::string version;
    if (!(is >> version) || version != "1") return false;
    std::string tag;
    if (!(is >> tag) || tag != "count" || !(is >> count)) return false;
    if (!(is >> tag) || tag != "keyed" || !(is >> keyed)) return false;

    options opts = out.settings();
    opts.keyed = keyed != 0;
    out = audit_log(opts);
    out.records_.clear();
    out.key_ = key;
    if (opts.keyed && key.empty()) out.opt_.keyed = true;

    for (std::size_t i = 0; i < count; ++i) {
        std::string marker;
        if (!(is >> marker) || marker != "R") return false;
        record r;
        std::uint64_t level = 0;
        std::string actor, action, detail, payload_hex, prev_hex, hash_hex;
        if (!(is >> r.index >> r.epoch >> r.logical_time >> level)) return false;
        if (!(is >> actor >> action >> detail >> payload_hex >> prev_hex >> hash_hex)) return false;
        r.level = static_cast<severity>(level);
        r.actor = (actor == "-") ? std::string{} : unescape(actor);
        r.action = (action == "-") ? std::string{} : unescape(action);
        r.detail = (detail == "-") ? std::string{} : unescape(detail);
        r.payload = (payload_hex == "-") ? bytes{} : from_hex(payload_hex);
        r.prev_hash = from_hex(prev_hex);
        r.hash = from_hex(hash_hex);
        out.records_.push_back(r);
    }
    return true;
}

void audit_log::reseal() {
    bytes prev(opt_.hash_len, 0x00);
    for (std::size_t i = 0; i < records_.size(); ++i) {
        records_[i].index = i;
        records_[i].prev_hash = prev;
        records_[i].hash = compute_hash(records_[i]);
        prev = records_[i].hash;
    }
}

}  // namespace audit
}  // namespace srsp
