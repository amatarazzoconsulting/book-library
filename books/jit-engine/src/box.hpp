// MIT License - (c) Anthony Matarazzo 2026
//
// box.hpp - Box: the tagged value container that every layer of the JIT
// engine passes around. A Box holds exactly one value kind. Small values
// (integers, floats, bool) live inline in a union so a Box copy never
// allocates. Heap values (string, vector) sit behind std::shared_ptr, so a
// copy is a refcount bump and the value is never leaked or double freed.
//
// This is the "jit_box.h" layer of the book, reduced to the 14 kinds the
// rest of this source tree actually needs.

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace jitx {

class Box {
public:
    enum class Type {
        VOID, BOOL,
        I8, U8, I16, U16, I32, U32, I64, U64,
        F32, F64, STRING, VECTOR
    };

    Box();
    explicit Box(bool v);
    explicit Box(int8_t v);
    explicit Box(uint8_t v);
    explicit Box(int16_t v);
    explicit Box(uint16_t v);
    explicit Box(int32_t v);
    explicit Box(uint32_t v);
    explicit Box(int64_t v);
    explicit Box(uint64_t v);
    explicit Box(float v);
    explicit Box(double v);
    explicit Box(const char* v);
    explicit Box(const std::string& v);

    // Container factory: an empty vector Box.
    static Box makeVector();

    Type type() const { return type_; }
    const char* typeName() const;
    static const char* typeName(Type t);

    bool isNumeric() const;
    bool isInteger() const;

    bool toBool() const;
    int64_t toInt64() const;
    uint64_t toUInt64() const;
    double toDouble() const;
    std::string toString() const;

    template <typename T>
    T as() const {
        if constexpr (std::is_same_v<T, bool>) return toBool();
        else if constexpr (std::is_same_v<T, int64_t>) return toInt64();
        else if constexpr (std::is_same_v<T, uint64_t>) return toUInt64();
        else if constexpr (std::is_same_v<T, double>) return toDouble();
        else if constexpr (std::is_same_v<T, float>) return static_cast<float>(toDouble());
        else if constexpr (std::is_same_v<T, int>) return static_cast<int>(toInt64());
        else if constexpr (std::is_same_v<T, std::string>) return toString();
        else return T{};
    }

    // Arithmetic. Two integer operands stay integer for + - * %, everything
    // else promotes to double (the book's rule). Division always produces a
    // double so 7 / 2 is 3.5 and never silently truncates.
    Box operator+(const Box& other) const;
    Box operator-(const Box& other) const;
    Box operator*(const Box& other) const;
    Box operator/(const Box& other) const;
    Box operator%(const Box& other) const;

    // Bitwise, integer domain only. Shifts are the exception: for a
    // non-integer operand they scale by a power of two, exactly what a shift
    // means for an integer, so an optimizer may rewrite x * 8 as x << 3
    // without changing any result.
    Box operator&(const Box& other) const;
    Box operator|(const Box& other) const;
    Box operator^(const Box& other) const;
    Box operator~() const;
    Box operator<<(int shift) const;
    Box operator>>(int shift) const;

    // Comparison. Numeric against numeric compares numerically, string
    // against string compares lexicographically, mixed kinds are unequal.
    bool operator==(const Box& other) const;
    bool operator!=(const Box& other) const;
    bool operator<(const Box& other) const;
    bool operator<=(const Box& other) const;
    bool operator>(const Box& other) const;
    bool operator>=(const Box& other) const;

    // Container operations (VECTOR kind).
    void push_back(const Box& v);
    size_t size() const;
    bool empty() const;
    const Box& at(size_t i) const;

private:
    Type type_;
    union {
        bool b;
        int8_t i8;
        uint8_t u8;
        int16_t i16;
        uint16_t u16;
        int32_t i32;
        uint32_t u32;
        int64_t i64;
        uint64_t u64;
        float f32;
        double f64;
    };
    std::shared_ptr<std::string> str_;
    std::shared_ptr<std::vector<Box>> vec_;
};

} // namespace jitx
