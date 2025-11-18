#include "color_value.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

#ifdef HAVE_AVX512
#include <immintrin.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

namespace clwe {

ColorValue ColorValue::mod_add(const ColorValue& other, uint32_t modulus) const {
    uint32_t this_val = to_math_value();
    uint32_t other_val = other.to_math_value();
    uint32_t sum = (this_val + other_val) % modulus;
    return from_math_value(sum);
}

ColorValue ColorValue::mod_subtract(const ColorValue& other, uint32_t modulus) const {
    uint32_t this_val = to_math_value();
    uint32_t other_val = other.to_math_value();
    uint32_t diff = (this_val >= other_val) ? (this_val - other_val) : (this_val + modulus - other_val);
    diff %= modulus;
    return from_math_value(diff);
}

ColorValue ColorValue::mod_multiply(const ColorValue& other, uint32_t modulus) const {
    uint32_t this_val = to_math_value();
    uint32_t other_val = other.to_math_value();
    uint64_t product = static_cast<uint64_t>(this_val) * other_val;
    uint32_t result = product % modulus;
    return from_math_value(result);
}

ColorValue ColorValue::to_hsv() const {
    float r = this->r / 255.0f;
    float g = this->g / 255.0f;
    float b = this->b / 255.0f;

    float max_val = std::max({r, g, b});
    float min_val = std::min({r, g, b});
    float delta = max_val - min_val;

    float h, s, v = max_val;

    if (delta == 0) {
        h = 0;
    } else if (max_val == r) {
        h = 60 * fmod(((g - b) / delta), 6);
    } else if (max_val == g) {
        h = 60 * (((b - r) / delta) + 2);
    } else {
        h = 60 * (((r - g) / delta) + 4);
    }

    if (h < 0) h += 360;
    s = (max_val == 0) ? 0 : (delta / max_val);

    return ColorValue(
        static_cast<uint8_t>(h / 360.0f * 255),
        static_cast<uint8_t>(s * 255),
        static_cast<uint8_t>(v * 255),
        this->a
    );
}

ColorValue ColorValue::from_hsv() const {
    float h = this->r / 255.0f * 360.0f;
    float s = this->g / 255.0f;
    float v = this->b / 255.0f;

    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
    float m = v - c;

    float r, g, b;

    if (h >= 0 && h < 60) {
        r = c; g = x; b = 0;
    } else if (h >= 60 && h < 120) {
        r = x; g = c; b = 0;
    } else if (h >= 120 && h < 180) {
        r = 0; g = c; b = x;
    } else if (h >= 180 && h < 240) {
        r = 0; g = x; b = c;
    } else if (h >= 240 && h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }

    return ColorValue(
        static_cast<uint8_t>((r + m) * 255),
        static_cast<uint8_t>((g + m) * 255),
        static_cast<uint8_t>((b + m) * 255),
        this->a
    );
}

std::string ColorValue::to_string() const {
    std::stringstream ss;
    ss << "Color(" << static_cast<int>(r) << ", "
       << static_cast<int>(g) << ", "
       << static_cast<int>(b) << ", "
       << static_cast<int>(a) << ")";
    return ss.str();
}

namespace color_ops {

ColorValue add_colors(const ColorValue& a, const ColorValue& b) {
    return ColorValue(
        std::min(255, a.r + b.r),
        std::min(255, a.g + b.g),
        std::min(255, a.b + b.b),
        std::min(255, a.a + b.a)
    );
}

ColorValue multiply_colors(const ColorValue& a, const ColorValue& b) {
    return ColorValue(
        (a.r * b.r) / 255,
        (a.g * b.g) / 255,
        (a.b * b.b) / 255,
        (a.a * b.a) / 255
    );
}

ColorValue mod_reduce_color(const ColorValue& c, uint32_t modulus) {
    uint32_t val = c.to_math_value();
    val %= modulus;
    return ColorValue::from_math_value(val);
}

#ifdef HAVE_AVX512
__m512i add_colors_avx512(__m512i a, __m512i b) {
    return _mm512_add_epi8(a, b);
}

__m512i multiply_colors_avx512(__m512i a, __m512i b) {
    __m512i product = _mm512_mullo_epi8(a, b);
    __m512i divisor = _mm512_set1_epi8(255);
    return _mm512_div_epi8(product, divisor);
}

__m512i mod_reduce_colors_avx512(__m512i c, uint32_t modulus) {
    __m512i mod_vec = _mm512_set1_epi32(modulus);
    return _mm512_rem_epi32(_mm512_cvtepi8_epi32(_mm512_extracti32x4_epi32(c, 0)), mod_vec);
}
#endif

#ifdef __ARM_NEON
uint32x4_t add_colors_neon(uint32x4_t a, uint32x4_t b) {
    return vaddq_u32(a, b);
}

uint32x4_t multiply_colors_neon(uint32x4_t a, uint32x4_t b) {
    uint32_t a_vals[4], b_vals[4], result[4];
    vst1q_u32(a_vals, a);
    vst1q_u32(b_vals, b);

    for (int i = 0; i < 4; ++i) {
        result[i] = (a_vals[i] * b_vals[i]) / 255;
        result[i] = result[i] > 255 ? 255 : result[i];
    }

    return vld1q_u32(result);
}

uint32x4_t mod_reduce_colors_neon(uint32x4_t c, uint32_t modulus) {
    uint32_t vals[4];
    vst1q_u32(vals, c);

    for (int i = 0; i < 4; ++i) {
        vals[i] %= modulus;
    }

    return vld1q_u32(vals);
}
#endif

ColorValue add_colors_simd(const ColorValue& a, const ColorValue& b) {
    return add_colors(a, b);
}

ColorValue multiply_colors_simd(const ColorValue& a, const ColorValue& b) {
    return multiply_colors(a, b);
}

ColorValue mod_reduce_color_simd(const ColorValue& c, uint32_t modulus) {
    return mod_reduce_color(c, modulus);
}

} // namespace color_ops

} // namespace clwe