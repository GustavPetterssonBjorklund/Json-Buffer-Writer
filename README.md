# Json Buffer Writer

Minimal streaming JSON writer for embedded systems.  
No heap allocation, proper string escaping, and support for nested objects/arrays.

## Features

- No heap allocation — works entirely on a caller-provided buffer  
- Proper JSON string escaping  
- Supports nested objects/arrays (up to `MAX_DEPTH`)  
- Works on Arduino / ESP32 / embedded platforms  
- Incremental writing without copying

---

## Installation

[View on PlatformIO Registry](https://registry.platformio.org/libraries/gustavpettersson/Json%20Buffer%20Writer)
