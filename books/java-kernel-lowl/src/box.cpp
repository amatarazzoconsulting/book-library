// MIT License - (c) Anthony Matarazzo 2026
//
// box.cpp - implementation of the LOWL tagged value type.

#include "box.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace lowl {
namespace {

[[noreturn]] void fail(const std::string& message) { throw std::runtime_error(message); }

std::string trim(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) --end;
    return text.substr(begin, end - begin);
}

// Shortest decimal form that parses back to the same value.
std::string format_double(double value) {
    if (!std::isfinite(value)) return std::isnan(value) ? "nan" : (value < 0 ? "-inf" : "inf");
    char buffer[64];
    for (int precision = 6; precision <= 17; ++precision) {
        std::snprintf(buffer, sizeof buffer, "%.*g", precision, value);
        if (std::strtod(buffer, nullptr) == value) return std::string(buffer);
    }
    std::snprintf(buffer, sizeof buffer, "%.17g", value);
    return std::string(buffer);
}

double apply_round(double value, RoundMode mode) {
    switch (mode) {
        case RoundMode::TRUNCATE: return std::trunc(value);
        case RoundMode::FLOOR: return std::floor(value);
        case RoundMode::CEIL: return std::ceil(value);
        case RoundMode::ROUND: return std::round(value);
    }
    return value;
}

constexpr int kRenderDepth = 8;

std::string render(const Box& box, int depth);

std::string render(const Box& box, int depth) {
    switch (box.type()) {
        case Box::Type::VOID: return "void";
        case Box::Type::BOOL: return box.as_bool() ? "true" : "false";
        case Box::Type::STRING: return box.as_string();
        case Box::Type::ARRAY: {
            if (depth >= kRenderDepth) return "[...]";
            std::string out = "[";
            for (std::size_t i = 0; i < box.size(); ++i) {
                if (i != 0) out += ", ";
                out += render(box.at(i), depth + 1);
            }
            return out + "]";
        }
        case Box::Type::MAP: {
            if (depth >= kRenderDepth) return "{...}";
            std::string out = "{";
            std::vector<std::string> names = box.keys();
            for (std::size_t i = 0; i < names.size(); ++i) {
                if (i != 0) out += ", ";
                out += names[i];
                out += ": ";
                out += render(box.get(names[i]), depth + 1);
            }
            return out + "}";
        }
        case Box::Type::OBJECT: {
            char buffer[32];
            std::snprintf(buffer, sizeof buffer, "object@%p", box.as_object());
            return std::string(buffer);
        }
        default: break;
    }
    return box.to_string();
}

}  // namespace

Box::Box() : type_(Type::VOID), data_(std::monostate{}) {}

Box::Box(bool value) : type_(Type::BOOL), data_(value) {}
Box::Box(std::int8_t value) : type_(Type::INT8), data_(static_cast<std::int64_t>(value)) {}
Box::Box(std::uint8_t value) : type_(Type::UINT8), data_(static_cast<std::uint64_t>(value)) {}
Box::Box(std::int16_t value) : type_(Type::INT16), data_(static_cast<std::int64_t>(value)) {}
Box::Box(std::uint16_t value) : type_(Type::UINT16), data_(static_cast<std::uint64_t>(value)) {}
Box::Box(std::int32_t value) : type_(Type::INT32), data_(static_cast<std::int64_t>(value)) {}
Box::Box(std::uint32_t value) : type_(Type::UINT32), data_(static_cast<std::uint64_t>(value)) {}
Box::Box(std::int64_t value) : type_(Type::INT64), data_(value) {}
Box::Box(std::uint64_t value) : type_(Type::UINT64), data_(value) {}
Box::Box(float value) : type_(Type::FLOAT32), data_(static_cast<double>(value)) {}
Box::Box(double value) : type_(Type::FLOAT64), data_(value) {}
Box::Box(const char* value) : type_(Type::STRING), data_(std::string(value ? value : "")) {}
Box::Box(std::string value) : type_(Type::STRING), data_(std::move(value)) {}
Box::Box(void* object) : type_(Type::OBJECT), data_(object) {}

Box Box::make_array() {
    Box box;
    box.type_ = Type::ARRAY;
    box.data_ = std::make_shared<Array>();
    return box;
}

Box Box::make_map() {
    Box box;
    box.type_ = Type::MAP;
    box.data_ = std::make_shared<Map>();
    return box;
}

Box Box::make_object(void* object) { return Box(object); }

