// MIT License - (c) Anthony Matarazzo 2026
//
// bignum.hpp - arbitrary precision signed integer, the "BigNum" class from
// Part 2 of LOWL Class Examples, rebuilt as portable C++17.
//
// Representation: little endian vector of 64 bit limbs plus a sign flag.
// The empty limb vector is the canonical zero (never negative).
#ifndef LOWL_BIGNUM_HPP
#define LOWL_BIGNUM_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace lowl {

class BigNum {
public:
    BigNum() = default;

    static BigNum from_u64(std::uint64_t value);
    static BigNum from_i64(std::int64_t value);
    // base may be 2..36; accepts an optional leading '+' or '-'.
    // Returns false and leaves the object zero when the text is not valid.
    static bool parse(const std::string& text, int base, BigNum& out);
    static BigNum from_string(const std::string& text, int base = 10);

    bool is_zero() const { return limbs_.empty(); }
    bool is_negative() const { return negative_; }
    std::size_t limb_count() const { return limbs_.size(); }
    std::uint64_t limb(std::size_t i) const { return i < limbs_.size() ? limbs_[i] : 0; }
    std::uint64_t to_u64() const;          // low limb, truncating
    bool fits_i64() const;                 // exact round trip through to_i64
    std::int64_t to_i64() const;           // two's complement wrap if too large

    BigNum absolute() const;
    BigNum negate() const;
    void trim();

    int compare(const BigNum& other) const;      // -1, 0, 1

    BigNum add(const BigNum& other) const;
    BigNum sub(const BigNum& other) const;
    BigNum mul(const BigNum& other) const;
    BigNum shl(unsigned bits) const;              // multiply by 2^bits
    BigNum shr(unsigned bits) const;              // floor divide by 2^bits

    // Unsigned long division by a single limb. Returns the remainder.
    std::uint32_t divmod_small(std::uint32_t divisor);

    std::string to_string(int base = 10) const;   // "-" prefix when negative
    std::size_t bit_length() const;               // 0 for zero

    BigNum operator+(const BigNum& o) const { return add(o); }
    BigNum operator-(const BigNum& o) const { return sub(o); }
    BigNum operator*(const BigNum& o) const { return mul(o); }
    BigNum operator-() const { return negate(); }
    bool operator==(const BigNum& o) const { return compare(o) == 0; }
    bool operator!=(const BigNum& o) const { return compare(o) != 0; }
    bool operator<(const BigNum& o) const { return compare(o) < 0; }
    bool operator>(const BigNum& o) const { return compare(o) > 0; }

private:
    std::vector<std::uint64_t> limbs_;
    bool negative_ = false;

    static int compare_magnitude(const BigNum& a, const BigNum& b);
    static BigNum add_magnitude(const BigNum& a, const BigNum& b);
    // requires |a| >= |b|
    static BigNum sub_magnitude(const BigNum& a, const BigNum& b);
};

} // namespace lowl

#endif // LOWL_BIGNUM_HPP
