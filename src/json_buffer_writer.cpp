#include "json_buffer_writer.hpp"

#include <stdio.h>
#include <cstdarg>

// Implementation

JsonBufWriter::JsonBufWriter(uint8_t *buf, size_t capacity)
    : buffer_(buf), capacity_(capacity), length_(0), hasError_(false),
      depth_(0), floatPrecision_(DEFAULT_FLOAT_PRECISION), expectValue_(false)
{
}

void JsonBufWriter::reset(uint8_t *buf, size_t capacity)
{
    buffer_ = buf;
    capacity_ = capacity;
    length_ = 0;
    hasError_ = false;
    depth_ = 0;
    expectValue_ = false;
    floatPrecision_ = DEFAULT_FLOAT_PRECISION;
}

void JsonBufWriter::setFloatPrecision(uint8_t digits)
{
    floatPrecision_ = digits;
}

bool JsonBufWriter::beginObject()
{
    return openContainer('{', true);
}

bool JsonBufWriter::beginArray()
{
    return openContainer('[', false);
}

bool JsonBufWriter::endObject()
{
    return closeContainer('}', true);
}

bool JsonBufWriter::endArray()
{
    return closeContainer(']', false);
}

bool JsonBufWriter::key(const char *key)
{
    if (hasError_ || !inObject())
    {
        return setError();
    }

    Frame &frame = currentFrame();

    // Add comma before key if this isn't the first key-value pair
    if (!frame.isFirst && !appendChar(','))
    {
        return false;
    }
    frame.isFirst = false;

    if (!writeString(key))
    {
        return false;
    }

    if (!appendChar(':'))
    {
        return false;
    }

    frame.expectValue = true;
    return true;
}

bool JsonBufWriter::value(const char *str)
{
    if (!addCommaIfNeeded())
    {
        return false;
    }
    return writeString(str);
}

bool JsonBufWriter::value(const char *str, size_t length)
{
    if (!addCommaIfNeeded())
    {
        return false;
    }
    return writeStringWithLength(str, length);
}

bool JsonBufWriter::value(bool boolean)
{
    if (!addCommaIfNeeded())
    {
        return false;
    }
    if (!writeRawData(boolean ? "true" : "false", boolean ? 4u : 5u))
    {
        return false;
    }
    updateStateAfterValue();
    return true;
}

bool JsonBufWriter::value(int32_t integer)
{
    return writeNumber("%ld", static_cast<long>(integer));
}

bool JsonBufWriter::value(uint32_t integer)
{
    return writeNumber("%lu", static_cast<unsigned long>(integer));
}

bool JsonBufWriter::value(int64_t integer)
{
    return writeNumber("%lld", static_cast<long long>(integer));
}

bool JsonBufWriter::value(uint64_t integer)
{
    return writeNumber("%llu", static_cast<unsigned long long>(integer));
}

bool JsonBufWriter::value(float number)
{
    return writeFloat(static_cast<double>(number));
}

bool JsonBufWriter::value(double number)
{
    return writeFloat(number);
}

bool JsonBufWriter::null()
{
    if (!addCommaIfNeeded())
    {
        return false;
    }
    if (!writeRawData("null", 4))
    {
        return false;
    }
    updateStateAfterValue();
    return true;
}

bool JsonBufWriter::raw(const char *json, size_t length)
{
    if (!addCommaIfNeeded())
    {
        return false;
    }
    if (!writeRawData(json, length))
    {
        return false;
    }
    updateStateAfterValue();
    return true;
}

bool JsonBufWriter::finalize(const uint8_t *&output, size_t &length)
{
    if (hasError_ || depth_ != 0)
    {
        return false;
    }

    output = buffer_;
    length = length_;
    return true;
}

bool JsonBufWriter::ok() const
{
    return !hasError_;
}

size_t JsonBufWriter::size() const
{
    return length_;
}

bool JsonBufWriter::inAnyContainer() const
{
    return depth_ > 0;
}

bool JsonBufWriter::inObject() const
{
    return inAnyContainer() && stack_[depth_ - 1].isObject;
}

JsonBufWriter::Frame &JsonBufWriter::currentFrame()
{
    return stack_[depth_ - 1];
}

bool JsonBufWriter::openContainer(char openChar, bool isObject)
{
    if (hasError_ || (depth_ == 0 && length_ != 0))
    {
        return setError(); // Only allow single root
    }

    if (!addCommaIfNeeded())
    {
        return false;
    }

    if (!appendChar(openChar))
    {
        return false;
    }

    if (depth_ >= MAX_DEPTH)
    {
        return setError();
    }

    stack_[depth_++] = Frame{isObject, true, false};
    expectValue_ = false; // Root flag only
    return true;
}

bool JsonBufWriter::closeContainer(char closeChar, bool isObject)
{
    if (hasError_ || !inAnyContainer() || currentFrame().isObject != isObject)
    {
        return setError();
    }

    if (!appendChar(closeChar))
    {
        return false;
    }

    depth_--;

    // After closing, the parent no longer expects a value
    if (inAnyContainer())
    {
        currentFrame().expectValue = false;
    }
    else
    {
        expectValue_ = false;
    }

    return true;
}

bool JsonBufWriter::addCommaIfNeeded()
{
    if (hasError_)
    {
        return false;
    }

    if (inAnyContainer())
    {
        Frame &frame = currentFrame();

        if (frame.isObject)
        {
            // In object: only add comma if we're not expecting a value (i.e., we just finished a key-value pair)
            if (!frame.expectValue)
            {
                if (!frame.isFirst && !appendChar(','))
                {
                    return false;
                }
                frame.isFirst = false;
            }
        }
        else
        {
            // In array: always add comma before elements (except the first)
            if (!frame.isFirst && !appendChar(','))
            {
                return false;
            }
            frame.isFirst = false;
        }
    }
    else
    {
        // Root: allow only a single value
        if (length_ != 0)
        {
            return setError();
        }
    }

    return true;
}

