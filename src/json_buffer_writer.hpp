#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <cstdarg>

/**
 * @file
 * @brief Streaming JSON writer for a fixed, caller-provided buffer.
 *
 * @details
 * `JsonBufWriter` produces well-formed JSON incrementally into a byte buffer supplied
 * by the caller. It targets embedded/resource-constrained systems:
 * - No heap allocation, no DOM.
 * - Works on Arduino/MCU platforms.
 * - Proper JSON string escaping.
 * - Supports nested objects/arrays (up to #MAX_DEPTH).
 * - Incremental writes with bounds checking.
 *
 * @note The writer does **not** append a trailing `'\0'` terminator. Use the returned
 *       `length` from finalize() and treat the buffer as a byte span.
 *
 * ### Example
 * @code{.cpp}
 * uint8_t buf[256];
 * JsonBufWriter jw(buf, sizeof(buf));
 *
 * jw.beginObject();
 *   jw.key("id");     jw.value(123u);
 *   jw.key("name");   jw.value("motor");
 *   jw.key("values"); jw.beginArray();
 *     jw.value(1.0f);
 *     jw.value(2.5f);
 *   jw.endArray();
 * jw.endObject();
 *
 * const uint8_t* out; size_t len;
 * if (jw.finalize(out, len)) {
 *   // send out[0..len-1] over UART, MQTT, etc.
 * }
 * @endcode
 */

/**
 * @class JsonBufWriter
 * @brief Minimal streaming JSON writer into a caller-provided buffer.
 *
 * The class enforces container correctness (objects vs arrays), comma insertion,
 * and escaping for strings. When an error occurs (capacity exceeded, invalid
 * state transition, etc.), the writer enters a permanent error state until reset.
 *
 * @warning `raw()` inserts text without validation—caller must ensure the fragment
 *          is valid JSON and fits in the remaining capacity.
 * @note Methods return `false` on failure and set an internal error flag; check
 *       #ok() or the return value of each call.
 */
class JsonBufWriter
{
public:
    /** @brief Maximum supported container nesting depth. */
    static constexpr size_t MAX_DEPTH = 8;

    /** @brief Default number of decimal places for floating point values. */
    static constexpr uint8_t DEFAULT_FLOAT_PRECISION = 3;

    /**
     * @brief Construct a JSON writer bound to a buffer.
     * @param buf Pointer to the output buffer (must remain valid for the writer’s lifetime or until reset()).
     * @param capacity Number of bytes available in @p buf.
     * @post #size() == 0 and #ok() == true.
     */
    explicit JsonBufWriter(uint8_t *buf, size_t capacity);

    // ----------------------------
    // Configuration
    // ----------------------------

    /**
     * @brief Reset the writer to start writing into a (possibly new) buffer.
     * @param buf Pointer to the buffer to use from now on.
     * @param capacity Capacity in bytes of @p buf.
     * @post Clears error state, depth, and counters; float precision set to #DEFAULT_FLOAT_PRECISION.
     */
    void reset(uint8_t *buf, size_t capacity);

    /**
     * @brief Set precision for floating-point values.
     * @param digits Digits after the decimal point (implementation clamps to a reasonable range).
     */
    void setFloatPrecision(uint8_t digits);

    // ----------------------------
    // Container operations
    // ----------------------------

    /**
     * @brief Begin a new JSON object `{ ... }`.
     * @retval true Success.
     * @retval false Error (e.g., depth exceeded, invalid state, or capacity).
     * @pre Either at root, inside an array, or following a key() in an object.
     */
    bool beginObject();

    /**
     * @brief Begin a new JSON array `[ ... ]`.
     * @retval true Success.
     * @retval false Error (e.g., depth exceeded, invalid state, or capacity).
     * @pre Either at root, inside an array, or following a key() in an object.
     */
    bool beginArray();

    /**
     * @brief Close the current JSON object.
     * @retval true Success.
     * @retval false Error (e.g., mismatched close or no open object).
     */
    bool endObject();

    /**
     * @brief Close the current JSON array.
     * @retval true Success.
     * @retval false Error (e.g., mismatched close or no open array).
     */
    bool endArray();

    // ----------------------------
    // Object keys and values
    // ----------------------------

    /**
     * @brief Write an object key (a JSON string followed by a colon).
     * @param key Null-terminated UTF-8 key string.
     * @retval true Success.
     * @retval false Error (e.g., not inside an object, capacity exceeded).
     * @pre Currently inside an object and not waiting for a value.
     * @post The writer expects a subsequent value() or container begin call.
     */
    bool key(const char *key);

