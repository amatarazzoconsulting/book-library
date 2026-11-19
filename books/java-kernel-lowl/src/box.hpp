// MIT License - (c) Anthony Matarazzo 2026
//
// box.hpp - the LOWL "Box" value type.
//
// Box is the single tagged value that crosses the JNI boundary: every
// lowl.java.types.Box.Type constant (VOID .. OBJECT) maps onto one instance of
// this class.  Conversions are checked, arithmetic promotes like the Java
// wrapper classes do, and containers are reference types (copies alias).

#ifndef LOWL_BOX_HPP
#define LOWL_BOX_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace lowl {

// How a floating point value becomes an integer.
enum class RoundMode { TRUNCATE, FLOOR, CEIL, ROUND };

// Policy for every narrowing conversion.
struct ConvertOptions {
    RoundMode round = RoundMode::TRUNCATE;  // float -> integer rounding
    bool range_check = true;                // throw on out-of-range values
    bool parse_strings = true;              // accept "42" as an integer
};

class Box {
public:
    enum class Type : std::uint8_t {
        VOID, BOOL, INT8, UINT8, INT16, UINT16, INT32, UINT32,
        INT64, UINT64, FLOAT32, FLOAT64, STRING, ARRAY, MAP, OBJECT
    };

    using Array = std::vector<Box>;
    using Map = std::vector<std::pair<std::string, Box>>;

    Box();
    explicit Box(bool value);
    explicit Box(std::int8_t value);
    explicit Box(std::uint8_t value);
    explicit Box(std::int16_t value);
    explicit Box(std::uint16_t value);
    explicit Box(std::int32_t value);
    explicit Box(std::uint32_t value);
    explicit Box(std::int64_t value);
    explicit Box(std::uint64_t value);
    explicit Box(float value);
    explicit Box(double value);
    explicit Box(const char* value);
    explicit Box(std::string value);
    explicit Box(void* object);

    static Box make_array();
    static Box make_map();
    static Box make_object(void* object);

    Type type() const { return type_; }
    const char* type_name() const { return type_name(type_); }
    static const char* type_name(Type t);

    bool is_void() const { return type_ == Type::VOID; }
    bool is_numeric() const;
    bool is_integer() const;
    bool is_container() const { return type_ == Type::ARRAY || type_ == Type::MAP; }
    bool is_string() const { return type_ == Type::STRING; }

    // --- conversions -------------------------------------------------------
    bool as_bool() const;
    std::int64_t to_int64(const ConvertOptions& opt = {}) const;
    std::uint64_t to_uint64(const ConvertOptions& opt = {}) const;
    double to_double(const ConvertOptions& opt = {}) const;
    std::string to_string() const;                 // rendering, never throws
    const std::string& as_string() const;          // STRING only
    void* as_object() const;                       // OBJECT only

    // Narrowing helpers, all honouring ConvertOptions.
    std::int8_t as_int8(const ConvertOptions& opt = {}) const;
    std::uint8_t as_uint8(const ConvertOptions& opt = {}) const;
    std::int16_t as_int16(const ConvertOptions& opt = {}) const;
    std::uint16_t as_uint16(const ConvertOptions& opt = {}) const;
    std::int32_t as_int32(const ConvertOptions& opt = {}) const;
    std::uint32_t as_uint32(const ConvertOptions& opt = {}) const;
    float as_float32(const ConvertOptions& opt = {}) const;

    // --- arithmetic --------------------------------------------------------
    // Promotion: any float -> FLOAT64, else UINT64 if either side is UINT64,
    // else INT64.  Integer overflow and division by zero throw.
    Box add(const Box& other) const;
    Box subtract(const Box& other) const;
    Box multiply(const Box& other) const;
    Box divide(const Box& other) const;

    // --- containers --------------------------------------------------------
    void push_back(Box value);                     // ARRAY
    const Box& at(std::size_t index) const;        // ARRAY
    void put(const std::string& key, Box value);   // MAP (replaces)
    const Box& get(const std::string& key) const;  // MAP
    bool has(const std::string& key) const;        // MAP
    std::vector<std::string> keys() const;         // MAP
    std::size_t size() const;                      // ARRAY, MAP, STRING
    void clear();

    bool equals(const Box& other) const;
    bool operator==(const Box& other) const { return equals(other); }
    bool operator!=(const Box& other) const { return !equals(other); }

private:
    using Storage = std::variant<std::monostate, bool, std::int64_t, std::uint64_t,
                                 double, std::string, std::shared_ptr<Array>,
                                 std::shared_ptr<Map>, void*>;

    Type type_;
    Storage data_;

    std::int64_t signed_value() const;
    std::uint64_t unsigned_value() const;
    double double_value() const;
    Array& array_ref();
    const Array& array_ref() const;
    Map& map_ref();
    const Map& map_ref() const;
};

}  // namespace lowl

#endif  // LOWL_BOX_HPP
