// MIT License - (c) Anthony Matarazzo 2026
#include "box_value.hpp"

#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace nebula {

struct Box::VectorData {
    std::vector<Box> items;
};

namespace {

bool isIntegerTag(Box::Type t) {
    switch (t) {
        case Box::Type::I8: case Box::Type::U8:
        case Box::Type::I16: case Box::Type::U16:
        case Box::Type::I32: case Box::Type::U32:
        case Box::Type::I64: case Box::Type::U64:
        case Box::Type::BOOL:
            return true;
        default:
            return false;
    }
}

bool isFloatTag(Box::Type t) {
    return t == Box::Type::F32 || t == Box::Type::F64;
}

std::string formatDouble(double v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.17g", v);
    std::string s(buf);
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos &&
        s.find("inf") == std::string::npos && s.find("nan") == std::string::npos) {
        s += ".0";
    }
    return s;
}

}  // namespace

Box::Box() = default;

Box::Box(bool value) : type_(Type::BOOL) { u_.b = value; }
Box::Box(int value) : type_(Type::I32) { u_.i32 = value; }
Box::Box(long long value) : type_(Type::I64) { u_.i64 = static_cast<std::int64_t>(value); }
Box::Box(unsigned value) : type_(Type::U32) { u_.u32 = value; }
Box::Box(double value) : type_(Type::F64) { u_.f64 = value; }
Box::Box(float value) : type_(Type::F32) { u_.f32 = value; }

Box::Box(const char* value)
    : type_(value ? Type::STRING : Type::VOID),
      shared_(value ? std::make_shared<std::string>(value) : nullptr) {}

Box::Box(const std::string& value)
    : type_(Type::STRING), shared_(std::make_shared<std::string>(value)) {}

Box::Box(Type tag, std::int64_t raw) : type_(tag) {
    switch (tag) {
        case Type::BOOL: u_.b = raw != 0; break;
        case Type::I8: u_.i8 = static_cast<std::int8_t>(raw); break;
        case Type::U8: u_.u8 = static_cast<std::uint8_t>(raw); break;
        case Type::I16: u_.i16 = static_cast<std::int16_t>(raw); break;
        case Type::U16: u_.u16 = static_cast<std::uint16_t>(raw); break;
        case Type::I32: u_.i32 = static_cast<std::int32_t>(raw); break;
        case Type::U32: u_.u32 = static_cast<std::uint32_t>(raw); break;
        case Type::I64: u_.i64 = raw; break;
        case Type::U64: u_.u64 = static_cast<std::uint64_t>(raw); break;
        case Type::F32: u_.f32 = static_cast<float>(raw); break;
        case Type::F64: u_.f64 = static_cast<double>(raw); break;
        default: type_ = Type::VOID; break;
    }
}

Box::Box(const Box& other) { copyFrom(other); }

Box::Box(Box&& other) noexcept
    : type_(other.type_), u_(other.u_), shared_(std::move(other.shared_)) {
    other.type_ = Type::VOID;
    other.u_ = decltype(other.u_){};
}

Box& Box::operator=(const Box& other) {
    if (this != &other) {
        reset();
        copyFrom(other);
    }
    return *this;
}

Box& Box::operator=(Box&& other) noexcept {
    if (this != &other) {
        type_ = other.type_;
        u_ = other.u_;
        shared_ = std::move(other.shared_);
        other.type_ = Type::VOID;
        other.u_ = decltype(other.u_){};
    }
    return *this;
}

Box::~Box() = default;

void Box::reset() {
    type_ = Type::VOID;
    u_ = decltype(u_){};
    shared_.reset();
}

void Box::copyFrom(const Box& other) {
    type_ = other.type_;
    u_ = other.u_;
    shared_ = other.shared_;
}

const char* Box::typeName(Type t) {
    switch (t) {
        case Type::VOID: return "VOID";
        case Type::BOOL: return "BOOL";
        case Type::I8: return "I8";
        case Type::U8: return "U8";
        case Type::I16: return "I16";
        case Type::U16: return "U16";
        case Type::I32: return "I32";
        case Type::U32: return "U32";
        case Type::I64: return "I64";
        case Type::U64: return "U64";
        case Type::F32: return "F32";
        case Type::F64: return "F64";
        case Type::STRING: return "STRING";
        case Type::PTR: return "PTR";
        case Type::VECTOR: return "VECTOR";
        case Type::LIST: return "LIST";
        case Type::MAP: return "MAP";
        case Type::SET: return "SET";
        case Type::FUNCTION: return "FUNCTION";
        case Type::OBJECT: return "OBJECT";
        case Type::CLASS: return "CLASS";
        case Type::METHOD: return "METHOD";
        case Type::DATABASE: return "DATABASE";
        case Type::TABLE: return "TABLE";
        case Type::JIT_COMPILER: return "JIT_COMPILER";
        case Type::BUFFER: return "BUFFER";
    }
    return "UNKNOWN";
}