const char* Box::type_name(Type t) {
    switch (t) {
        case Type::VOID: return "VOID";
        case Type::BOOL: return "BOOL";
        case Type::INT8: return "INT8";
        case Type::UINT8: return "UINT8";
        case Type::INT16: return "INT16";
        case Type::UINT16: return "UINT16";
        case Type::INT32: return "INT32";
        case Type::UINT32: return "UINT32";
        case Type::INT64: return "INT64";
        case Type::UINT64: return "UINT64";
        case Type::FLOAT32: return "FLOAT32";
        case Type::FLOAT64: return "FLOAT64";
        case Type::STRING: return "STRING";
        case Type::ARRAY: return "ARRAY";
        case Type::MAP: return "MAP";
        case Type::OBJECT: return "OBJECT";
    }
    return "VOID";
}

bool Box::is_numeric() const {
    switch (type_) {
        case Type::INT8: case Type::UINT8: case Type::INT16: case Type::UINT16:
        case Type::INT32: case Type::UINT32: case Type::INT64: case Type::UINT64:
        case Type::FLOAT32: case Type::FLOAT64: return true;
        default: return false;
    }
}

bool Box::is_integer() const {
    return is_numeric() && type_ != Type::FLOAT32 && type_ != Type::FLOAT64;
}

bool Box::as_bool() const {
    if (type_ != Type::BOOL) fail(std::string("as_bool on ") + type_name());
    return std::get<bool>(data_);
}

std::int64_t Box::signed_value() const { return std::get<std::int64_t>(data_); }
std::uint64_t Box::unsigned_value() const { return std::get<std::uint64_t>(data_); }
double Box::double_value() const { return std::get<double>(data_); }

Box::Array& Box::array_ref() { return *std::get<std::shared_ptr<Array>>(data_); }
const Box::Array& Box::array_ref() const { return *std::get<std::shared_ptr<Array>>(data_); }
Box::Map& Box::map_ref() { return *std::get<std::shared_ptr<Map>>(data_); }
const Box::Map& Box::map_ref() const { return *std::get<std::shared_ptr<Map>>(data_); }

double Box::to_double(const ConvertOptions& opt) const {
    switch (type_) {
        case Type::INT8: case Type::INT16: case Type::INT32: case Type::INT64:
            return static_cast<double>(signed_value());
        case Type::UINT8: case Type::UINT16: case Type::UINT32: case Type::UINT64:
            return static_cast<double>(unsigned_value());
        case Type::FLOAT32: case Type::FLOAT64: return double_value();
        case Type::STRING: {
            if (!opt.parse_strings) fail("to_double on STRING with parse_strings disabled");
            const std::string text = trim(as_string());
            if (text.empty()) fail("to_double on empty STRING");
            errno = 0;
            char* end = nullptr;
            const double value = std::strtod(text.c_str(), &end);
            if (end == nullptr || *end != '\0' || errno == ERANGE) fail("to_double on non-numeric STRING '" + text + "'");
            return value;
        }
        default: fail(std::string("to_double on ") + type_name());
    }
    return 0.0;
}

std::int64_t Box::to_int64(const ConvertOptions& opt) const {
    switch (type_) {
        case Type::INT8: case Type::INT16: case Type::INT32: case Type::INT64:
            return signed_value();
        case Type::UINT8: case Type::UINT16: case Type::UINT32: case Type::UINT64: {
            const std::uint64_t raw = unsigned_value();
            if (raw > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                if (opt.range_check) fail("to_int64: UINT64 value out of INT64 range");
                return static_cast<std::int64_t>(raw);
            }
            return static_cast<std::int64_t>(raw);
        }
        case Type::FLOAT32: case Type::FLOAT64: {
            const double raw = double_value();
            if (!std::isfinite(raw)) fail("to_int64: non-finite FLOAT");
            const double rounded = apply_round(raw, opt.round);
            const double low = static_cast<double>(std::numeric_limits<std::int64_t>::min());
            const double high = 9223372036854775808.0;  // 2^63, exclusive
            if (rounded < low || rounded >= high) {
                if (opt.range_check) fail("to_int64: FLOAT out of INT64 range");
                return rounded < low ? std::numeric_limits<std::int64_t>::min()
                                     : std::numeric_limits<std::int64_t>::max();
            }
            return static_cast<std::int64_t>(rounded);
        }
        case Type::STRING: {
            if (!opt.parse_strings) fail("to_int64 on STRING with parse_strings disabled");
            const std::string text = trim(as_string());
            if (text.empty()) fail("to_int64 on empty STRING");
            errno = 0;
            char* end = nullptr;
            const long long value = std::strtoll(text.c_str(), &end, 10);
            if (end == nullptr || *end != '\0' || errno == ERANGE) fail("to_int64 on non-integer STRING '" + text + "'");
            return static_cast<std::int64_t>(value);
        }
        default: fail(std::string("to_int64 on ") + type_name());
    }
    return 0;
}