bool JsonBufWriter::writeString(const char *str)
{
    if (!appendChar('"'))
    {
        return false;
    }

    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(str); *p; ++p)
    {
        if (!escapeCharacter(*p))
        {
            return false;
        }
    }

    if (!appendChar('"'))
    {
        return false;
    }

    updateStateAfterValue();
    return true;
}

bool JsonBufWriter::writeStringWithLength(const char *str, size_t length)
{
    if (!appendChar('"'))
    {
        return false;
    }

    const unsigned char *p = reinterpret_cast<const unsigned char *>(str);
    for (size_t i = 0; i < length; ++i)
    {
        if (!escapeCharacter(p[i]))
        {
            return false;
        }
    }

    if (!appendChar('"'))
    {
        return false;
    }

    updateStateAfterValue();
    return true;
}

bool JsonBufWriter::writeNumber(const char *format, ...)
{
    if (!addCommaIfNeeded())
    {
        return false;
    }

    va_list args;
    va_start(args, format);
    bool success = formatWithVArgs(format, args);
    va_end(args);

    updateStateAfterValue();
    return success;
}

bool JsonBufWriter::writeFloat(double value)
{
    if (!addCommaIfNeeded())
    {
        return false;
    }

    // Avoid locale issues; snprintf with precision
    char format[8];
    snprintf(format, sizeof(format), "%%.%df", floatPrecision_);

    int result = formatFloat(format, value);
    if (result < 0)
    {
        return setError();
    }

    updateStateAfterValue();
    return true;
}

bool JsonBufWriter::writeRawData(const char *data, size_t length)
{
    if (hasError_ || !ensureCapacity(length))
    {
        return setError();
    }

    for (size_t i = 0; i < length; ++i)
    {
        buffer_[length_++] = static_cast<uint8_t>(data[i]);
    }

    return true;
}

bool JsonBufWriter::appendChar(char character)
{
    if (hasError_ || !ensureCapacity(1))
    {
        return setError();
    }

    buffer_[length_++] = static_cast<uint8_t>(character);
    return true;
}

bool JsonBufWriter::escapeCharacter(unsigned char c)
{
    switch (c)
    {
    case '"':
        return appendString("\\\"", 2);
    case '\\':
        return appendString("\\\\", 2);
    case '\b':
        return appendString("\\b", 2);
    case '\f':
        return appendString("\\f", 2);
    case '\n':
        return appendString("\\n", 2);
    case '\r':
        return appendString("\\r", 2);
    case '\t':
        return appendString("\\t", 2);
    default:
        if (c < 0x20)
        {
            // Control character -> \u00XX
            char unicode[6] = {'\\', 'u', '0', '0', 0, 0};
            static const char *hexDigits = "0123456789abcdef";
            unicode[4] = hexDigits[(c >> 4) & 0xF];
            unicode[5] = hexDigits[c & 0xF];
            return appendString(unicode, 6);
        }
        return appendChar(static_cast<char>(c));
    }
}

bool JsonBufWriter::appendString(const char *str, size_t length)
{
    if (hasError_ || !ensureCapacity(length))
    {
        return setError();
    }

    for (size_t i = 0; i < length; ++i)
    {
        buffer_[length_++] = static_cast<uint8_t>(str[i]);
    }

    return true;
}

bool JsonBufWriter::ensureCapacity(size_t additionalBytes) const
{
    return length_ + additionalBytes <= capacity_;
}

bool JsonBufWriter::setError()
{
    hasError_ = true;
    return false;
}

int JsonBufWriter::formatFloat(const char *format, double value)
{
    if (!buffer_)
    {
        return -1;
    }

    size_t remaining = capacity_ > length_ ? capacity_ - length_ : 0;
    int result = snprintf(reinterpret_cast<char *>(buffer_ + length_), remaining, format, value);

    if (result <= 0)
    {
        return -1;
    }

    if (!ensureCapacity(static_cast<size_t>(result)))
    {
        hasError_ = true;
        return -1;
    }

    length_ += static_cast<size_t>(result);
    return result;
}

bool JsonBufWriter::formatWithVArgs(const char *format, va_list args)
{
    if (!buffer_)
    {
        return setError();
    }

    size_t remaining = capacity_ > length_ ? capacity_ - length_ : 0;
    int result = vsnprintf(reinterpret_cast<char *>(buffer_ + length_), remaining, format, args);

    if (result < 0)
    {
        return setError();
    }

    if (!ensureCapacity(static_cast<size_t>(result)))
    {
        return setError();
    }

    length_ += static_cast<size_t>(result);
    return true;
}

void JsonBufWriter::updateStateAfterValue()
{
    if (inAnyContainer())
    {
        Frame &frame = currentFrame();
        if (frame.isObject)
        {
            frame.expectValue = false; // After a value in object, we're done with this key-value pair
        }
        // For arrays, we don't change expectValue (it stays false)
    }
    else
    {
        expectValue_ = false;
    }
}

void JsonBufWriter::updateStateAfterValueIfArrayOrRoot()
{
    if (inAnyContainer())
    {
        if (!currentFrame().isObject)
        {
            currentFrame().expectValue = false; // Array element done
        }
    }
    else
    {
        expectValue_ = false;
    }
}
