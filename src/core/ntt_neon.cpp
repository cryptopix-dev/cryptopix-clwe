#include "ntt_neon.hpp"
#include "utils.hpp"
#include <algorithm>
#include <iostream>
#include <cstring>

namespace clwe {

NEONNTTEngine::NEONNTTEngine(uint32_t q, uint32_t n)
    : NTTEngine(q, n), zetas_(nullptr), zetas_inv_(nullptr),
      montgomery_r_(0), montgomery_r_inv_(0) {

#ifdef __ARM_NEON
    // NEON: 4 uint32_t per uint32x4_t
    size_t neon_count = n / 4;
    zetas_ = new uint32x4_t[neon_count];
    zetas_inv_ = new uint32x4_t[neon_count];
#endif

    // Pre-compute Montgomery constants
    montgomery_r_ = (1ULL << 32) % q_;
    montgomery_r_inv_ = mod_inverse(montgomery_r_, q_);

    precompute_zetas();
}

NEONNTTEngine::~NEONNTTEngine() {
#ifdef __ARM_NEON
    if (zetas_) delete[] zetas_;
    if (zetas_inv_) delete[] zetas_inv_;
#endif
}

void NEONNTTEngine::precompute_zetas() {
    // Precompute primitive n-th root of unity
    uint32_t g = 17;  // Primitive root for q = 3329 (Kyber modulus)
    uint32_t zeta = mod_pow(g, (q_ - 1) / n_, q_);

    std::vector<uint32_t> zetas_scalar(n_);
    std::vector<uint32_t> zetas_inv_scalar(n_);

    zetas_scalar[0] = 1;
    for (uint32_t i = 1; i < n_; ++i) {
        zetas_scalar[i] = (static_cast<uint64_t>(zetas_scalar[i-1]) * zeta) % q_;
    }

    // Inverse zetas for inverse NTT
    uint32_t zeta_inv = mod_inverse(zeta, q_);
    zetas_inv_scalar[0] = 1;
    for (uint32_t i = 1; i < n_; ++i) {
        zetas_inv_scalar[i] = (static_cast<uint64_t>(zetas_inv_scalar[i-1]) * zeta_inv) % q_;
    }

#ifdef __ARM_NEON
    // Pack into NEON vectors (4 values per vector)
    for (uint32_t i = 0; i < n_; i += 4) {
        uint32_t vals[4];
        uint32_t vals_inv[4];
        for (int j = 0; j < 4 && i + j < n_; ++j) {
            vals[j] = zetas_scalar[i + j];
            vals_inv[j] = zetas_inv_scalar[i + j];
        }
        zetas_[i/4] = vld1q_u32(vals);
        zetas_inv_[i/4] = vld1q_u32(vals_inv);
    }
#endif
}

void NEONNTTEngine::butterfly_neon(uint32x4_t& a, uint32x4_t& b, uint32x4_t zeta) const {
#ifdef __ARM_NEON
    // NEON butterfly: a, b = a + b, (a - b) * zeta
    uint32x4_t sum = vaddq_u32(a, b);
    uint32x4_t diff = vsubq_u32(a, b);

    // Multiply diff by zeta (this is simplified - actual Montgomery multiplication needed)
    // For now, use scalar multiplication as placeholder
    uint32_t a_vals[4], b_vals[4], zeta_vals[4];
    vst1q_u32(a_vals, a);
    vst1q_u32(b_vals, b);
    vst1q_u32(zeta_vals, zeta);

    for (int i = 0; i < 4; ++i) {
        uint32_t prod = montgomery_reduce(static_cast<uint64_t>(b_vals[i]) * zeta_vals[i]);
        a_vals[i] = sum[i];  // This won't work - need proper vector access
        b_vals[i] = prod;
    }

    // This is a placeholder - proper NEON Montgomery multiplication needed
    a = vld1q_u32(a_vals);
    b = vld1q_u32(b_vals);
#endif
}

void NEONNTTEngine::butterfly_inv_neon(uint32x4_t& a, uint32x4_t& b, uint32x4_t zeta) const {
    // Same as forward for inverse (zeta is already inverse)
    butterfly_neon(a, b, zeta);
}

uint32x4_t NEONNTTEngine::mod_reduce_neon(uint32x4_t val) const {
#ifdef __ARM_NEON
    // Placeholder - proper NEON modular reduction needed
    uint32_t vals[4];
    vst1q_u32(vals, val);
    for (int i = 0; i < 4; ++i) {
        if (vals[i] >= q_) {
            vals[i] -= q_;
        }
    }
    return vld1q_u32(vals);
#else
    return val;  // Should not reach here
#endif
}

uint32_t NEONNTTEngine::montgomery_reduce(uint64_t val) const {
    // Montgomery reduction: (val * R^-1) mod q
    uint64_t t = val * montgomery_r_inv_;
    uint32_t k = t % (1ULL << 32);
    uint64_t res = val - static_cast<uint64_t>(k) * q_;
    return res >> 32;
}

uint32x4_t NEONNTTEngine::montgomery_reduce_neon(uint32x4_t val) const {
#ifdef __ARM_NEON
    // Placeholder - proper NEON Montgomery reduction needed
    uint32_t vals[4];
    vst1q_u32(vals, val);
    for (int i = 0; i < 4; ++i) {
        vals[i] = montgomery_reduce(vals[i]);
    }
    return vld1q_u32(vals);
#else
    return val;  // Should not reach here
#endif
}

void NEONNTTEngine::ntt_forward(uint32_t* poly) const {
#ifdef __ARM_NEON
    // NEON-optimized NTT implementation
    uint32_t m = 1;
    uint32_t k = n_ / 2;

    for (uint32_t stage = 0; stage < log_degree(); ++stage) {
        uint32_t j = 0;
        for (uint32_t i = 0; i < k; i += 4) {  // Process 4 butterflies at once
            if (i + 4 <= k) {
                // Load zeta values
                uint32x4_t zeta_vec = zetas_[j];

                // Load and process 4 pairs
                uint32x4_t a0 = vld1q_u32(&poly[i]);
                uint32x4_t b0 = vld1q_u32(&poly[i + k]);

                butterfly_neon(a0, b0, zeta_vec);

                vst1q_u32(&poly[i], a0);
                vst1q_u32(&poly[i + k], b0);
            } else {
                // Handle remaining elements with scalar operations
                for (uint32_t idx = i; idx < k; ++idx) {
                    uint32_t zeta = reinterpret_cast<const uint32_t*>(&zetas_[j/4])[j%4];
                    uint32_t sum = (poly[idx] + poly[idx + k]) % q_;
                    uint32_t diff = (poly[idx] >= poly[idx + k]) ?
                        (poly[idx] - poly[idx + k]) : (poly[idx] + q_ - poly[idx + k]);
                    uint32_t prod = montgomery_reduce(static_cast<uint64_t>(diff) * zeta);
                    poly[idx] = sum;
                    poly[idx + k] = prod;
                }
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

void NEONNTTEngine::ntt_inverse(uint32_t* poly) const {
#ifdef __ARM_NEON
    // NEON inverse NTT
    uint32_t m = n_ / 2;
    uint32_t k = 1;

    for (uint32_t stage = 0; stage < log_degree(); ++stage) {
        uint32_t j = 0;
        for (uint32_t i = 0; i < k; i += 4) {
            if (i + 4 <= k) {
                uint32x4_t zeta_vec = zetas_inv_[j];
                uint32x4_t a0 = vld1q_u32(&poly[i]);
                uint32x4_t b0 = vld1q_u32(&poly[i + k]);

                butterfly_inv_neon(a0, b0, zeta_vec);

                vst1q_u32(&poly[i], a0);
                vst1q_u32(&poly[i + k], b0);
            } else {
                // Scalar fallback
                for (uint32_t idx = i; idx < k; ++idx) {
                    uint32_t zeta = reinterpret_cast<const uint32_t*>(&zetas_inv_[j/4])[j%4];
                    uint32_t sum = (poly[idx] + poly[idx + k]) % q_;
                    uint32_t diff = (poly[idx] >= poly[idx + k]) ?
                        (poly[idx] - poly[idx + k]) : (poly[idx] + q_ - poly[idx + k]);
                    uint32_t prod = montgomery_reduce(static_cast<uint64_t>(diff) * zeta);
                    poly[idx] = sum;
                    poly[idx + k] = prod;
                }
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

void NEONNTTEngine::multiply(const uint32_t* a, const uint32_t* b, uint32_t* result) const {
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