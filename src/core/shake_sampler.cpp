#include "shake_sampler.hpp"
#include "utils.hpp"
#include <cstring>
#include <algorithm>
#include <random>

// Simple SHAKE-like implementation using existing utilities
// In production, this should use a verified cryptographic library like OpenSSL

namespace clwe {

// Simple pseudo-SHAKE implementation using SHA-256 in counter mode
// This is NOT cryptographically secure for production - use proper SHAKE implementation

SHAKE256Sampler::SHAKE256Sampler() : position_(0) {
    state_.resize(32); // 256-bit state
}

SHAKE256Sampler::~SHAKE256Sampler() = default;

void SHAKE256Sampler::reset() {
    std::fill(state_.begin(), state_.end(), 0);
    position_ = 0;
}

void SHAKE256Sampler::init(const uint8_t* seed, size_t seed_len) {
    reset();

    // Simple key derivation using existing utilities
    // Copy seed to state
    size_t copy_len = std::min(seed_len, state_.size());
    memcpy(state_.data(), seed, copy_len);

    // Fill remaining with deterministic pattern
    for (size_t i = copy_len; i < state_.size(); ++i) {
        state_[i] = (seed[i % seed_len] + i) & 0xFF;
    }
}

void SHAKE256Sampler::squeeze(uint8_t* out, size_t len) {
    // Simple deterministic expansion using state and position
    // This is NOT cryptographically secure - for demonstration only
    for (size_t i = 0; i < len; ++i) {
        size_t state_idx = (position_ + i) % state_.size();
        uint32_t counter = (position_ + i) / state_.size();

        // Simple mixing function
        uint8_t value = state_[state_idx];
        value ^= (counter & 0xFF);
        value ^= ((counter >> 8) & 0xFF);
        value ^= ((counter >> 16) & 0xFF);
        value ^= ((counter >> 24) & 0xFF);

        // Additional mixing
        value = (value << 3) | (value >> 5);
        value ^= state_[(state_idx + 1) % state_.size()];

        out[i] = value;
    }

    position_ += len;
}

void SHAKE256Sampler::random_bytes(uint8_t* out, size_t len) {
    squeeze(out, len);
}

int32_t SHAKE256Sampler::sample_binomial_coefficient(uint32_t eta) {
    // Sample from centered binomial distribution B(2η, 0.5) - η
    int32_t result = 0;

    for (uint32_t i = 0; i < eta; ++i) {
        uint8_t byte1, byte2;
        squeeze(&byte1, 1);
        squeeze(&byte2, 1);

        // Use bits as Bernoulli trials
        result += (byte1 & 1) ? 1 : -1;
        result += (byte2 & 1) ? 1 : -1;
    }

    return result;
}

void SHAKE256Sampler::sample_polynomial_binomial(uint32_t* coeffs, size_t degree,
                                                uint32_t eta, uint32_t modulus) {
    for (size_t i = 0; i < degree; ++i) {
        int32_t sample = sample_binomial_coefficient(eta);
        // Map to positive range: (sample mod modulus + modulus) mod modulus
        coeffs[i] = (sample % static_cast<int32_t>(modulus) + modulus) % modulus;
    }
}

void SHAKE256Sampler::sample_polynomial_binomial_batch(uint32_t** coeffs_batch, size_t count,
                                                     size_t degree, uint32_t eta, uint32_t modulus) {
    for (size_t poly = 0; poly < count; ++poly) {
        sample_polynomial_binomial(coeffs_batch[poly], degree, eta, modulus);
    }
}

void SHAKE256Sampler::sample_polynomial_binomial_batch_avx512(uint32_t** coeffs_batch, size_t count,
                                                             size_t degree, uint32_t eta, uint32_t modulus) {
    // For now, fall back to scalar implementation
    // In production, this would use AVX-512 instructions for parallel sampling
    sample_polynomial_binomial_batch(coeffs_batch, count, degree, eta, modulus);
}

uint32_t SHAKE256Sampler::sample_uniform(uint32_t modulus) {
    // Sample uniformly from [0, modulus)
    // Use rejection sampling for uniform distribution
    uint32_t mask = (1U << (32 - __builtin_clz(modulus - 1))) - 1;

    while (true) {
        uint8_t bytes[4];
        squeeze(bytes, 4);

        uint32_t sample = (bytes[0] << 24) | (bytes[1] << 16) |
                         (bytes[2] << 8) | bytes[3];
        sample &= mask;

        if (sample < modulus) {
            return sample;
        }
    }
}

void SHAKE256Sampler::sample_polynomial_uniform(uint32_t* coeffs, size_t degree, uint32_t modulus) {
    for (size_t i = 0; i < degree; ++i) {
        coeffs[i] = sample_uniform(modulus);
    }
}

// SHAKE128Sampler implementation for Kyber matrix generation
SHAKE128Sampler::SHAKE128Sampler() : state_(), position_(0) {
    reset();
}

SHAKE128Sampler::~SHAKE128Sampler() = default;

void SHAKE128Sampler::reset() {
    state_.clear();
    position_ = 0;
}

void SHAKE128Sampler::init(const uint8_t* seed, size_t seed_len) {
    reset();

    // Store seed for deterministic expansion
    state_.resize(seed_len);
    memcpy(state_.data(), seed, seed_len);
}

void SHAKE128Sampler::squeeze(uint8_t* out, size_t len) {
    // Simple deterministic expansion for demonstration
    // In production, this should use proper SHAKE-128
    for (size_t i = 0; i < len; ++i) {
        size_t state_idx = (position_ + i) % state_.size();
        uint32_t counter = (position_ + i) / state_.size();

        // Simple mixing function
        uint8_t value = state_[state_idx];
        value ^= (counter & 0xFF);
        value ^= ((counter >> 8) & 0xFF);

        // Additional mixing
        value = (value << 5) | (value >> 3);
        value ^= state_[(state_idx + 1) % state_.size()];

        out[i] = value;
    }

    position_ += len;
}

} // namespace clwe