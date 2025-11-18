#include "sampling.hpp"
#include "shake_sampler.hpp"
#include <cstring>
#include <random>
#include <chrono>
#include <array>

#ifdef HAVE_AVX512
#include <immintrin.h>
#endif

namespace clwe {

// Global sampling functions using production-ready SHAKE256Sampler

void sample_polynomial_binomial(uint32_t* coeffs, uint32_t degree, uint32_t eta, uint32_t modulus) {
    // Use production-ready SHAKE256Sampler
    SHAKE256Sampler sampler;
    std::array<uint8_t, 32> seed;
    std::random_device rd;
    std::generate(seed.begin(), seed.end(), [&rd]() { return rd() % 256; });
    sampler.init(seed.data(), seed.size());
    sampler.sample_polynomial_binomial(coeffs, degree, eta, modulus);
}

void sample_polynomial_binomial_batch(uint32_t** coeffs_batch, uint32_t batch_size,
                                     uint32_t degree, uint32_t eta, uint32_t modulus) {
    for (uint32_t i = 0; i < batch_size; ++i) {
        sample_polynomial_binomial(coeffs_batch[i], degree, eta, modulus);
    }
}

void sample_polynomial_binomial_batch_avx512(uint32_t** coeffs_batch, uint32_t batch_size,
                                            uint32_t degree, uint32_t eta, uint32_t modulus) {
    // Use production-ready SHAKE256Sampler with AVX-512 acceleration
    SHAKE256Sampler sampler;
    std::array<uint8_t, 32> seed;
    std::random_device rd;
    std::generate(seed.begin(), seed.end(), [&rd]() { return rd() % 256; });
    sampler.init(seed.data(), seed.size());
    sampler.sample_polynomial_binomial_batch_avx512(coeffs_batch, batch_size, degree, eta, modulus);
}

} // namespace clwe