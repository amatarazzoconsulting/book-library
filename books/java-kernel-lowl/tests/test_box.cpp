// MIT License - (c) Anthony Matarazzo 2026
//
// test_box.cpp - normal and edge cases for the LOWL Box value type.

#include "box.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>

using lowl::Box;
using lowl::ConvertOptions;
using lowl::RoundMode;

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
    ++checks;
    if (condition) {
        std::printf("PASS %s\n", what);
    } else {
        std::printf("FAIL %s\n", what);
        ++failures;
    }
}

template <typename Fn>
bool throws(Fn fn) {
    try {
        fn();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

void test_type_tags() {
    check(Box().type() == Box::Type::VOID, "default Box is VOID");
    check(Box(true).type() == Box::Type::BOOL, "bool Box is BOOL");
    check(Box(std::int8_t(-3)).type() == Box::Type::INT8, "int8 Box is INT8");
    check(Box(std::uint8_t(3)).type() == Box::Type::UINT8, "uint8 Box is UINT8");
    check(Box(std::int16_t(3)).type() == Box::Type::INT16, "int16 Box is INT16");
    check(Box(std::uint16_t(3)).type() == Box::Type::UINT16, "uint16 Box is UINT16");
    check(Box(std::int32_t(3)).type() == Box::Type::INT32, "int32 Box is INT32");
    check(Box(std::uint32_t(3)).type() == Box::Type::UINT32, "uint32 Box is UINT32");
    check(Box(std::int64_t(3)).type() == Box::Type::INT64, "int64 Box is INT64");
    check(Box(std::uint64_t(3)).type() == Box::Type::UINT64, "uint64 Box is UINT64");
    check(Box(1.5f).type() == Box::Type::FLOAT32, "float Box is FLOAT32");
    check(Box(1.5).type() == Box::Type::FLOAT64, "double Box is FLOAT64");
    check(Box("text").type() == Box::Type::STRING, "const char* Box is STRING");
    check(Box::make_array().type() == Box::Type::ARRAY, "make_array is ARRAY");
    check(Box::make_map().type() == Box::Type::MAP, "make_map is MAP");
    check(Box::make_object(nullptr).type() == Box::Type::OBJECT, "make_object is OBJECT");
    check(std::string(Box::type_name(Box::Type::FLOAT64)) == "FLOAT64", "type_name renders FLOAT64");
    check(Box(std::int32_t(1)).is_numeric(), "INT32 is numeric");
    check(Box(std::int32_t(1)).is_integer(), "INT32 is integer");
    check(!Box(1.0).is_integer(), "FLOAT64 is not integer");
    check(Box("x").is_string(), "STRING is string");
    check(Box::make_array().is_container(), "ARRAY is container");
}

void test_wide_conversions() {
    check(Box(std::int32_t(-42)).to_int64() == -42, "INT32 -> int64");
    check(Box(std::uint64_t(7)).to_uint64() == 7u, "UINT64 -> uint64");
    check(Box(2.75).to_double() == 2.75, "FLOAT64 -> double");
    check(Box(true).as_bool(), "BOOL -> bool");
    check(Box("42").to_int64() == 42, "numeric STRING parses to int64");
    check(Box("  -7  ").to_int64() == -7, "STRINGS are trimmed before parsing");
    check(Box("2.5").to_double() == 2.5, "numeric STRING parses to double");
    check(Box(std::uint64_t(18446744073709551615ull)).to_double() > 1.8e19, "UINT64 max -> double");
    check(Box(std::int64_t(-5)).to_uint64(ConvertOptions{RoundMode::TRUNCATE, false, true}) ==
              std::uint64_t(-5),
          "negative INT64 -> uint64 with range_check off wraps");
}

void test_narrow_conversions() {
    check(Box(std::int64_t(127)).as_int8() == 127, "127 fits int8");
    check(Box(std::int64_t(-128)).as_int8() == -128, "INT8 min fits");
    check(throws([] { (void)Box(std::int64_t(128)).as_int8(); }), "128 rejected by as_int8");
    check(throws([] { (void)Box(std::int64_t(256)).as_uint8(); }), "256 rejected by as_uint8");
    check(Box(std::int64_t(256)).as_uint8(ConvertOptions{RoundMode::TRUNCATE, false, true}) == 0,
          "256 wraps to 0 with range_check off");
    check(throws([] { (void)Box(std::int64_t(70000)).as_uint16(); }), "70000 rejected by as_uint16");
    check(throws([] { (void)Box(std::int64_t(1) << 40).as_int32(); }), "2^40 rejected by as_int32");
    check(Box(3.99).as_int32() == 3, "TRUNCATE drops the fraction");
    check(Box(-3.99).as_int32(ConvertOptions{RoundMode::FLOOR, true, true}) == -4, "FLOOR rounds down");
    check(Box(3.01).as_int32(ConvertOptions{RoundMode::CEIL, true, true}) == 4, "CEIL rounds up");
    check(Box(3.5).as_int32(ConvertOptions{RoundMode::ROUND, true, true}) == 4, "ROUND half away from zero");
    check(throws([] { (void)Box(1e300).as_int32(); }), "huge FLOAT64 rejected by as_int32");
    check(Box(1e300).as_int32(ConvertOptions{RoundMode::TRUNCATE, false, true}) ==
              std::numeric_limits<std::int32_t>::max(),
          "saturates to INT32 max with range_check off");
    check(Box(1.25).as_float32() == 1.25f, "FLOAT64 -> float32");
    check(Box(1e300).as_float32() > 3.4e38f, "float32 conversion saturates to infinity, never throws");
}

void test_string_and_errors() {
    check(Box("hello").as_string() == "hello", "as_string round trip");
    check(throws([] { (void)Box(std::int32_t(1)).as_string(); }), "as_string on INT32 throws");
    check(throws([] { (void)Box("abc").to_int64(); }), "non-numeric STRING throws");
    check(throws([] { (void)Box("").to_int64(); }), "empty STRING throws");
    check(throws([] { (void)Box("12abc").to_int64(); }), "trailing garbage throws");
    check(throws([] { (void)Box("x").to_int64(ConvertOptions{RoundMode::TRUNCATE, true, false}); }),
          "parse_strings=false rejects STRING");
    check(throws([] { (void)Box().to_double(); }), "VOID has no numeric value");
    check(throws([] { (void)Box("x").as_bool(); }), "as_bool on STRING throws");
    check(Box(1.5).to_string() == "1.5", "FLOAT64 renders 1.5");
    check(Box(0.1).to_string() == "0.1", "FLOAT64 renders shortest round-trip form");
    check(Box(std::int32_t(-9)).to_string() == "-9", "INT32 renders -9");
    check(Box(true).to_string() == "true", "BOOL renders true");
}

void test_arithmetic() {
    check(Box(std::int32_t(2)).add(Box(std::int32_t(3))).to_int64() == 5, "2 + 3 = 5");
    check(Box(std::int32_t(2)).add(Box(0.5)).to_double() == 2.5, "int + float promotes to FLOAT64");
    check(Box(std::int32_t(2)).add(Box(0.5)).type() == Box::Type::FLOAT64, "promotion type is FLOAT64");
    check(Box(std::uint64_t(4)).add(Box(std::uint64_t(5))).type() == Box::Type::UINT64,
          "UINT64 + UINT64 stays UINT64");
    check(Box(std::int32_t(9)).subtract(Box(std::int32_t(4))).to_int64() == 5, "9 - 4 = 5");
    check(Box(std::int32_t(3)).multiply(Box(std::int32_t(4))).to_int64() == 12, "3 * 4 = 12");
    check(Box(std::int32_t(9)).divide(Box(std::int32_t(4))).to_int64() == 2, "integer division truncates");
    check(Box(9.0).divide(Box(4.0)).to_double() == 2.25, "float division is exact");
    check(throws([] { (void)Box(std::int32_t(1)).divide(Box(std::int32_t(0))); }), "integer / 0 throws");
    check(throws([] { (void)Box(1.0).divide(Box(0.0)); }), "float / 0 throws");
    check(Box(std::int64_t(1)).add(Box(std::int64_t(-1))).to_int64() == 0, "1 + (-1) = 0");
    check(throws([] {
              (void)Box(std::int64_t(-1)).multiply(Box(std::numeric_limits<std::int64_t>::min()));
          }),
          "-1 * INT64 min throws instead of wrapping to an unsigned magnitude");
    check(throws([] {
              (void)Box(std::numeric_limits<std::int64_t>::max()).add(Box(std::int64_t(1)));
          }),
          "INT64 max + 1 throws");
    check(throws([] {
              (void)Box(std::numeric_limits<std::int64_t>::min()).subtract(Box(std::int64_t(1)));
          }),
          "INT64 min - 1 throws");
    check(throws([] {
              (void)Box(std::numeric_limits<std::int64_t>::max()).multiply(Box(std::int64_t(2)));
          }),
          "INT64 max * 2 throws");
    check(throws([] { (void)Box(std::int64_t(3)).subtract(Box(std::uint64_t(4))); }),
          "UINT64 underflow throws");
    check(throws([] { (void)Box(std::int32_t(1)).add(Box("x")); }), "arithmetic on STRING throws");
    check(Box(std::int32_t(3)).equals(Box(std::int64_t(3))) == false,
          "equal values of different types are not equal");
}

void test_containers() {
    Box array = Box::make_array();
    array.push_back(Box(std::int32_t(1)));
    array.push_back(Box("two"));
    check(array.size() == 2, "array size is 2");
    check(array.at(0).to_int64() == 1, "array[0] == 1");
    check(array.at(1).as_string() == "two", "array[1] == two");
    check(throws([&] { (void)array.at(2); }), "array out of bounds throws");

    Box alias = array;  // ARRAY is a reference type
    alias.push_back(Box(true));
    check(array.size() == 3, "copies of an ARRAY alias the same storage");
    check(array.to_string() == "[1, two, true]", "array renders inline");
    array.clear();
    check(array.size() == 0, "clear empties the array");

    Box map = Box::make_map();
    map.put("width", Box(std::int32_t(640)));
    map.put("title", Box("panel"));
    map.put("width", Box(std::int32_t(800)));
    check(map.size() == 2, "put replaces an existing key");
    check(map.get("width").to_int64() == 800, "replaced value is visible");
    check(map.has("title"), "has finds title");
    check(!map.has("height"), "has rejects missing key");
    check(throws([&] { (void)map.get("height"); }), "missing key throws");
    const std::vector<std::string> names = map.keys();
    check(names.size() == 2 && names[0] == "width" && names[1] == "title", "keys keep insertion order");

    Box nested = Box::make_array();
    Box inner = Box::make_map();
    inner.put("k", Box(1));
    nested.push_back(inner);
    check(nested.to_string() == "[{k: 1}]", "nested containers render");

    Box cycle = Box::make_array();
    cycle.push_back(cycle);
    check(cycle.to_string().find("...") != std::string::npos, "self-referential array renders with an ellipsis");

    check(Box("abcd").size() == 4, "STRING size is its length");
    check(throws([] { (void)Box(std::int32_t(1)).size(); }), "size on a scalar throws");
    check(Box::make_array().equals(Box::make_array()), "empty arrays are equal");
    check(!Box::make_array().equals(Box::make_map()), "array != map");
}

void test_object_and_void() {
    int marker = 0;
    Box object = Box::make_object(&marker);
    check(object.as_object() == &marker, "OBJECT keeps the pointer identity");
    check(object.is_numeric() == false, "OBJECT is not numeric");
    check(throws([&] { (void)object.to_int64(); }), "OBJECT has no numeric conversion");
    check(Box().to_string() == "void", "VOID renders as void");
    check(Box().equals(Box()), "two VOID values are equal");
}

}  // namespace

int main() {
    test_type_tags();
    test_wide_conversions();
    test_narrow_conversions();
    test_string_and_errors();
    test_arithmetic();
    test_containers();
    test_object_and_void();
    std::printf("---\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
