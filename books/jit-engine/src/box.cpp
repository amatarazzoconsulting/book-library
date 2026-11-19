// MIT License - (c) Anthony Matarazzo 2026
//
// box.cpp - implementation of the tagged value type.

#include "box.hpp"

#include <cmath>
#include <sstream>

namespace jitx {

Box::Box() : type_(Type::VOID) { u64 = 0; }
Box::Box(bool v) : type_(Type::BOOL) { b = v; }
Box::Box(int8_t v) : type_(Type::I8) { i8 = v; }
Box::Box(uint8_t v) : type_(Type::U8) { u8 = v; }
Box::Box(int16_t v) : type_(Type::I16) { i16 = v; }
Box::Box(uint16_t v) : type_(Type::U16) { u16 = v; }
Box::Box(int32_t v) : type_(Type::I32) { i32 = v; }
Box::Box(uint32_t v) : type_(Type::U32) { u32 = v; }
Box::Box(int64_t v) : type_(Type::I64) { i64 = v; }
Box::Box(uint64_t v) : type_(Type::U64) { u64 = v; }
Box::Box(float v) : type_(Type::F32) { f32 = v; }
Box::Box(double v) : type_(Type::F64) { f64 = v; }
Box::Box(const char* v)
    : type_(Type::STRING), str_(std::make_shared<std::string>(v ? v : "")) {
    u64 = 0;
}
Box::Box(const std::string& v)
    : type_(Type::STRING), str_(std::make_shared<std::string>(v)) {
    u64 = 0;
}

Box Box::makeVector() {
    Box b;
    b.type_ = Type::VECTOR;
    b.vec_ = std::make_shared<std::vector<Box>>();
    return b;
}

const char* Box::typeName() const { return typeName(type_); }

const char* Box::typeName(Type t) {
    switch (t) {
        case Type::VOID: return "void";
        case Type::BOOL: return "bool";
        case Type::I8: return "i8";
        case Type::U8: return "u8";
        case Type::I16: return "i16";
        case Type::U16: return "u16";
        case Type::I32: return "i32";
        case Type::U32: return "u32";
        case Type::I64: return "i64";
        case Type::U64: return "u64";
        case Type::F32: return "f32";
        case Type::F64: return "f64";
        case Type::STRING: return "string";
        case Type::VECTOR: return "vector";
    }
    return "void";
}

bool Box::isNumeric() const {
    switch (type_) {
        case Type::BOOL:
        case Type::I8: case Type::U8:
        case Type::I16: case Type::U16:
        case Type::I32: case Type::U32:
        case Type::I64: case Type::U64:
        case Type::F32: case Type::F64:
            return true;
        default:
            return false;
    }
}

bool Box::isInteger() const {
    switch (type_) {
        case Type::BOOL:
        case Type::I8: case Type::U8:
        case Type::I16: case Type::U16:
        case Type::I32: case Type::U32:
        case Type::I64: case Type::U64:
            return true;
        default:
            return false;
    }
}

bool Box::toBool() const {
    switch (type_) {
        case Type::BOOL: return b;
        case Type::I8: return i8 != 0;
        case Type::U8: return u8 != 0;
        case Type::I16: return i16 != 0;
        case Type::U16: return u16 != 0;
        case Type::I32: return i32 != 0;
        case Type::U32: return u32 != 0;
        case Type::I64: return i64 != 0;
        case Type::U64: return u64 != 0;
        case Type::F32: return f32 != 0.0f;
        case Type::F64: return f64 != 0.0;
        case Type::STRING: return str_ && !str_->empty();
        case Type::VECTOR: return vec_ && !vec_->empty();
        default: return false;
    }
}

int64_t Box::toInt64() const {
    switch (type_) {
        case Type::BOOL: return b ? 1 : 0;
        case Type::I8: return i8;
        case Type::U8: return u8;
        case Type::I16: return i16;
        case Type::U16: return u16;
        case Type::I32: return i32;
        case Type::U32: return u32;
        case Type::I64: return i64;
        case Type::U64: return static_cast<int64_t>(u64);
        case Type::F32: return static_cast<int64_t>(f32);
        case Type::F64: return static_cast<int64_t>(f64);
        case Type::STRING: return str_ ? static_cast<int64_t>(str_->size()) : 0;
        case Type::VECTOR: return static_cast<int64_t>(size());
        default: return 0;
    }
}

uint64_t Box::toUInt64() const {
    switch (type_) {
        case Type::U64: return u64;
        case Type::I64: return static_cast<uint64_t>(i64);
        default: return static_cast<uint64_t>(toInt64());
    }
}

double Box::toDouble() const {
    switch (type_) {
        case Type::BOOL: return b ? 1.0 : 0.0;
        case Type::I8: return i8;
        case Type::U8: return u8;
        case Type::I16: return i16;
        case Type::U16: return u16;
        case Type::I32: return i32;
        case Type::U32: return u32;
        case Type::I64: return static_cast<double>(i64);
        case Type::U64: return static_cast<double>(u64);
        case Type::F32: return static_cast<double>(f32);
        case Type::F64: return f64;
        default: return static_cast<double>(toInt64());
    }
}

std::string Box::toString() const {
    switch (type_) {
        case Type::VOID: return "";
        case Type::BOOL: return b ? "true" : "false";
        case Type::I8: return std::to_string(static_cast<int>(i8));
        case Type::U8: return std::to_string(static_cast<unsigned>(u8));
        case Type::I16: return std::to_string(i16);
        case Type::U16: return std::to_string(u16);
        case Type::I32: return std::to_string(i32);
        case Type::U32: return std::to_string(u32);
        case Type::I64: return std::to_string(i64);
        case Type::U64: return std::to_string(u64);
        case Type::F32: {
            std::ostringstream os;
            os << f32;
            return os.str();
        }
        case Type::F64: {
            std::ostringstream os;
            os << f64;
            return os.str();
        }
        case Type::STRING: return str_ ? *str_ : "";
        case Type::VECTOR: {
            std::ostringstream os;
            os << "<vector size=" << size() << ">";
            return os.str();
        }
    }
    return "";
}

// Arithmetic. Both integer operands keep the integer domain, so counters and
// bit tricks behave as a programmer expects; any float operand promotes.
Box Box::operator+(const Box& o) const {
    if (isInteger() && o.isInteger()) return Box(toInt64() + o.toInt64());
    return Box(toDouble() + o.toDouble());
}
Box Box::operator-(const Box& o) const {
    if (isInteger() && o.isInteger()) return Box(toInt64() - o.toInt64());
    return Box(toDouble() - o.toDouble());
}
Box Box::operator*(const Box& o) const {
    if (isInteger() && o.isInteger()) return Box(toInt64() * o.toInt64());
    return Box(toDouble() * o.toDouble());
}
Box Box::operator/(const Box& o) const {
    double d = o.toDouble();
    if (d == 0.0) return Box();  // division by zero yields VOID, never a trap
    return Box(toDouble() / d);
}
Box Box::operator%(const Box& o) const {
    int64_t d = o.toInt64();
    if (d == 0) return Box();
    return Box(toInt64() % d);
}

Box Box::operator&(const Box& o) const { return Box(toUInt64() & o.toUInt64()); }
Box Box::operator|(const Box& o) const { return Box(toUInt64() | o.toUInt64()); }
Box Box::operator^(const Box& o) const { return Box(toUInt64() ^ o.toUInt64()); }
Box Box::operator~() const { return Box(~toUInt64()); }
Box Box::operator<<(int s) const {
    if (s < 0 || s > 63) return Box(uint64_t{0});
    if (!isInteger()) return Box(std::ldexp(toDouble(), s));
    return Box(static_cast<int64_t>(toUInt64() << s));
}
Box Box::operator>>(int s) const {
    if (s < 0 || s > 63) return Box(uint64_t{0});
    if (!isInteger()) return Box(std::ldexp(toDouble(), -s));
    return Box(static_cast<int64_t>(toUInt64() >> s));
}

bool Box::operator==(const Box& o) const {
    if (type_ == Type::STRING && o.type_ == Type::STRING)
        return toString() == o.toString();
    if (isNumeric() && o.isNumeric()) {
        if (isInteger() && o.isInteger()) return toInt64() == o.toInt64();
        return toDouble() == o.toDouble();
    }
    if (type_ == Type::VOID && o.type_ == Type::VOID) return true;
    return false;
}
bool Box::operator!=(const Box& o) const { return !(*this == o); }

bool Box::operator<(const Box& o) const {
    if (type_ == Type::STRING && o.type_ == Type::STRING)
        return toString() < o.toString();
    if (isNumeric() && o.isNumeric()) return toDouble() < o.toDouble();
    return false;
}
bool Box::operator<=(const Box& o) const {
    if (type_ == Type::STRING && o.type_ == Type::STRING)
        return toString() <= o.toString();
    if (isNumeric() && o.isNumeric()) return toDouble() <= o.toDouble();
    return false;
}
bool Box::operator>(const Box& o) const {
    if (type_ == Type::STRING && o.type_ == Type::STRING)
        return toString() > o.toString();
    if (isNumeric() && o.isNumeric()) return toDouble() > o.toDouble();
    return false;
}
bool Box::operator>=(const Box& o) const {
    if (type_ == Type::STRING && o.type_ == Type::STRING)
        return toString() >= o.toString();
    if (isNumeric() && o.isNumeric()) return toDouble() >= o.toDouble();
    return false;
}

void Box::push_back(const Box& v) {
    if (type_ != Type::VECTOR) {
        type_ = Type::VECTOR;
        vec_ = std::make_shared<std::vector<Box>>();
        str_.reset();
    }
    vec_->push_back(v);
}

size_t Box::size() const {
    if (type_ == Type::VECTOR) return vec_ ? vec_->size() : 0;
    if (type_ == Type::STRING) return str_ ? str_->size() : 0;
    return 0;
}

bool Box::empty() const { return size() == 0; }

const Box& Box::at(size_t i) const {
    static const Box kNull;
    if (type_ != Type::VECTOR || !vec_ || i >= vec_->size()) return kNull;
    return (*vec_)[i];
}

} // namespace jitx