std::uint64_t Box::to_uint64(const ConvertOptions& opt) const {
    switch (type_) {
        case Type::UINT8: case Type::UINT16: case Type::UINT32: case Type::UINT64:
            return unsigned_value();
        case Type::INT8: case Type::INT16: case Type::INT32: case Type::INT64: {
            const std::int64_t raw = signed_value();
            if (raw < 0) {
                if (opt.range_check) fail("to_uint64: negative INT64 value");
                return static_cast<std::uint64_t>(raw);
            }
            return static_cast<std::uint64_t>(raw);
        }
        case Type::FLOAT32: case Type::FLOAT64: {
            const double raw = double_value();
            if (!std::isfinite(raw)) fail("to_uint64: non-finite FLOAT");
            const double rounded = apply_round(raw, opt.round);
            const double high = 18446744073709551616.0;  // 2^64, exclusive
            if (rounded < 0.0 || rounded >= high) {
                if (opt.range_check) fail("to_uint64: FLOAT out of UINT64 range");
                return rounded < 0.0 ? 0u : std::numeric_limits<std::uint64_t>::max();
            }
            return static_cast<std::uint64_t>(rounded);
        }
        case Type::STRING: {
            if (!opt.parse_strings) fail("to_uint64 on STRING with parse_strings disabled");
            const std::string text = trim(as_string());
            if (text.empty()) fail("to_uint64 on empty STRING");
            if (text[0] == '-') fail("to_uint64 on negative STRING '" + text + "'");
            errno = 0;
            char* end = nullptr;
            const unsigned long long value = std::strtoull(text.c_str(), &end, 10);
            if (end == nullptr || *end != '\0' || errno == ERANGE) fail("to_uint64 on non-integer STRING '" + text + "'");
            return static_cast<std::uint64_t>(value);
        }
        default: fail(std::string("to_uint64 on ") + type_name());
    }
    return 0;
}

std::string Box::to_string() const {
    switch (type_) {
        case Type::VOID: return "void";
        case Type::BOOL: return as_bool() ? "true" : "false";
        case Type::STRING: return as_string();
        case Type::ARRAY: case Type::MAP: case Type::OBJECT: return render(*this, 0);
        case Type::FLOAT32: case Type::FLOAT64: return format_double(double_value());
        default: break;
    }
    if (is_integer()) {
        if (type_ == Type::INT8 || type_ == Type::INT16 || type_ == Type::INT32 || type_ == Type::INT64) {
            return std::to_string(signed_value());
        }
        return std::to_string(unsigned_value());
    }
    return "void";
}

const std::string& Box::as_string() const {
    if (type_ != Type::STRING) fail(std::string("as_string on ") + type_name());
    return std::get<std::string>(data_);
}

void* Box::as_object() const {
    if (type_ != Type::OBJECT) fail(std::string("as_object on ") + type_name());
    return std::get<void*>(data_);
}

namespace {
std::int64_t narrow_signed(const Box& box, const ConvertOptions& opt, std::int64_t low, std::int64_t high,
                           const char* label) {
    const std::int64_t value = box.to_int64(opt);
    if (value >= low && value <= high) return value;
    if (opt.range_check) fail(std::string(label) + ": value out of range");
    // Java semantics, JLS 5.1.3: a floating point source saturates at the
    // target bound, an integer source wraps through the caller's static_cast.
    if (!box.is_integer()) return value < low ? low : high;
    return value;
}
}  // namespace

std::int8_t Box::as_int8(const ConvertOptions& opt) const {
    return static_cast<std::int8_t>(narrow_signed(*this, opt, -128, 127, "as_int8"));
}

std::uint8_t Box::as_uint8(const ConvertOptions& opt) const {
    const std::uint64_t value = to_uint64(opt);
    if (opt.range_check && value > 255u) fail("as_uint8: value out of range");
    return static_cast<std::uint8_t>(value);
}

std::int16_t Box::as_int16(const ConvertOptions& opt) const {
    return static_cast<std::int16_t>(narrow_signed(*this, opt, -32768, 32767, "as_int16"));
}