bool Box::isNumeric() const { return isIntegerTag(type_) || isFloatTag(type_); }
bool Box::isInteger() const { return isIntegerTag(type_); }

bool Box::asBool() const {
    switch (type_) {
        case Type::VOID: return false;
        case Type::BOOL: return u_.b;
        case Type::STRING: return !shared_ || !static_cast<std::string*>(shared_.get())->empty();
        case Type::VECTOR: return size() != 0;
        default: return numericAsDouble() != 0.0;
    }
}

std::int64_t Box::asInt64() const {
    if (type_ == Type::STRING) return std::stoll(asString());
    if (type_ == Type::VECTOR) return static_cast<std::int64_t>(size());
    return numericAsInt64();
}

std::uint64_t Box::asUint64() const {
    if (type_ == Type::STRING) return std::stoull(asString());
    if (type_ == Type::U64) return u_.u64;
    return static_cast<std::uint64_t>(asInt64());
}

double Box::asDouble() const {
    if (type_ == Type::STRING) return std::stod(asString());
    if (type_ == Type::VECTOR) return static_cast<double>(size());
    return numericAsDouble();
}

std::string Box::asString() const {
    switch (type_) {
        case Type::VOID: return "";
        case Type::STRING:
            return shared_ ? *static_cast<std::string*>(shared_.get()) : std::string();
        case Type::BOOL: return u_.b ? "true" : "false";
        case Type::I8: return std::to_string(static_cast<int>(u_.i8));
        case Type::U8: return std::to_string(static_cast<unsigned>(u_.u8));
        case Type::I16: return std::to_string(static_cast<int>(u_.i16));
        case Type::U16: return std::to_string(static_cast<unsigned>(u_.u16));
        case Type::I32: return std::to_string(u_.i32);
        case Type::U32: return std::to_string(u_.u32);
        case Type::I64: return std::to_string(u_.i64);
        case Type::U64: return std::to_string(u_.u64);
        case Type::F32: return formatDouble(static_cast<double>(u_.f32));
        case Type::F64: return formatDouble(u_.f64);
        case Type::PTR: {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "0x%llx",
                          static_cast<unsigned long long>(
                              reinterpret_cast<std::uintptr_t>(u_.ptr)));
            return std::string(buf);
        }
        case Type::VECTOR: {
            std::string out = "[";
            if (shared_) {
                const auto& v = static_cast<VectorData*>(shared_.get())->items;
                for (std::size_t i = 0; i < v.size(); ++i) {
                    if (i) out += ", ";
                    out += v[i].asString();
                }
            }
            out += "]";
            return out;
        }
        default:
            return std::string("<") + typeName(type_) + ">";
    }
}

std::string Box::asTypeString() const { return typeName(type_); }

bool Box::fitsIn(Type target) const {
    if (type_ == target) return true;
    if (!isInteger() || !isIntegerTag(target)) {
        if (isFloatTag(target) && isNumeric()) return true;
        return false;
    }
    if (target == Type::BOOL) return true;
    if (type_ == Type::BOOL) return true;
    const std::int64_t v = numericAsInt64();
    switch (target) {
        case Type::I8: return v >= INT8_MIN && v <= INT8_MAX;
        case Type::U8: return v >= 0 && v <= UINT8_MAX;
        case Type::I16: return v >= INT16_MIN && v <= INT16_MAX;
        case Type::U16: return v >= 0 && v <= UINT16_MAX;
        case Type::I32: return v >= INT32_MIN && v <= INT32_MAX;
        case Type::U32: return v >= 0 && v <= static_cast<std::int64_t>(UINT32_MAX);
        case Type::I64: return true;
        case Type::U64: return v >= 0;
        default: return false;
    }
}

double Box::numericAsDouble() const {
    switch (type_) {
        case Type::BOOL: return u_.b ? 1.0 : 0.0;
        case Type::I8: return u_.i8;
        case Type::U8: return u_.u8;
        case Type::I16: return u_.i16;
        case Type::U16: return u_.u16;
        case Type::I32: return u_.i32;
        case Type::U32: return u_.u32;
        case Type::I64: return static_cast<double>(u_.i64);
        case Type::U64: return static_cast<double>(u_.u64);
        case Type::F32: return u_.f32;
        case Type::F64: return u_.f64;
        default: return 0.0;
    }
}

std::int64_t Box::numericAsInt64() const {
    switch (type_) {
        case Type::BOOL: return u_.b ? 1 : 0;
        case Type::I8: return u_.i8;
        case Type::U8: return u_.u8;
        case Type::I16: return u_.i16;
        case Type::U16: return u_.u16;
        case Type::I32: return u_.i32;
        case Type::U32: return u_.u32;
        case Type::I64: return u_.i64;
        case Type::U64: return static_cast<std::int64_t>(u_.u64);
        case Type::F32: return static_cast<std::int64_t>(u_.f32);
        case Type::F64: return static_cast<std::int64_t>(u_.f64);
        default: return 0;
    }
}

