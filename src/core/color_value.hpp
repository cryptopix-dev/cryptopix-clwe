#ifndef COLOR_VALUE_HPP
#define COLOR_VALUE_HPP

#include <cstdint>
#include <iostream>

namespace clwe {

struct ColorValue {
    uint8_t r, g, b, a;

    ColorValue() : r(0), g(0), b(0), a(255) {}
    ColorValue(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}

    uint32_t to_math_value() const {
        return (static_cast<uint32_t>(r) << 24) |
               (static_cast<uint32_t>(g) << 16) |
               (static_cast<uint32_t>(b) << 8) |
               static_cast<uint32_t>(a);
    }

    static ColorValue from_math_value(uint32_t value) {
        return ColorValue(
            (value >> 24) & 0xFF,
            (value >> 16) & 0xFF,
            (value >> 8) & 0xFF,
            value & 0xFF
        );
    }

    uint64_t to_precise_value() const {
        return (static_cast<uint64_t>(r) << 32) |
               (static_cast<uint64_t>(g) << 16) |
               static_cast<uint64_t>(b);
    }

    static ColorValue from_precise_value(uint64_t value) {
        return ColorValue(
            (value >> 32) & 0xFF,
            (value >> 16) & 0xFF,
            value & 0xFF,
            255
        );
    }

    ColorValue mod_add(const ColorValue& other, uint32_t modulus) const;
    ColorValue mod_subtract(const ColorValue& other, uint32_t modulus) const;
    ColorValue mod_multiply(const ColorValue& other, uint32_t modulus) const;

    ColorValue to_hsv() const;
    ColorValue from_hsv() const;

    bool operator==(const ColorValue& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    bool operator!=(const ColorValue& other) const {
        return !(*this == other);
    }

    std::string to_string() const;
    void print() const { std::cout << to_string() << std::endl; }
};

namespace color_ops {

    ColorValue add_colors(const ColorValue& a, const ColorValue& b);
    ColorValue multiply_colors(const ColorValue& a, const ColorValue& b);
    ColorValue mod_reduce_color(const ColorValue& c, uint32_t modulus);

    #ifdef HAVE_AVX512
    __m512i add_colors_avx512(__m512i a, __m512i b);
    __m512i multiply_colors_avx512(__m512i a, __m512i b);
    __m512i mod_reduce_colors_avx512(__m512i c, uint32_t modulus);
    #endif

    #ifdef __ARM_NEON
    #include <arm_neon.h>
    uint32x4_t add_colors_neon(uint32x4_t a, uint32x4_t b);
    uint32x4_t multiply_colors_neon(uint32x4_t a, uint32x4_t b);
    uint32x4_t mod_reduce_colors_neon(uint32x4_t c, uint32_t modulus);
    #endif

    ColorValue add_colors_simd(const ColorValue& a, const ColorValue& b);
    ColorValue multiply_colors_simd(const ColorValue& a, const ColorValue& b);
    ColorValue mod_reduce_color_simd(const ColorValue& c, uint32_t modulus);

};

} // namespace clwe

#endif // COLOR_VALUE_HPP