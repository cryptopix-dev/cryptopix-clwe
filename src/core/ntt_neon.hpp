#ifndef NTT_NEON_HPP
#define NTT_NEON_HPP

#include "ntt_engine.hpp"
#include <cstdint>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

namespace clwe {

class NEONNTTEngine : public NTTEngine {
private:
#ifdef __ARM_NEON
    // NEON uses 128-bit vectors (4 uint32_t values)
    uint32x4_t* zetas_;       // Precomputed zetas in NEON format
    uint32x4_t* zetas_inv_;   // Inverse zetas in NEON format
#endif

    // Montgomery reduction constants
    uint32_t montgomery_r_;       // 2^32 mod q
    uint32_t montgomery_r_inv_;   // R^(-1) mod q

    // Precompute zetas for NTT
    void precompute_zetas();

    // NEON butterfly operations
    void butterfly_neon(uint32x4_t& a, uint32x4_t& b, uint32x4_t zeta) const;
    void butterfly_inv_neon(uint32x4_t& a, uint32x4_t& b, uint32x4_t zeta) const;

    // Modular reduction
    uint32x4_t mod_reduce_neon(uint32x4_t val) const;

    // Montgomery reduction
    uint32_t montgomery_reduce(uint64_t val) const;
    uint32x4_t montgomery_reduce_neon(uint32x4_t val) const;

public:
    NEONNTTEngine(uint32_t q, uint32_t n);
    ~NEONNTTEngine() override;

    // Implement pure virtual methods
    void ntt_forward(uint32_t* poly) const override;
    void ntt_inverse(uint32_t* poly) const override;
    void multiply(const uint32_t* a, const uint32_t* b, uint32_t* result) const override;

    SIMDSupport get_simd_support() const override { return SIMDSupport::NEON; }
};

} // namespace clwe

#endif // NTT_NEON_HPP