std::uint16_t Box::as_uint16(const ConvertOptions& opt) const {
    const std::uint64_t value = to_uint64(opt);
    if (opt.range_check && value > 65535u) fail("as_uint16: value out of range");
    return static_cast<std::uint16_t>(value);
}

std::int32_t Box::as_int32(const ConvertOptions& opt) const {
    return static_cast<std::int32_t>(
        narrow_signed(*this, opt, std::numeric_limits<std::int32_t>::min(),
                      std::numeric_limits<std::int32_t>::max(), "as_int32"));
}

std::uint32_t Box::as_uint32(const ConvertOptions& opt) const {
    const std::uint64_t value = to_uint64(opt);
    if (opt.range_check && value > std::numeric_limits<std::uint32_t>::max()) fail("as_uint32: value out of range");
    return static_cast<std::uint32_t>(value);
}

float Box::as_float32(const ConvertOptions& opt) const { return static_cast<float>(to_double(opt)); }

// --- arithmetic -------------------------------------------------------------

namespace {

Box make_signed(std::int64_t value) { return Box(value); }
Box make_unsigned(std::uint64_t value) { return Box(value); }
Box make_double(double value) { return Box(value); }

}  // namespace

Box Box::add(const Box& other) const {
    if (!is_numeric() || !other.is_numeric()) fail("add requires two numeric Box values");
    if (type_ == Type::FLOAT32 || type_ == Type::FLOAT64 || other.type_ == Type::FLOAT32 ||
        other.type_ == Type::FLOAT64) {
        return make_double(to_double() + other.to_double());
    }
    if (type_ == Type::UINT64 || other.type_ == Type::UINT64) {
        const std::uint64_t a = to_uint64();
        const std::uint64_t b = other.to_uint64();
        if (a > std::numeric_limits<std::uint64_t>::max() - b) fail("add: UINT64 overflow");
        return make_unsigned(a + b);
    }
    const std::int64_t a = to_int64();
    const std::int64_t b = other.to_int64();
    if ((b > 0 && a > std::numeric_limits<std::int64_t>::max() - b) ||
        (b < 0 && a < std::numeric_limits<std::int64_t>::min() - b)) {
        fail("add: INT64 overflow");
    }
    return make_signed(a + b);
}

Box Box::subtract(const Box& other) const {
    if (!is_numeric() || !other.is_numeric()) fail("subtract requires two numeric Box values");
    if (type_ == Type::FLOAT32 || type_ == Type::FLOAT64 || other.type_ == Type::FLOAT32 ||
        other.type_ == Type::FLOAT64) {
        return make_double(to_double() - other.to_double());
    }
    if (type_ == Type::UINT64 || other.type_ == Type::UINT64) {
        const std::uint64_t a = to_uint64();
        const std::uint64_t b = other.to_uint64();
        if (b > a) fail("subtract: UINT64 underflow");
        return make_unsigned(a - b);
    }
    const std::int64_t a = to_int64();
    const std::int64_t b = other.to_int64();
    if ((b < 0 && a > std::numeric_limits<std::int64_t>::max() + b) ||
        (b > 0 && a < std::numeric_limits<std::int64_t>::min() + b)) {
        fail("subtract: INT64 overflow");
    }
    return make_signed(a - b);
}

Box Box::multiply(const Box& other) const {
    if (!is_numeric() || !other.is_numeric()) fail("multiply requires two numeric Box values");
    if (type_ == Type::FLOAT32 || type_ == Type::FLOAT64 || other.type_ == Type::FLOAT32 ||
        other.type_ == Type::FLOAT64) {
        return make_double(to_double() * other.to_double());
    }
    if (type_ == Type::UINT64 || other.type_ == Type::UINT64) {
        const std::uint64_t a = to_uint64();
        const std::uint64_t b = other.to_uint64();
        if (a != 0 && b > std::numeric_limits<std::uint64_t>::max() / a) fail("multiply: UINT64 overflow");
        return make_unsigned(a * b);
    }
    const std::int64_t a = to_int64();
    const std::int64_t b = other.to_int64();
    if (a != 0 && b != 0) {
        const std::int64_t max = std::numeric_limits<std::int64_t>::max();
        const std::int64_t min = std::numeric_limits<std::int64_t>::min();
        const bool overflow = (a == -1 && b == min) || (b == -1 && a == min) ||
                              (a > 0 && b > 0 && a > max / b) ||
                              (a > 0 && b < 0 && b < min / a) ||
                              (a < 0 && b > 0 && a < min / b) ||
                              (a < 0 && b < 0 && a < max / b);
        if (overflow) fail("multiply: INT64 overflow");
    }
    return make_signed(a * b);
}

