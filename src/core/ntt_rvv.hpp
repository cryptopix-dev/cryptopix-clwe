#ifndef NTT_RVV_HPP
#define NTT_RVV_HPP

#include "ntt_engine.hpp"
#include <cstdint>

#ifdef __riscv_v
#include <riscv_vector.h>
#endif

namespace clwe {

class RVVNTTEngine : public NTTEngine {
private:
    // RVV uses variable-length vectors
    uint32_t* zetas_;       // Precomputed zetas
    uint32_t* zetas_inv_;   // Inverse zetas

    // Montgomery reduction constants
    uint32_t montgomery_r_;       // 2^32 mod q
    uint32_t montgomery_r_inv_;   // R^(-1) mod q

    // Precompute zetas for NTT
    void precompute_zetas();

    // RVV butterfly operations
    void butterfly_rvv(uint32_t* a, uint32_t* b, uint32_t zeta, size_t vl) const;
    void butterfly_inv_rvv(uint32_t* a, uint32_t* b, uint32_t zeta, size_t vl) const;

    // Modular reduction
    void mod_reduce_rvv(uint32_t* val, size_t vl) const;

    // Montgomery reduction
    uint32_t montgomery_reduce(uint64_t val) const;
    void montgomery_reduce_rvv(uint32_t* val, size_t vl) const;

public:
    RVVNTTEngine(uint32_t q, uint32_t n);
    ~RVVNTTEngine() override;

    // Implement pure virtual methods
    void ntt_forward(uint32_t* poly) const override;
    void ntt_inverse(uint32_t* poly) const override;
    void multiply(const uint32_t* a, const uint32_t* b, uint32_t* result) const override;

    SIMDSupport get_simd_support() const override { return SIMDSupport::RVV; }
};

} // namespace clwe

#endif // NTT_RVV_HPP