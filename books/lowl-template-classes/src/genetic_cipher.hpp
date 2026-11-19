// MIT License - (c) Anthony Matarazzo 2026
//
// genetic_cipher.hpp - the Genetic Cipher from LOWL Class Examples, rebuilt in
// portable C++17. Data is viewed as elements of BitMode width; a key derived
// pipeline of reversible operations (scatter, substitute, diffuse, mix, ...)
// transforms those elements, and the inverse pipeline restores them.
#ifndef LOWL_GENETIC_CIPHER_HPP
#define LOWL_GENETIC_CIPHER_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace lowl {

// Bit access modes from crypto_types.lowl
enum class BitMode : std::uint8_t {
    BIT_1 = 1,
    BIT_2 = 2,
    BIT_3 = 3,
    NIBBLE = 4,
    BIT_5 = 5,
    BIT_6 = 6,
    BYTE = 8,
    WORD16 = 16,
    WORD32 = 32,
    WORD64 = 64,
    BLOCK = 128
};

const char* bit_mode_name(BitMode mode);
std::size_t bit_mode_width(BitMode mode);
// True when the enum value is one of the declared modes.
bool bit_mode_valid(std::uint8_t raw);

// ChaCha20 based PRNG (CryptographicPRNG in the book).
class CryptoPrng {
public:
    CryptoPrng();
    // Mixes the two passwords into a 32 byte key and 12 byte nonce, exactly
    // like CryptographicPRNG::seed in the book, then restarts the stream.
    void seed(const std::string& user_password, const std::string& electronic_password);
    void reset();                       // rewind to the first block
    std::uint64_t next_u64();
    std::uint64_t range(std::uint64_t lo, std::uint64_t hi);   // inclusive
    std::vector<std::uint32_t> permutation(std::size_t n);
    std::uint64_t blocks_generated() const { return blocks_; }
    const std::array<std::uint8_t, 32>& key() const { return key_; }
    const std::array<std::uint8_t, 12>& nonce() const { return nonce_; }

private:
    std::array<std::uint8_t, 32> key_{};
    std::array<std::uint8_t, 12> nonce_{};
    std::array<std::uint64_t, 16> state_{};
    std::array<std::uint64_t, 16> buffer_{};
    std::size_t buffer_pos_ = 16;
    std::uint64_t counter_ = 0;
    std::uint64_t blocks_ = 0;
    void init_state();
    void generate_block();
};

struct CipherSettings {
    BitMode bit_mode = BitMode::BYTE;
    std::uint32_t shuffle_passes = 2;
    std::uint32_t diffusion_rounds = 2;
    std::uint32_t scatter_passes = 1;
    bool enable_rotation = true;
    bool enable_flip = true;
    BitMode cycle_mode = BitMode::NIBBLE;   // target width for OpModeCycle
};

// Shared mutable state handed to every operation.
struct CipherState {
    std::vector<unsigned __int128> elements;
    std::size_t bit_count = 0;     // exact number of meaningful bits
    std::size_t width = 8;         // current element width in bits
    unsigned __int128 mask = 0xFF;
    CryptoPrng* prng = nullptr;
    const CipherSettings* settings = nullptr;
    std::vector<BitMode> mode_stack;
    std::array<std::uint8_t, 32> master_key{};   // per-operation sub-PRNG seed
};

// Operation tags. forward() and inverse() must consume PRNG draws in the same
// order so that a reset stream replays identically.
struct OpScatter {
    static const char* name() { return "Scatter"; }
    static void forward(CipherState& s);
    static void inverse(CipherState& s);
};
struct OpSubstitute {
    static const char* name() { return "Substitute"; }
    static void forward(CipherState& s);
    static void inverse(CipherState& s);
};
struct OpDiffuse {
    static const char* name() { return "Diffuse"; }
    static void forward(CipherState& s);
    static void inverse(CipherState& s);
};
struct OpMix {
    static const char* name() { return "Mix"; }
    static void forward(CipherState& s);
    static void inverse(CipherState& s) { forward(s); }
};
struct OpBitwiseXor {
    static const char* name() { return "BitwiseXor"; }
    static void forward(CipherState& s);
    static void inverse(CipherState& s) { forward(s); }
};
struct OpBitwiseNot {
    static const char* name() { return "BitwiseNot"; }
    static void forward(CipherState& s);
    static void inverse(CipherState& s) { forward(s); }
};
struct OpBitwiseRotL {
    static const char* name() { return "BitwiseRotL"; }
    static void forward(CipherState& s);
    static void inverse(CipherState& s);
};
struct OpFlip {
    static const char* name() { return "Flip"; }
    static void forward(CipherState& s);
    static void inverse(CipherState& s) { forward(s); }
};
struct OpRotate {
    static const char* name() { return "Rotate"; }
    static void forward(CipherState& s);
    static void inverse(CipherState& s);
};
struct OpModeCycle {
    static const char* name() { return "ModeCycle"; }
    static void forward(CipherState& s);
    static void inverse(CipherState& s);
};

