#include "color_ntt_engine.hpp"
#include "utils.hpp"
#include <algorithm>
#include <iostream>

namespace clwe {

ColorNTTEngine::ColorNTTEngine(uint32_t q, uint32_t n)
    : NTTEngine(q, n), color_zetas_(n), color_zetas_inv_(n) {

    std::vector<uint32_t> standard_zetas(n);
    std::vector<uint32_t> standard_zetas_inv(n);

    uint32_t g = (q_ == 3329) ? 17 : 3;
    uint32_t zeta = mod_pow(g, (q_ - 1) / n_, q_);

    standard_zetas[0] = 1;
    for (uint32_t i = 1; i < n_; ++i) {
        standard_zetas[i] = (static_cast<uint64_t>(standard_zetas[i-1]) * zeta) % q_;
    }

    uint32_t zeta_inv = mod_inverse(zeta, q_);
    standard_zetas_inv[0] = 1;
    for (uint32_t i = 1; i < n_; ++i) {
        standard_zetas_inv[i] = (static_cast<uint64_t>(standard_zetas_inv[i-1]) * zeta_inv) % q_;
    }

    for (uint32_t i = 0; i < n_; ++i) {
        color_zetas_[i] = ColorValue::from_math_value(standard_zetas[i]);
        color_zetas_inv_[i] = ColorValue::from_math_value(standard_zetas_inv[i]);
    }
}

ColorValue ColorNTTEngine::color_to_crypto_space(const ColorValue& color) const {
    return color;
}

ColorValue ColorNTTEngine::crypto_space_to_color(const ColorValue& crypto_val) const {
    return crypto_val;
}

void ColorNTTEngine::color_butterfly(ColorValue& a, ColorValue& b, const ColorValue& zeta, uint32_t modulus) const {
    ColorValue sum = color_add_precise(a, b, modulus);
    ColorValue diff = color_subtract_precise(a, b, modulus);
    ColorValue product = color_multiply_precise(diff, zeta, modulus);

    a = sum;
    b = product;
}

ColorValue ColorNTTEngine::color_add_precise(const ColorValue& a, const ColorValue& b, uint32_t modulus) const {
    uint64_t a_val = a.to_precise_value();
    uint64_t b_val = b.to_precise_value();
    uint64_t sum = (a_val + b_val) % modulus;
    return ColorValue::from_precise_value(sum);
}

ColorValue ColorNTTEngine::color_subtract_precise(const ColorValue& a, const ColorValue& b, uint32_t modulus) const {
    uint64_t a_val = a.to_precise_value();
    uint64_t b_val = b.to_precise_value();
    uint64_t diff = (a_val >= b_val) ? (a_val - b_val) : (a_val + modulus - b_val);
    diff %= modulus;
    return ColorValue::from_precise_value(diff);
}

ColorValue ColorNTTEngine::color_multiply_precise(const ColorValue& a, const ColorValue& b, uint32_t modulus) const {
    uint64_t a_val = a.to_precise_value();
    uint64_t b_val = b.to_precise_value();
    uint64_t product = (a_val * b_val) % modulus;
    return ColorValue::from_precise_value(product);
}

void ColorNTTEngine::color_butterfly_inv(ColorValue& a, ColorValue& b, const ColorValue& zeta, uint32_t modulus) const {
    color_butterfly(a, b, zeta, modulus);
}

void ColorNTTEngine::ntt_forward_colors(ColorValue* poly) const {
    uint32_t m = 1;
    uint32_t k = n_ / 2;

    for (uint32_t stage = 0; stage < log_degree(); ++stage) {
        uint32_t j = 0;
        for (uint32_t i = 0; i < k; ++i) {
            const ColorValue& zeta = color_zetas_[j];
            color_butterfly(poly[i], poly[i + k], zeta, modulus());
            j += m;
        }
        m *= 2;
        k /= 2;
    }
}

void ColorNTTEngine::ntt_inverse_colors(ColorValue* poly) const {
    uint32_t m = n_ / 2;
    uint32_t k = 1;

    for (uint32_t stage = 0; stage < log_degree(); ++stage) {
        uint32_t j = 0;
        for (uint32_t i = 0; i < k; ++i) {
            const ColorValue& zeta = color_zetas_inv_[j];
            color_butterfly_inv(poly[i], poly[i + k], zeta, modulus());
            j += m;
        }
        m /= 2;
        k *= 2;
    }

    uint32_t n_inv = mod_inverse(n_, modulus());
    for (uint32_t i = 0; i < n_; ++i) {
        uint64_t current_val = poly[i].to_precise_value();
        uint64_t scaled = (current_val * n_inv) % modulus();
        poly[i] = ColorValue::from_precise_value(scaled);
    }
}

void ColorNTTEngine::multiply_colors(const ColorValue* a, const ColorValue* b, ColorValue* result) const {
    std::vector<ColorValue> a_ntt(n_);
    std::vector<ColorValue> b_ntt(n_);
    std::copy(a, a + n_, a_ntt.begin());
    std::copy(b, b + n_, b_ntt.begin());

    ntt_forward_colors(a_ntt.data());
    ntt_forward_colors(b_ntt.data());

    for (uint32_t i = 0; i < n_; ++i) {
        uint64_t a_val = a_ntt[i].to_precise_value();
        uint64_t b_val = b_ntt[i].to_precise_value();
        uint64_t product = (a_val * b_val) % modulus();
        result[i] = ColorValue::from_precise_value(product);
    }

    ntt_inverse_colors(result);
}

void ColorNTTEngine::convert_uint32_to_colors(const uint32_t* coeffs, ColorValue* colors) const {
    for (uint32_t i = 0; i < n_; ++i) {
        colors[i] = ColorValue::from_precise_value(coeffs[i]);
    }
}

void ColorNTTEngine::convert_colors_to_uint32(const ColorValue* colors, uint32_t* coeffs) const {
    for (uint32_t i = 0; i < n_; ++i) {
        coeffs[i] = static_cast<uint32_t>(colors[i].to_precise_value());
    }
}

void ColorNTTEngine::ntt_forward(uint32_t* poly) const {
    std::vector<ColorValue> color_poly(n_);
    convert_uint32_to_colors(poly, color_poly.data());
    ntt_forward_colors(color_poly.data());
    convert_colors_to_uint32(color_poly.data(), poly);
}

void ColorNTTEngine::ntt_inverse(uint32_t* poly) const {
    std::vector<ColorValue> color_poly(n_);
    convert_uint32_to_colors(poly, color_poly.data());
    ntt_inverse_colors(color_poly.data());
    convert_colors_to_uint32(color_poly.data(), poly);
}

void ColorNTTEngine::multiply(const uint32_t* a, const uint32_t* b, uint32_t* result) const {
    std::vector<ColorValue> color_a(n_), color_b(n_), color_result(n_);
    convert_uint32_to_colors(a, color_a.data());
    convert_uint32_to_colors(b, color_b.data());
    multiply_colors(color_a.data(), color_b.data(), color_result.data());
    convert_colors_to_uint32(color_result.data(), result);
}

} // namespace clwe