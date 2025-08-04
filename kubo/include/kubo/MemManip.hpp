#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <span>

inline int32_t bytes_2_int(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    return static_cast<int>(a << 24 | b << 16 | c << 8  | d << 0);
}

inline int32_t bytes_2_int(std::span<uint8_t> bytes)
{
    assert(bytes.size() == 4);
    return bytes_2_int(bytes[0], bytes[1], bytes[2], bytes[3]);
}

inline std::array<uint8_t, 4> int_2_bytes(int value)
{
    return {
        static_cast<uint8_t>((value >> 24) & 0xFF),
        static_cast<uint8_t>((value >> 16) & 0xFF),
        static_cast<uint8_t>((value >> 8)  & 0xFF),
        static_cast<uint8_t>((value >> 0)  & 0xFF),
    };
}