// Repack helpers shared by the operations and the public API.
std::vector<unsigned __int128> bytes_to_elements(const std::uint8_t* data, std::size_t len,
                                                 std::size_t width, std::size_t* bit_count);
std::vector<std::uint8_t> elements_to_bytes(const std::vector<unsigned __int128>& elems,
                                            std::size_t width, std::size_t bit_count);
// Re-groups the element stream into new_width wide elements, preserving the
// exact bit sequence and the padding above bit_count.
void repack(CipherState& s, std::size_t new_width);
unsigned __int128 width_mask(std::size_t width);
unsigned __int128 rotl128(unsigned __int128 v, unsigned n, std::size_t width);
unsigned __int128 rotr128(unsigned __int128 v, unsigned n, std::size_t width);

// Pipeline of operations applied in template order.
template <typename... Ops>
class GeneticCipher {
public:
    GeneticCipher() = default;

    void set_password(const std::string& user_password,
                      const std::string& electronic_password = std::string()) {
        user_password_ = user_password;
        electronic_password_ = electronic_password;
        prng_.seed(user_password_, electronic_password_);
    }

    CipherSettings& settings() { return settings_; }
    const CipherSettings& settings() const { return settings_; }

    // Returns "GC" + version + mode + 4 byte length + payload.
    // Empty input yields empty output.
    std::vector<std::uint8_t> encrypt(const std::vector<std::uint8_t>& plain) {
        if (plain.empty()) return {};
        CipherState s = load(plain.data(), plain.size(), settings_.bit_mode);
        prng_.reset();
        (Ops::forward(s), ...);
        std::vector<std::uint8_t> out = elements_to_bytes(s.elements, s.width, s.bit_count);
        std::vector<std::uint8_t> framed;
        framed.reserve(out.size() + 8);
        framed.push_back('G');
        framed.push_back('C');
        framed.push_back(1);
        framed.push_back(static_cast<std::uint8_t>(settings_.bit_mode));
        const std::uint32_t n = static_cast<std::uint32_t>(plain.size());
        for (int i = 0; i < 4; ++i) framed.push_back(static_cast<std::uint8_t>(n >> (i * 8)));
        framed.insert(framed.end(), out.begin(), out.end());
        return framed;
    }

    std::vector<std::uint8_t> decrypt(const std::vector<std::uint8_t>& framed) {
        if (framed.empty()) return {};
        if (framed.size() < 8 || framed[0] != 'G' || framed[1] != 'C' || framed[2] != 1)
            return {};
        if (!bit_mode_valid(framed[3])) return {};
        std::uint32_t n = 0;
        for (int i = 0; i < 4; ++i) n |= static_cast<std::uint32_t>(framed[4 + i]) << (i * 8);
        CipherState s = load(framed.data() + 8, framed.size() - 8,
                             static_cast<BitMode>(framed[3]));
        // A payload whose bit count does not match the recorded length is corrupt.
        if (s.bit_count != static_cast<std::size_t>(n) * 8) return {};
        prng_.reset();
        reverse_apply(s);
        return elements_to_bytes(s.elements, s.width, s.bit_count);
    }

    // Bit mode switching stack (push / pop), mirroring m_mode_stack.
    void push_mode(BitMode m) { settings_.bit_mode = m; }
    void pop_mode() { settings_.bit_mode = BitMode::BYTE; }
    std::uint64_t prng_blocks() const { return prng_.blocks_generated(); }
    const std::string& password() const { return user_password_; }
    const CryptoPrng& prng() const { return prng_; }

    static const char* pipeline_name() { return pipeline_string().c_str(); }

private:
    CipherState load(const std::uint8_t* data, std::size_t len, BitMode mode) {
        CipherState s;
        s.settings = &settings_;
        s.prng = &prng_;
        s.width = bit_mode_width(mode);
        s.mask = width_mask(s.width);
        s.elements = bytes_to_elements(data, len, s.width, &s.bit_count);
        s.mode_stack.push_back(mode);
        s.master_key = prng_.key();
        return s;
    }

    void reverse_apply(CipherState& s) {
        using Inv = void (*)(CipherState&);
        Inv fns[] = {&Ops::inverse...};
        for (std::size_t i = sizeof...(Ops); i-- > 0;) fns[i](s);
    }

    static std::string pipeline_string() {
        std::string out;
        const char* names[] = {Ops::name()...};
        for (std::size_t i = 0; i < sizeof...(Ops); ++i) {
            if (i) out += " -> ";
            out += names[i];
        }
        return out;
    }

    CryptoPrng prng_;
    CipherSettings settings_;
    std::string user_password_;
    std::string electronic_password_;
};

} // namespace lowl

#endif // LOWL_GENETIC_CIPHER_HPP
