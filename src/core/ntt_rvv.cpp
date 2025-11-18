#include "ntt_rvv.hpp"
#include "utils.hpp"
#include <algorithm>
#include <iostream>
#include <cstring>

namespace clwe {

RVVNTTEngine::RVVNTTEngine(uint32_t q, uint32_t n)
    : NTTEngine(q, n), zetas_(nullptr), zetas_inv_(nullptr),
      montgomery_r_(0), montgomery_r_inv_(0) {

    zetas_ = new uint32_t[n];
    zetas_inv_ = new uint32_t[n];

    // Pre-compute Montgomery constants
    montgomery_r_ = (1ULL << 32) % q_;
    montgomery_r_inv_ = mod_inverse(montgomery_r_, q_);

    precompute_zetas();
}

RVVNTTEngine::~RVVNTTEngine() {
    if (zetas_) delete[] zetas_;
    if (zetas_inv_) delete[] zetas_inv_;
}

void RVVNTTEngine::precompute_zetas() {
    // Precompute primitive n-th root of unity
    uint32_t g = 17;  // Primitive root for q = 3329 (Kyber modulus)
    uint32_t zeta = mod_pow(g, (q_ - 1) / n_, q_);

    zetas_[0] = 1;
    for (uint32_t i = 1; i < n_; ++i) {
        zetas_[i] = (static_cast<uint64_t>(zetas_[i-1]) * zeta) % q_;
    }

    // Inverse zetas for inverse NTT
    uint32_t zeta_inv = mod_inverse(zeta, q_);
    zetas_inv_[0] = 1;
    for (uint32_t i = 1; i < n_; ++i) {
        zetas_inv_[i] = (static_cast<uint64_t>(zetas_inv_[i-1]) * zeta_inv) % q_;
    }
}

void RVVNTTEngine::butterfly_rvv(uint32_t* a, uint32_t* b, uint32_t zeta, size_t vl) const {
#ifdef __riscv_v
    // RVV vectorized butterfly operation
    // This is a simplified implementation - full RVV optimization would be more complex
    vuint32m1_t va = vle32_v_u32m1(a, vl);
    vuint32m1_t vb = vle32_v_u32m1(b, vl);
    vuint32m1_t vzeta = vmv_v_x_u32m1(zeta, vl);

    // sum = a + b
    vuint32m1_t vsum = vadd_vv_u32m1(va, vb, vl);

    // diff = a - b
    vuint32m1_t vdiff = vsub_vv_u32m1(va, vb, vl);

    // prod = (a - b) * zeta mod q
    // This is simplified - proper modular multiplication needed
    vuint32m1_t vprod = vmul_vv_u32m1(vdiff, vzeta, vl);

    // Store results
    vse32_v_u32m1(a, vsum, vl);
    vse32_v_u32m1(b, vprod, vl);
#endif
}

void RVVNTTEngine::butterfly_inv_rvv(uint32_t* a, uint32_t* b, uint32_t zeta, size_t vl) const {
    // Same as forward for inverse (zeta is already inverse)
    butterfly_rvv(a, b, zeta, vl);
}

void RVVNTTEngine::mod_reduce_rvv(uint32_t* val, size_t vl) const {
#ifdef __riscv_v
    // RVV modular reduction
    vuint32m1_t vval = vle32_v_u32m1(val, vl);
    vuint32m1_t vq = vmv_v_x_u32m1(q_, vl);

    // Simple modular reduction (not optimal)
    vuint32m1_t vmask = vmsgeu_vx_u32m1_b8(vval, vq, vl);
    vval = vsub_vv_u32m1_m(vmask, vval, vval, vq, vl);

    vse32_v_u32m1(val, vval, vl);
#endif
}

uint32_t RVVNTTEngine::montgomery_reduce(uint64_t val) const {
    // Montgomery reduction: (val * R^-1) mod q
    uint64_t t = val * montgomery_r_inv_;
    uint32_t k = t % (1ULL << 32);
    uint64_t res = val - static_cast<uint64_t>(k) * q_;
    return res >> 32;
}

void RVVNTTEngine::montgomery_reduce_rvv(uint32_t* val, size_t vl) const {
#ifdef __riscv_v
    // Simplified RVV Montgomery reduction
    for (size_t i = 0; i < vl; ++i) {
        val[i] = montgomery_reduce(val[i]);
    }
#endif
}

void RVVNTTEngine::ntt_forward(uint32_t* poly) const {
#ifdef __riscv_v
    // RVV-optimized NTT implementation
    size_t vl = vsetvl_e32m1(n_);  // Set vector length
    uint32_t m = 1;
    uint32_t k = n_ / 2;

    for (uint32_t stage = 0; stage < log_degree(); ++stage) {
        uint32_t j = 0;
        for (uint32_t i = 0; i < k; i += vl) {
            size_t current_vl = (i + vl <= k) ? vl : (k - i);
            if (current_vl > 0) {
                uint32_t zeta = zetas_[j];
                butterfly_rvv(&poly[i], &poly[i + k], zeta, current_vl);
                mod_reduce_rvv(&poly[i], current_vl);
                mod_reduce_rvv(&poly[i + k], current_vl);
            }
            j += m;
        }
        m *= 2;
        k /= 2;
    }
#else
    // Fallback to scalar implementation
    ntt_forward(poly);
#endif
}

void RVVNTTEngine::ntt_inverse(uint32_t* poly) const {
#ifdef __riscv_v
    // RVV inverse NTT
    size_t vl = vsetvl_e32m1(n_);
    uint32_t m = n_ / 2;
    uint32_t k = 1;

    for (uint32_t stage = 0; stage < log_degree(); ++stage) {
        uint32_t j = 0;
        for (uint32_t i = 0; i < k; i += vl) {
            size_t current_vl = (i + vl <= k) ? vl : (k - i);
            if (current_vl > 0) {
                uint32_t zeta = zetas_inv_[j];
                butterfly_inv_rvv(&poly[i], &poly[i + k], zeta, current_vl);
                mod_reduce_rvv(&poly[i], current_vl);
                mod_reduce_rvv(&poly[i + k], current_vl);
            }
            j += m;
        }
        m /= 2;
        k *= 2;
    }

    // Scale by n^(-1) mod q
    uint32_t n_inv = mod_inverse(n_, q_);
    for (uint32_t i = 0; i < n_; ++i) {
        poly[i] = montgomery_reduce(static_cast<uint64_t>(poly[i]) * n_inv);
    }
#else
    // Fallback
    ntt_inverse(poly);
#endif
}

void RVVNTTEngine::multiply(const uint32_t* a, const uint32_t* b, uint32_t* result) const {
    // Copy inputs for NTT
    std::vector<uint32_t> a_ntt(n_);
    std::vector<uint32_t> b_ntt(n_);
    std::copy(a, a + n_, a_ntt.begin());
    std::copy(b, b + n_, b_ntt.begin());

    // Forward NTT
    ntt_forward(a_ntt.data());
    ntt_forward(b_ntt.data());

    // Pointwise multiplication with Montgomery reduction
    for (uint32_t i = 0; i < n_; ++i) {
        result[i] = montgomery_reduce(static_cast<uint64_t>(a_ntt[i]) * b_ntt[i]);
    }

    // Inverse NTT
    ntt_inverse(result);
}

} // namespace clwe