    /**
     * @name Value writers
     * @brief Emit a JSON value at the current position.
     * @details All `value(...)` overloads:
     *  - Insert commas automatically when needed.
     *  - Enforce the object/array/root state machine.
     *  - Return `false` and set the error flag on failure.
     * @retval true on success, @retval false on error.
     * @{
     */

    /** @brief Write a string value (null-terminated). */
    bool value(const char *str);

    /**
     * @overload
     * @brief Write a string value with explicit length.
     * @param str Pointer to string bytes (need not be null-terminated).
     * @param length Number of bytes from @p str to write.
     */
    bool value(const char *str, size_t length);

    /** @overload @brief Write a boolean value (`true`/`false`). */
    bool value(bool boolean);

    /** @overload @brief Write a 32-bit signed integer. */
    bool value(int32_t integer);

    /** @overload @brief Write a 32-bit unsigned integer. */
    bool value(uint32_t integer);

    /** @overload @brief Write a 64-bit signed integer. */
    bool value(int64_t integer);

    /** @overload @brief Write a 64-bit unsigned integer. */
    bool value(uint64_t integer);

    /** @overload @brief Write a single-precision floating-point number. */
    bool value(float number);

    /** @overload @brief Write a double-precision floating-point number. */
    bool value(double number);
    /** @} */

    /**
     * @brief Write a JSON `null`.
     * @retval true Success.
     * @retval false Error (invalid state or capacity).
     */
    bool null();

    /**
     * @brief Insert a raw JSON fragment verbatim (no validation or escaping).
     * @param json Pointer to a fragment (UTF-8).
     * @param length Number of bytes to copy.
     * @retval true Success.
     * @retval false Error (invalid state or capacity).
     * @warning No syntax checks or escaping—intended for advanced use only.
     */
    bool raw(const char *json, size_t length);

    // ----------------------------
    // Finalization
    // ----------------------------

    /**
     * @brief Finalize the document and expose the buffer span.
     * @param[out] output Receives pointer to the start of the buffer.
     * @param[out] length Receives the number of bytes written.
     * @retval true Success (all containers closed, writer valid).
     * @retval false Error (e.g., unclosed containers or prior error).
     * @post #ok() remains unchanged; you may continue writing only if it returns false (no).
     * @note The buffer is owned by the caller; this call does not allocate or copy.
     */
    bool finalize(const uint8_t *&output, size_t &length);

    // ----------------------------
    // Query
    // ----------------------------

    /**
     * @brief Check whether the writer is currently error-free.
     * @return `true` if no error has occurred since construction/reset, else `false`.
     */
    bool ok() const;

    /**
     * @brief Bytes written so far.
     * @return Current size in bytes (0 if nothing written).
     */
    size_t size() const;

private:
    /** @brief Container frame state for nesting. */
    struct Frame
    {
        bool isObject;    ///< True if this frame is an object.
        bool isFirst;     ///< True if writing the first element in the container.
        bool expectValue; ///< True if a value is expected (after a key in an object).
    };

    // Buffer pointers and counters
    uint8_t *buffer_; ///< Output buffer.
    size_t capacity_; ///< Total capacity of the buffer.
    size_t length_;   ///< Current write position.
    bool hasError_;   ///< Error flag.

    // State tracking
    uint8_t depth_;          ///< Current nesting depth.
    uint8_t floatPrecision_; ///< Decimal digits for float/double serialization.
    bool expectValue_;       ///< Root-level value expectation flag.
    Frame stack_[MAX_DEPTH]; ///< Stack of active container frames.

    // The following helpers are internal implementation details.
    /// @cond INTERNAL
    // State queries
    bool inAnyContainer() const;
    bool inObject() const;
    Frame &currentFrame();

    // Internal container operations
    bool openContainer(char openChar, bool isObject);
    bool closeContainer(char closeChar, bool isObject);

    // Output helpers
    bool addCommaIfNeeded();
    bool writeString(const char *str);
    bool writeStringWithLength(const char *str, size_t length);
    bool writeNumber(const char *format, ...);
    bool writeFloat(double value);
    bool writeRawData(const char *data, size_t length);
    bool appendChar(char character);
    bool appendString(const char *str, size_t length);
    bool escapeCharacter(unsigned char character);

    // Buffer checks
    bool ensureCapacity(size_t additionalBytes) const;
    bool setError();

    // Format helpers
    int formatFloat(const char *format, double value);
    bool formatWithVArgs(const char *format, va_list args);

    // State updates after writing values
    void updateStateAfterValue();
    void updateStateAfterValueIfArrayOrRoot();
    /// @endcond
};
