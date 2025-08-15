#include <unity.h>
#include <Arduino.h>
#include "../../src/json_buffer_writer.hpp"

// Test buffer size
constexpr size_t BUFFER_SIZE = 512;
static uint8_t testBuffer[BUFFER_SIZE];

void setUp(void)
{
    // Called before each test
    memset(testBuffer, 0, BUFFER_SIZE);
}

void tearDown(void)
{
    // Called after each test
}

// Helper function to get JSON string from writer
String getJsonString(JsonBufWriter &writer)
{
    const uint8_t *output;
    size_t length;
    if (writer.finalize(output, length))
    {
        return String(reinterpret_cast<const char *>(output), length);
    }
    return "";
}

// Basic functionality tests
void test_empty_object()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginObject());
    TEST_ASSERT_TRUE(writer.endObject());

    String result = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("{}", result.c_str());
}

void test_empty_array()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.endArray());

    String result = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("[]", result.c_str());
}

void test_simple_object()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginObject());
    TEST_ASSERT_TRUE(writer.key("name"));
    TEST_ASSERT_TRUE(writer.value("John"));
    TEST_ASSERT_TRUE(writer.key("age"));
    TEST_ASSERT_TRUE(writer.value(30));
    TEST_ASSERT_TRUE(writer.endObject());

    String result = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("{\"name\":\"John\",\"age\":30}", result.c_str());
}

void test_simple_array()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.value(1));
    TEST_ASSERT_TRUE(writer.value(2));
    TEST_ASSERT_TRUE(writer.value(3));
    TEST_ASSERT_TRUE(writer.endArray());

    String result = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("[1,2,3]", result.c_str());
}

// Data type tests
void test_string_values()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.value("hello"));
    TEST_ASSERT_TRUE(writer.value("world"));
    TEST_ASSERT_TRUE(writer.value("", 0)); // Empty string
    TEST_ASSERT_TRUE(writer.endArray());

    String result = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("[\"hello\",\"world\",\"\"]", result.c_str());
}

void test_boolean_values()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.value(true));
    TEST_ASSERT_TRUE(writer.value(false));
    TEST_ASSERT_TRUE(writer.endArray());

    String result = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("[true,false]", result.c_str());
}

void test_integer_values()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.value(static_cast<int32_t>(-123)));
    TEST_ASSERT_TRUE(writer.value(static_cast<uint32_t>(456)));
    TEST_ASSERT_TRUE(writer.value(static_cast<int64_t>(-789123456789LL)));
    TEST_ASSERT_TRUE(writer.value(static_cast<uint64_t>(987654321098ULL)));
    TEST_ASSERT_TRUE(writer.endArray());

    String result = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("[-123,456,-789123456789,987654321098]", result.c_str());
}

void test_float_values()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);
    writer.setFloatPrecision(2);

    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.value(3.14f));
    TEST_ASSERT_TRUE(writer.value(2.718));
    TEST_ASSERT_TRUE(writer.endArray());

    String result = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("[3.14,2.72]", result.c_str());
}

void test_null_values()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.null());
    TEST_ASSERT_TRUE(writer.value("not null"));
    TEST_ASSERT_TRUE(writer.null());
    TEST_ASSERT_TRUE(writer.endArray());

    String result = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("[null,\"not null\",null]", result.c_str());
}

// String escaping tests
void test_string_escaping()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginObject());
    TEST_ASSERT_TRUE(writer.key("quotes"));
    TEST_ASSERT_TRUE(writer.value("He said \"Hello\""));
    TEST_ASSERT_TRUE(writer.key("backslash"));
    TEST_ASSERT_TRUE(writer.value("C:\\path\\file.txt"));
    TEST_ASSERT_TRUE(writer.key("newline"));
    TEST_ASSERT_TRUE(writer.value("line1\nline2"));
    TEST_ASSERT_TRUE(writer.key("tab"));
    TEST_ASSERT_TRUE(writer.value("col1\tcol2"));
    TEST_ASSERT_TRUE(writer.endObject());

    String result = getJsonString(writer);
    const char *expected = "{\"quotes\":\"He said \\\"Hello\\\"\","
                           "\"backslash\":\"C:\\\\path\\\\file.txt\","
                           "\"newline\":\"line1\\nline2\","
                           "\"tab\":\"col1\\tcol2\"}";
    TEST_ASSERT_EQUAL_STRING(expected, result.c_str());
}

