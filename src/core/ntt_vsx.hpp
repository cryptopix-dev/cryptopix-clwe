#ifndef NTT_VSX_HPP
#define NTT_VSX_HPP

#include "ntt_engine.hpp"
#include <cstdint>

#ifdef __VSX__
#include <altivec.h>
#endif

namespace clwe {

class VSXNTTEngine : public NTTEngine {
private:
#ifdef __VSX__
    // VSX uses 128-bit vectors (4 float/int32)
    __vector unsigned int* zetas_;       // Precomputed zetas in VSX format
    __vector unsigned int* zetas_inv_;   // Inverse zetas in VSX format
#endif

    // Montgomery reduction constants
    uint32_t montgomery_r_;       // 2^32 mod q
    uint32_t montgomery_r_inv_;   // R^(-1) mod q

    // Precompute zetas for NTT
    void precompute_zetas();

    // VSX butterfly operations
    void butterfly_vsx(__vector unsigned int& a, __vector unsigned int& b, __vector unsigned int zeta) const;

    // Modular reduction
    __vector unsigned int mod_reduce_vsx(__vector unsigned int val) const;

    // Montgomery reduction
    uint32_t montgomery_reduce(uint64_t val) const;
    __vector unsigned int montgomery_reduce_vsx(__vector unsigned int val) const;

public:
    VSXNTTEngine(uint32_t q, uint32_t n);
    ~VSXNTTEngine() override;

    // Implement pure virtual methods
    void ntt_forward(uint32_t* poly) const override;
    void ntt_inverse(uint32_t* poly) const override;
    void multiply(const uint32_t* a, const uint32_t* b, uint32_t* result) const override;

    SIMDSupport get_simd_support() const override { return SIMDSupport::VSX; }
};

} // namespace clwe

#endif // NTT_VSX_HPP