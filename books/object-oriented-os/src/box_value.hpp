// MIT License - (c) Anthony Matarazzo 2026
//
// NEBULA Box type system (book section 4). A tagged union value that every
// subsystem can pass around, with checked numeric conversions, arithmetic,
// bitwise operators and a vector container.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace nebula {

class Box {
public:
    enum class Type {
        VOID, BOOL, I8, U8, I16, U16, I32, U32, I64, U64,
        F32, F64, STRING, PTR, VECTOR, LIST, MAP, SET,
        FUNCTION, OBJECT, CLASS, METHOD,
        DATABASE, TABLE, JIT_COMPILER, BUFFER
    };

    Box();
    Box(bool value);
    Box(int value);
    Box(long long value);
    Box(unsigned value);
    Box(double value);
    Box(float value);
    Box(const char* value);
    Box(const std::string& value);
    Box(Type tag, std::int64_t raw);  // explicit integer tag construction
    Box(const Box& other);
    Box(Box&& other) noexcept;
    Box& operator=(const Box& other);
    Box& operator=(Box&& other) noexcept;
    ~Box();

    Type type() const { return type_; }
    static const char* typeName(Type t);
    const char* typeName() const { return typeName(type_); }
    bool isVoid() const { return type_ == Type::VOID; }
    bool isNumeric() const;
    bool isInteger() const;
    bool isString() const { return type_ == Type::STRING; }
    bool isVector() const { return type_ == Type::VECTOR; }

    // Checked conversions. asInt64 truncates floats; asDouble widens integers.
    bool asBool() const;
    std::int64_t asInt64() const;
    std::uint64_t asUint64() const;
    double asDouble() const;
    std::string asString() const;
    std::string asTypeString() const;  // type tag name only, no payload

    // True when the value fits losslessly in the target tag.
    bool fitsIn(Type target) const;

    // Arithmetic and bitwise. Operands are promoted: any double forces double,
    // any string forces string concatenation for +, otherwise 64-bit integers.
    Box operator+(const Box& other) const;
    Box operator-(const Box& other) const;
    Box operator*(const Box& other) const;
    Box operator/(const Box& other) const;
    Box operator%(const Box& other) const;
    Box operator&(const Box& other) const;
    Box operator|(const Box& other) const;
    Box operator^(const Box& other) const;
    Box operator~() const;
    Box operator-() const;
    bool operator==(const Box& other) const;
    bool operator!=(const Box& other) const { return !(*this == other); }
    bool operator<(const Box& other) const;

    // Vector helpers (Type::VECTOR).
    static Box makeVector();
    static Box makeVector(std::vector<Box> items);
    void push_back(const Box& value);
    Box& operator[](std::size_t index);
    const Box& operator[](std::size_t index) const;
    std::size_t size() const;
    const std::vector<Box>& items() const;

    // Statistics for the teaching material and benchmarks.
    static std::size_t bytesPerBox();

private:
    struct VectorData;

    void reset();
    void copyFrom(const Box& other);
    double numericAsDouble() const;
    std::int64_t numericAsInt64() const;

    Type type_ = Type::VOID;
    union {
        bool b;
        std::int8_t i8;
        std::uint8_t u8;
        std::int16_t i16;
        std::uint16_t u16;
        std::int32_t i32;
        std::uint32_t u32;
        std::int64_t i64;
        std::uint64_t u64;
        float f32;
        double f64;
        void* ptr;
    } u_{};
    std::shared_ptr<void> shared_;  // holds STRING and VECTOR payloads
};

}  // namespace nebula
