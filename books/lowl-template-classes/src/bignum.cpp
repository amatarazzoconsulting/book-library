// MIT License - (c) Anthony Matarazzo 2026
//
// bignum.cpp - implementation of the BigNum class.
#include "bignum.hpp"

#include <algorithm>
#include <cctype>
#include <limits>

namespace lowl {

BigNum BigNum::from_u64(std::uint64_t value) {
    BigNum bn;
    if (value != 0) bn.limbs_.push_back(value);
    bn.negative_ = false;
    return bn;
}

BigNum BigNum::from_i64(std::int64_t value) {
    BigNum bn;
    bn.negative_ = value < 0;
    std::uint64_t magnitude;
    if (value == std::numeric_limits<std::int64_t>::min()) {
        magnitude = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1u;
    } else if (value < 0) {
        magnitude = static_cast<std::uint64_t>(-value);
    } else {
        magnitude = static_cast<std::uint64_t>(value);
    }
    if (magnitude != 0) bn.limbs_.push_back(magnitude);
    if (bn.limbs_.empty()) bn.negative_ = false;
    return bn;
}

void BigNum::trim() {
    while (!limbs_.empty() && limbs_.back() == 0) limbs_.pop_back();
    if (limbs_.empty()) negative_ = false;
}

std::uint64_t BigNum::to_u64() const { return limbs_.empty() ? 0 : limbs_[0]; }

bool BigNum::fits_i64() const {
    if (limbs_.size() > 1) return false;
    const std::uint64_t limit = negative_
        ? static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1u
        : static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    return to_u64() <= limit;
}

std::int64_t BigNum::to_i64() const {
    std::uint64_t v = to_u64();
    if (negative_) return static_cast<std::int64_t>(0u - v);
    return static_cast<std::int64_t>(v);
}

BigNum BigNum::absolute() const {
    BigNum bn = *this;
    bn.negative_ = false;
    return bn;
}

BigNum BigNum::negate() const {
    BigNum bn = *this;
    if (!bn.is_zero()) bn.negative_ = !bn.negative_;
    return bn;
}

int BigNum::compare_magnitude(const BigNum& a, const BigNum& b) {
    if (a.limbs_.size() != b.limbs_.size())
        return a.limbs_.size() < b.limbs_.size() ? -1 : 1;
    for (std::size_t i = a.limbs_.size(); i-- > 0;) {
        if (a.limbs_[i] != b.limbs_[i]) return a.limbs_[i] < b.limbs_[i] ? -1 : 1;
    }
    return 0;
}

int BigNum::compare(const BigNum& other) const {
    if (is_zero() && other.is_zero()) return 0;
    if (negative_ != other.negative_) return negative_ ? -1 : 1;
    int mag = compare_magnitude(*this, other);
    return negative_ ? -mag : mag;
}

BigNum BigNum::add_magnitude(const BigNum& a, const BigNum& b) {
    BigNum r;
    const std::size_t n = std::max(a.limbs_.size(), b.limbs_.size());
    r.limbs_.assign(n, 0);
    std::uint64_t carry = 0;
    for (std::size_t i = 0; i < n; ++i) {
        const std::uint64_t av = a.limb(i);
        const std::uint64_t bv = b.limb(i);
        const std::uint64_t sum = av + bv;
        const std::uint64_t with_carry = sum + carry;
        // overflow happened if sum wrapped, or the carry addition wrapped
        const std::uint64_t new_carry = (sum < av) ? 1u : 0u;
        r.limbs_[i] = with_carry;
        carry = new_carry + ((with_carry < sum) ? 1u : 0u);
    }
    if (carry != 0) r.limbs_.push_back(carry);
    return r;
}

BigNum BigNum::sub_magnitude(const BigNum& a, const BigNum& b) {
    BigNum r;
    r.limbs_.assign(a.limbs_.size(), 0);
    std::uint64_t borrow = 0;
    for (std::size_t i = 0; i < a.limbs_.size(); ++i) {
        const std::uint64_t av = a.limbs_[i];
        const std::uint64_t bv = b.limb(i);
        const std::uint64_t diff = av - bv;
        const std::uint64_t with_borrow = diff - borrow;
        borrow = (av < bv) ? 1u : 0u;
        if (diff < borrow) borrow += 1u;
        r.limbs_[i] = with_borrow;
    }
    r.trim();
    return r;
}

BigNum BigNum::add(const BigNum& other) const {
    if (is_zero()) return other;
    if (other.is_zero()) return *this;
    if (negative_ == other.negative_) {
        BigNum r = add_magnitude(*this, other);
        r.negative_ = negative_;
        r.trim();
        return r;
    }
    const int mag = compare_magnitude(*this, other);
    if (mag == 0) return BigNum();
    if (mag > 0) {
        BigNum r = sub_magnitude(*this, other);
        r.negative_ = negative_;
        return r;
    }
    BigNum r = sub_magnitude(other, *this);
    r.negative_ = other.negative_;
    return r;
}

BigNum BigNum::sub(const BigNum& other) const { return add(other.negate()); }

BigNum BigNum::mul(const BigNum& other) const {
    if (is_zero() || other.is_zero()) return BigNum();
    BigNum r;
    r.limbs_.assign(limbs_.size() + other.limbs_.size(), 0);
    for (std::size_t i = 0; i < limbs_.size(); ++i) {
        std::uint64_t carry = 0;
        for (std::size_t j = 0; j < other.limbs_.size(); ++j) {
            // 64x64 -> 128 bit product accumulated in place
            const unsigned __int128 cur =
                static_cast<unsigned __int128>(limbs_[i]) * other.limbs_[j] +
                r.limbs_[i + j] + carry;
            r.limbs_[i + j] = static_cast<std::uint64_t>(cur);
            carry = static_cast<std::uint64_t>(cur >> 64);
        }
        r.limbs_[i + other.limbs_.size()] += carry;
    }
    r.negative_ = (negative_ != other.negative_);
    r.trim();
    return r;
}

BigNum BigNum::shl(unsigned bits) const {
    if (is_zero() || bits == 0) return *this;
    const unsigned limb_shift = bits / 64;
    const unsigned bit_shift = bits % 64;
    BigNum r;
    r.limbs_.assign(limbs_.size() + limb_shift + 1, 0);
    for (std::size_t i = 0; i < limbs_.size(); ++i) {
        const std::uint64_t v = limbs_[i];
        r.limbs_[i + limb_shift] |= v << bit_shift;
        if (bit_shift != 0) r.limbs_[i + limb_shift + 1] |= v >> (64 - bit_shift);
    }
    r.negative_ = negative_;
    r.trim();
    return r;
}

BigNum BigNum::shr(unsigned bits) const {
    if (is_zero() || bits == 0) return *this;
    const unsigned limb_shift = bits / 64;
    const unsigned bit_shift = bits % 64;
    if (limb_shift >= limbs_.size()) return BigNum();
    BigNum r;
    r.limbs_.assign(limbs_.size() - limb_shift, 0);
    for (std::size_t i = 0; i < r.limbs_.size(); ++i) {
        std::uint64_t v = limbs_[i + limb_shift] >> bit_shift;
        if (bit_shift != 0 && i + limb_shift + 1 < limbs_.size())
            v |= limbs_[i + limb_shift + 1] << (64 - bit_shift);
        r.limbs_[i] = v;
    }
    r.negative_ = negative_;
    r.trim();
    return r;
}

std::uint32_t BigNum::divmod_small(std::uint32_t divisor) {
    if (divisor == 0) return 0;
    unsigned __int128 rem = 0;
    for (std::size_t i = limbs_.size(); i-- > 0;) {
        const unsigned __int128 cur = (rem << 64) | limbs_[i];
        limbs_[i] = static_cast<std::uint64_t>(cur / divisor);
        rem = cur % divisor;
    }
    trim();
    return static_cast<std::uint32_t>(rem);
}

std::size_t BigNum::bit_length() const {
    if (limbs_.empty()) return 0;
    std::size_t bits = (limbs_.size() - 1) * 64;
    std::uint64_t top = limbs_.back();
    while (top != 0) { ++bits; top >>= 1; }
    return bits;
}

std::string BigNum::to_string(int base) const {
    static const char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    if (base < 2 || base > 36) base = 10;
    if (is_zero()) return "0";

    BigNum work = absolute();
    std::string out;
    while (!work.is_zero()) {
        const std::uint32_t rem = work.divmod_small(static_cast<std::uint32_t>(base));
        out.push_back(digits[rem]);
    }
    if (negative_) out.push_back('-');
    std::reverse(out.begin(), out.end());
    return out;
}

bool BigNum::parse(const std::string& text, int base, BigNum& out) {
    if (base < 2 || base > 36) return false;
    std::size_t i = 0;
    bool neg = false;
    if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
        neg = text[i] == '-';
        ++i;
    }
    if (i == text.size()) return false;

    BigNum acc;
    bool any = false;
    for (; i < text.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == '_') continue;
        int digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'z') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') digit = c - 'A' + 10;
        else return false;
        if (digit >= base) return false;
        acc = acc.mul(BigNum::from_u64(static_cast<std::uint64_t>(base)))
                 .add(BigNum::from_u64(static_cast<std::uint64_t>(digit)));
        any = true;
    }
    if (!any) return false;
    acc.negative_ = neg && !acc.is_zero();
    out = acc;
    return true;
}

BigNum BigNum::from_string(const std::string& text, int base) {
    BigNum bn;
    BigNum::parse(text, base, bn);
    return bn;
}

} // namespace lowl
