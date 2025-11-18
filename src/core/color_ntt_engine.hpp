#ifndef COLOR_NTT_ENGINE_HPP
#define COLOR_NTT_ENGINE_HPP

#include "ntt_engine.hpp"
#include "color_value.hpp"
#include <vector>

namespace clwe {

class ColorNTTEngine : public NTTEngine {
private:
    std::vector<ColorValue> color_zetas_;
    std::vector<ColorValue> color_zetas_inv_;

    ColorValue color_to_crypto_space(const ColorValue& color) const;
    ColorValue crypto_space_to_color(const ColorValue& crypto_val) const;

    void color_butterfly(ColorValue& a, ColorValue& b, const ColorValue& zeta, uint32_t modulus) const;
    void color_butterfly_inv(ColorValue& a, ColorValue& b, const ColorValue& zeta, uint32_t modulus) const;

    ColorValue color_add_precise(const ColorValue& a, const ColorValue& b, uint32_t modulus) const;
    ColorValue color_subtract_precise(const ColorValue& a, const ColorValue& b, uint32_t modulus) const;
    ColorValue color_multiply_precise(const ColorValue& a, const ColorValue& b, uint32_t modulus) const;

public:
    ColorNTTEngine(uint32_t q, uint32_t n);
    ~ColorNTTEngine() override = default;

    void ntt_forward_colors(ColorValue* poly) const;
    void ntt_inverse_colors(ColorValue* poly) const;
    void multiply_colors(const ColorValue* a, const ColorValue* b, ColorValue* result) const;

    void ntt_forward(uint32_t* poly) const override;
    void ntt_inverse(uint32_t* poly) const override;
    void multiply(const uint32_t* a, const uint32_t* b, uint32_t* result) const override;

    SIMDSupport get_simd_support() const override { return SIMDSupport::NONE; }

    void convert_uint32_to_colors(const uint32_t* coeffs, ColorValue* colors) const;
    void convert_colors_to_uint32(const ColorValue* colors, uint32_t* coeffs) const;
};

} // namespace clwe

#endif // COLOR_NTT_ENGINE_HPP