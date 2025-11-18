#include "ntt_engine.hpp"
#include "cpu_features.hpp"
#include "ntt_scalar.hpp"
#include "ntt_neon.hpp"
#include "ntt_rvv.hpp"
#include "utils.hpp"
#include <algorithm>
#include <stdexcept>

namespace clwe {

NTTEngine::NTTEngine(uint32_t q, uint32_t n)
    : q_(q), n_(n), log_n_(0), bitrev_(n) {

    if (!is_power_of_two(n)) {
        throw std::invalid_argument("NTT degree must be a power of 2");
    }

    // Calculate log_n_
    uint32_t temp = n;
    while (temp > 1) {
        temp >>= 1;
        log_n_++;
    }

    precompute_bitrev();
}

void NTTEngine::precompute_bitrev() {
    for (uint32_t i = 0; i < n_; ++i) {
        uint32_t rev = 0;
        for (uint32_t j = 0; j < log_n_; ++j) {
            rev |= ((i >> j) & 1) << (log_n_ - 1 - j);
        }
        bitrev_[i] = rev;
    }
}

void NTTEngine::bit_reverse(uint32_t* poly) const {
    std::vector<uint32_t> temp(n_);
    for (uint32_t i = 0; i < n_; ++i) {
        temp[bitrev_[i]] = poly[i];
    }
    std::copy(temp.begin(), temp.end(), poly);
}

void NTTEngine::copy_from_uint32(const uint32_t* coeffs, uint32_t* ntt_coeffs) const {
    std::copy(coeffs, coeffs + n_, ntt_coeffs);
}

void NTTEngine::copy_to_uint32(const uint32_t* ntt_coeffs, uint32_t* coeffs) const {
    std::copy(ntt_coeffs, ntt_coeffs + n_, coeffs);
}

// Forward declarations for concrete implementations
class ScalarNTTEngine;
// class AVX2NTTEngine;
// class AVX512NTTEngine;
class NEONNTTEngine;
class RVVNTTEngine;
// class VSXNTTEngine;

std::unique_ptr<NTTEngine> create_optimal_ntt_engine(uint32_t q, uint32_t n) {
    CPUFeatures features = CPUFeatureDetector::detect();
    return create_ntt_engine(features.max_simd_support, q, n);
}

std::unique_ptr<NTTEngine> create_ntt_engine(SIMDSupport simd_support, uint32_t q, uint32_t n) {
    switch (simd_support) {
        case SIMDSupport::NEON:
            return std::make_unique<NEONNTTEngine>(q, n);
        case SIMDSupport::RVV:
            return std::make_unique<RVVNTTEngine>(q, n);
        case SIMDSupport::AVX512:
        case SIMDSupport::AVX2:
        case SIMDSupport::VSX:
        case SIMDSupport::NONE:
        default:
            return std::make_unique<ScalarNTTEngine>(q, n);
    }
}

} // namespace clwe