void test_control_character_escaping()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.value("\x01\x1F")); // Control characters
    TEST_ASSERT_TRUE(writer.endArray());

    String result = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("[\"\\u0001\\u001f\"]", result.c_str());
}

// Nested structure tests
void test_nested_objects()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginObject());
    TEST_ASSERT_TRUE(writer.key("person"));
    TEST_ASSERT_TRUE(writer.beginObject());
    TEST_ASSERT_TRUE(writer.key("name"));
    TEST_ASSERT_TRUE(writer.value("Alice"));
    TEST_ASSERT_TRUE(writer.key("address"));
    TEST_ASSERT_TRUE(writer.beginObject());
    TEST_ASSERT_TRUE(writer.key("street"));
    TEST_ASSERT_TRUE(writer.value("123 Main St"));
    TEST_ASSERT_TRUE(writer.endObject());
    TEST_ASSERT_TRUE(writer.endObject());
    TEST_ASSERT_TRUE(writer.endObject());

    String result = getJsonString(writer);
    const char *expected = "{\"person\":{\"name\":\"Alice\",\"address\":{\"street\":\"123 Main St\"}}}";
    TEST_ASSERT_EQUAL_STRING(expected, result.c_str());
}

void test_nested_arrays()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.value(1));
    TEST_ASSERT_TRUE(writer.value(2));
    TEST_ASSERT_TRUE(writer.endArray());
    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.value(3));
    TEST_ASSERT_TRUE(writer.value(4));
    TEST_ASSERT_TRUE(writer.endArray());
    TEST_ASSERT_TRUE(writer.endArray());

    String result = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("[[1,2],[3,4]]", result.c_str());
}

void test_mixed_nested_structures()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginObject());
    TEST_ASSERT_TRUE(writer.key("users"));
    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.beginObject());
    TEST_ASSERT_TRUE(writer.key("id"));
    TEST_ASSERT_TRUE(writer.value(1));
    TEST_ASSERT_TRUE(writer.key("tags"));
    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.value("admin"));
    TEST_ASSERT_TRUE(writer.value("active"));
    TEST_ASSERT_TRUE(writer.endArray());
    TEST_ASSERT_TRUE(writer.endObject());
    TEST_ASSERT_TRUE(writer.endArray());
    TEST_ASSERT_TRUE(writer.endObject());

    String result = getJsonString(writer);
    const char *expected = "{\"users\":[{\"id\":1,\"tags\":[\"admin\",\"active\"]}]}";
    TEST_ASSERT_EQUAL_STRING(expected, result.c_str());
}

// Raw JSON tests
void test_raw_json()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginObject());
    TEST_ASSERT_TRUE(writer.key("custom"));
    TEST_ASSERT_TRUE(writer.raw("{\"raw\":true}", 12));
    TEST_ASSERT_TRUE(writer.key("normal"));
    TEST_ASSERT_TRUE(writer.value("value"));
    TEST_ASSERT_TRUE(writer.endObject());

    String result = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("{\"custom\":{\"raw\":true},\"normal\":\"value\"}", result.c_str());
}

// Edge cases and error handling
void test_buffer_overflow()
{
    uint8_t smallBuffer[20]; // Increased size slightly
    JsonBufWriter writer(smallBuffer, 20);

    TEST_ASSERT_TRUE(writer.beginObject());
    TEST_ASSERT_TRUE(writer.key("key"));
    // This should fail due to buffer overflow
    TEST_ASSERT_FALSE(writer.value("very long string that exceeds buffer capacity"));
    TEST_ASSERT_FALSE(writer.ok());
}

void test_invalid_structure()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    // Try to end object without starting one
    TEST_ASSERT_FALSE(writer.endObject());
    TEST_ASSERT_FALSE(writer.ok());
}

void test_mismatched_containers()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginObject());
    // Try to end with array instead of object
    TEST_ASSERT_FALSE(writer.endArray());
    TEST_ASSERT_FALSE(writer.ok());
}

void test_key_without_object()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginArray());
    // Try to add key inside array
    TEST_ASSERT_FALSE(writer.key("invalid"));
    TEST_ASSERT_FALSE(writer.ok());
}