Box Box::operator+(const Box& other) const {
    if (type_ == Type::STRING || other.type_ == Type::STRING)
        return Box(asString() + other.asString());
    if (type_ == Type::VECTOR && other.type_ == Type::VECTOR) {
        Box out = makeVector();
        for (std::size_t i = 0; i < size(); ++i) out.push_back((*this)[i]);
        for (std::size_t i = 0; i < other.size(); ++i) out.push_back(other[i]);
        return out;
    }
    if (isFloatTag(type_) || isFloatTag(other.type_))
        return Box(numericAsDouble() + other.numericAsDouble());
    return Box(static_cast<long long>(numericAsInt64() + other.numericAsInt64()));
}

Box Box::operator-(const Box& other) const {
    if (isFloatTag(type_) || isFloatTag(other.type_))
        return Box(numericAsDouble() - other.numericAsDouble());
    return Box(static_cast<long long>(numericAsInt64() - other.numericAsInt64()));
}

Box Box::operator*(const Box& other) const {
    if (isFloatTag(type_) || isFloatTag(other.type_))
        return Box(numericAsDouble() * other.numericAsDouble());
    return Box(static_cast<long long>(numericAsInt64() * other.numericAsInt64()));
}

Box Box::operator/(const Box& other) const {
    if (isFloatTag(type_) || isFloatTag(other.type_)) {
        const double d = other.numericAsDouble();
        if (d == 0.0) throw std::domain_error("Box: division by zero");
        return Box(numericAsDouble() / d);
    }
    const std::int64_t d = other.numericAsInt64();
    if (d == 0) throw std::domain_error("Box: integer division by zero");
    return Box(static_cast<long long>(numericAsInt64() / d));
}

Box Box::operator%(const Box& other) const {
    const std::int64_t d = other.numericAsInt64();
    if (d == 0) throw std::domain_error("Box: modulo by zero");
    return Box(static_cast<long long>(numericAsInt64() % d));
}

Box Box::operator&(const Box& other) const {
    return Box(static_cast<long long>(numericAsInt64() & other.numericAsInt64()));
}
Box Box::operator|(const Box& other) const {
    return Box(static_cast<long long>(numericAsInt64() | other.numericAsInt64()));
}
Box Box::operator^(const Box& other) const {
    return Box(static_cast<long long>(numericAsInt64() ^ other.numericAsInt64()));
}
Box Box::operator~() const {
    return Box(static_cast<long long>(~numericAsInt64()));
}
Box Box::operator-() const {
    if (isFloatTag(type_)) return Box(-numericAsDouble());
    return Box(static_cast<long long>(-numericAsInt64()));
}

bool Box::operator==(const Box& other) const {
    if (isNumeric() && other.isNumeric()) {
        if (isFloatTag(type_) || isFloatTag(other.type_))
            return numericAsDouble() == other.numericAsDouble();
        return numericAsInt64() == other.numericAsInt64();
    }
    if (type_ != other.type_) return false;
    if (type_ == Type::VECTOR) {
        if (size() != other.size()) return false;
        for (std::size_t i = 0; i < size(); ++i)
            if (!((*this)[i] == other[i])) return false;
        return true;
    }
    return asString() == other.asString();
}

bool Box::operator<(const Box& other) const {
    if (isNumeric() && other.isNumeric()) return numericAsDouble() < other.numericAsDouble();
    return asString() < other.asString();
}

Box Box::makeVector() {
    Box b;
    b.type_ = Type::VECTOR;
    b.shared_ = std::make_shared<VectorData>();
    return b;
}

Box Box::makeVector(std::vector<Box> items) {
    Box b = makeVector();
    static_cast<VectorData*>(b.shared_.get())->items = std::move(items);
    return b;
}

void Box::push_back(const Box& value) {
    if (type_ != Type::VECTOR) throw std::logic_error("Box: not a VECTOR");
    static_cast<VectorData*>(shared_.get())->items.push_back(value);
}

Box& Box::operator[](std::size_t index) {
    if (type_ != Type::VECTOR) throw std::logic_error("Box: not a VECTOR");
    return static_cast<VectorData*>(shared_.get())->items.at(index);
}

const Box& Box::operator[](std::size_t index) const {
    if (type_ != Type::VECTOR) throw std::logic_error("Box: not a VECTOR");
    return static_cast<VectorData*>(shared_.get())->items.at(index);
}

std::size_t Box::size() const {
    if (type_ != Type::VECTOR || !shared_) return 0;
    return static_cast<VectorData*>(shared_.get())->items.size();
}

const std::vector<Box>& Box::items() const {
    static const std::vector<Box> empty;
    if (type_ != Type::VECTOR || !shared_) return empty;
    return static_cast<VectorData*>(shared_.get())->items;
}

std::size_t Box::bytesPerBox() { return sizeof(Box); }

}  // namespace nebula