Box Box::divide(const Box& other) const {
    if (!is_numeric() || !other.is_numeric()) fail("divide requires two numeric Box values");
    if (type_ == Type::FLOAT32 || type_ == Type::FLOAT64 || other.type_ == Type::FLOAT32 ||
        other.type_ == Type::FLOAT64) {
        const double divisor = other.to_double();
        if (divisor == 0.0) fail("divide: floating point division by zero");
        return make_double(to_double() / divisor);
    }
    if (type_ == Type::UINT64 || other.type_ == Type::UINT64) {
        const std::uint64_t divisor = other.to_uint64();
        if (divisor == 0) fail("divide: integer division by zero");
        return make_unsigned(to_uint64() / divisor);
    }
    const std::int64_t divisor = other.to_int64();
    if (divisor == 0) fail("divide: integer division by zero");
    const std::int64_t dividend = to_int64();
    if (dividend == std::numeric_limits<std::int64_t>::min() && divisor == -1) fail("divide: INT64 overflow");
    return make_signed(dividend / divisor);
}

// --- containers -------------------------------------------------------------

void Box::push_back(Box value) {
    if (type_ != Type::ARRAY) fail(std::string("push_back on ") + type_name());
    array_ref().push_back(std::move(value));
}

const Box& Box::at(std::size_t index) const {
    if (type_ != Type::ARRAY) fail(std::string("at on ") + type_name());
    const Array& array = array_ref();
    if (index >= array.size()) fail("at: index " + std::to_string(index) + " out of bounds");
    return array[index];
}

void Box::put(const std::string& key, Box value) {
    if (type_ != Type::MAP) fail(std::string("put on ") + type_name());
    Map& map = map_ref();
    for (auto& entry : map) {
        if (entry.first == key) {
            entry.second = std::move(value);
            return;
        }
    }
    map.emplace_back(key, std::move(value));
}

const Box& Box::get(const std::string& key) const {
    if (type_ != Type::MAP) fail(std::string("get on ") + type_name());
    for (const auto& entry : map_ref()) {
        if (entry.first == key) return entry.second;
    }
    fail("get: missing key '" + key + "'");
}

bool Box::has(const std::string& key) const {
    if (type_ != Type::MAP) fail(std::string("has on ") + type_name());
    for (const auto& entry : map_ref()) {
        if (entry.first == key) return true;
    }
    return false;
}

std::vector<std::string> Box::keys() const {
    if (type_ != Type::MAP) fail(std::string("keys on ") + type_name());
    std::vector<std::string> names;
    names.reserve(map_ref().size());
    for (const auto& entry : map_ref()) names.push_back(entry.first);
    return names;
}

std::size_t Box::size() const {
    switch (type_) {
        case Type::ARRAY: return array_ref().size();
        case Type::MAP: return map_ref().size();
        case Type::STRING: return as_string().size();
        default: fail(std::string("size on ") + type_name());
    }
    return 0;
}

void Box::clear() {
    switch (type_) {
        case Type::ARRAY: array_ref().clear(); return;
        case Type::MAP: map_ref().clear(); return;
        case Type::STRING: std::get<std::string>(data_).clear(); return;
        default: fail(std::string("clear on ") + type_name());
    }
}

bool Box::equals(const Box& other) const {
    if (type_ != other.type_) return false;
    switch (type_) {
        case Type::VOID: return true;
        case Type::BOOL: return as_bool() == other.as_bool();
        case Type::INT8: case Type::INT16: case Type::INT32: case Type::INT64:
            return signed_value() == other.signed_value();
        case Type::UINT8: case Type::UINT16: case Type::UINT32: case Type::UINT64:
            return unsigned_value() == other.unsigned_value();
        case Type::FLOAT32: case Type::FLOAT64: return double_value() == other.double_value();
        case Type::STRING: return as_string() == other.as_string();
        case Type::OBJECT: return as_object() == other.as_object();
        case Type::ARRAY: {
            if (array_ref().size() != other.array_ref().size()) return false;
            for (std::size_t i = 0; i < array_ref().size(); ++i) {
                if (!array_ref()[i].equals(other.array_ref()[i])) return false;
            }
            return true;
        }
        case Type::MAP: {
            if (map_ref().size() != other.map_ref().size()) return false;
            for (const auto& entry : map_ref()) {
                if (!other.has(entry.first)) return false;
                if (!entry.second.equals(other.get(entry.first))) return false;
            }
            return true;
        }
    }
    return false;
}

}  // namespace lowl