void test_multiple_root_values()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.value("first"));
    // Second root value should fail
    TEST_ASSERT_FALSE(writer.value("second"));
    TEST_ASSERT_FALSE(writer.ok());
}

void test_max_depth()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    // Create nested structure up to max depth
    for (int i = 0; i < JsonBufWriter::MAX_DEPTH; i++)
    {
        TEST_ASSERT_TRUE(writer.beginObject());
        TEST_ASSERT_TRUE(writer.key("level"));
    }

    // This should fail (exceeds max depth)
    TEST_ASSERT_FALSE(writer.beginObject());
    TEST_ASSERT_FALSE(writer.ok());
}

// Reset and reuse tests
void test_reset_functionality()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    // First use
    TEST_ASSERT_TRUE(writer.beginObject());
    TEST_ASSERT_TRUE(writer.key("test"));
    TEST_ASSERT_TRUE(writer.value(123));
    TEST_ASSERT_TRUE(writer.endObject());

    String result1 = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("{\"test\":123}", result1.c_str());

    // Reset and reuse
    writer.reset(testBuffer, BUFFER_SIZE);
    TEST_ASSERT_TRUE(writer.ok());
    TEST_ASSERT_EQUAL_UINT32(0, writer.size());

    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.value("new"));
    TEST_ASSERT_TRUE(writer.endArray());

    String result2 = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("[\"new\"]", result2.c_str());
}

void test_float_precision_setting()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    // Test with default precision
    writer.setFloatPrecision(1);
    TEST_ASSERT_TRUE(writer.beginArray());
    TEST_ASSERT_TRUE(writer.value(3.14159));
    TEST_ASSERT_TRUE(writer.endArray());

    String result = getJsonString(writer);
    TEST_ASSERT_EQUAL_STRING("[3.1]", result.c_str());
}

// Performance/stress test
void test_large_object()
{
    JsonBufWriter writer(testBuffer, BUFFER_SIZE);

    TEST_ASSERT_TRUE(writer.beginObject());

    // Add many key-value pairs
    for (int i = 0; i < 10; i++)
    {
        char key[16];
        sprintf(key, "key%d", i);
        TEST_ASSERT_TRUE(writer.key(key));
        TEST_ASSERT_TRUE(writer.value(i * 100));
    }

    TEST_ASSERT_TRUE(writer.endObject());
    TEST_ASSERT_TRUE(writer.ok());

    String result = getJsonString(writer);
    TEST_ASSERT_TRUE(result.length() > 0);
    TEST_ASSERT_TRUE(result.startsWith("{"));
    TEST_ASSERT_TRUE(result.endsWith("}"));
}

// Test runner setup
void setup()
{
    delay(2000); // Wait for serial monitor

    UNITY_BEGIN();

    // Basic functionality
    RUN_TEST(test_empty_object);
    RUN_TEST(test_empty_array);
    RUN_TEST(test_simple_object);
    RUN_TEST(test_simple_array);

    // Data types
    RUN_TEST(test_string_values);
    RUN_TEST(test_boolean_values);
    RUN_TEST(test_integer_values);
    RUN_TEST(test_float_values);
    RUN_TEST(test_null_values);

    // String escaping
    RUN_TEST(test_string_escaping);
    RUN_TEST(test_control_character_escaping);

    // Nested structures
    RUN_TEST(test_nested_objects);
    RUN_TEST(test_nested_arrays);
    RUN_TEST(test_mixed_nested_structures);

    // Raw JSON
    RUN_TEST(test_raw_json);

    // Error handling
    RUN_TEST(test_buffer_overflow);
    RUN_TEST(test_invalid_structure);
    RUN_TEST(test_mismatched_containers);
    RUN_TEST(test_key_without_object);
    RUN_TEST(test_multiple_root_values);
    RUN_TEST(test_max_depth);

    // Reset and configuration
    RUN_TEST(test_reset_functionality);
    RUN_TEST(test_float_precision_setting);

    // Stress test
    RUN_TEST(test_large_object);

    UNITY_END();
}

void loop()
{
    // Unity test framework handles everything in setup()
}

// Additional test for platformio.ini
/*
[env:test]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    throwtheswitch/Unity@^2.5.2
test_filter = test_*
build_flags = -DUNITY_INCLUDE_CONFIG_H
*/