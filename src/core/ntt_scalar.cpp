#include "ntt_scalar.hpp"
#include "utils.hpp"
#include <algorithm>
#include <iostream>
#include <cstring>
#include <unordered_map>

namespace clwe {

ScalarNTTEngine::ScalarNTTEngine(uint32_t q, uint32_t n)
    : NTTEngine(q, n), zetas_(n), zetas_inv_(n),
      montgomery_r_(0), montgomery_r_inv_(0) {

    // Pre-compute Montgomery constants for modular reduction
    montgomery_r_ = (1ULL << 32) % q_;
    montgomery_r_inv_ = mod_inverse(montgomery_r_, q_);

    precompute_zetas();
}

void ScalarNTTEngine::precompute_zetas() {
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

void ScalarNTTEngine::butterfly(uint32_t& a, uint32_t& b, uint32_t zeta) const {
    uint32_t sum = (a + b) % q_;
    uint32_t diff = (a >= b) ? (a - b) : (a + q_ - b);
    uint32_t prod = montgomery_reduce(static_cast<uint64_t>(diff) * zeta);
    a = sum;
    b = prod;
}

void ScalarNTTEngine::butterfly_inv(uint32_t& a, uint32_t& b, uint32_t zeta) const {
    // Same as forward for inverse (zeta is already inverse)
    butterfly(a, b, zeta);
}

uint32_t ScalarNTTEngine::mod_reduce(uint32_t val) const {
    return val % q_;
}

uint32_t ScalarNTTEngine::montgomery_reduce(uint64_t val) const {
    // Montgomery reduction: (val * R^-1) mod q
    uint64_t t = val * montgomery_r_inv_;
    uint32_t k = t % (1ULL << 32);
    uint64_t res = val - static_cast<uint64_t>(k) * q_;
    return res >> 32;
}

void ScalarNTTEngine::ntt_forward(uint32_t* poly) const {
    // Iterative NTT implementation
    uint32_t m = 1;
    uint32_t k = n_ / 2;

    for (uint32_t stage = 0; stage < log_degree(); ++stage) {
        uint32_t j = 0;
        for (uint32_t i = 0; i < k; ++i) {
            uint32_t zeta = zetas_[j];
            butterfly(poly[i], poly[i + k], zeta);
            j += m;
        }
        m *= 2;
        k /= 2;
    }
}

void ScalarNTTEngine::ntt_inverse(uint32_t* poly) const {
    // Inverse NTT: similar to forward but with inverse zetas and scaling
    uint32_t m = n_ / 2;
    uint32_t k = 1;

    for (uint32_t stage = 0; stage < log_degree(); ++stage) {
        uint32_t j = 0;
        for (uint32_t i = 0; i < k; ++i) {
            uint32_t zeta = zetas_inv_[j];
            butterfly_inv(poly[i], poly[i + k], zeta);
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
}

void ScalarNTTEngine::multiply(const uint32_t* a, const uint32_t* b, uint32_t* result) const {
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