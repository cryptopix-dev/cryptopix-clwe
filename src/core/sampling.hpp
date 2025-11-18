#ifndef SAMPLING_HPP
#define SAMPLING_HPP

#include <cstdint>

// Forward declaration for SHAKE256Sampler
namespace clwe {
class SHAKE256Sampler;
}

namespace clwe {

// Global sampling functions using SHAKE256Sampler
void sample_polynomial_binomial(uint32_t* coeffs, uint32_t degree, uint32_t eta, uint32_t modulus);
void sample_polynomial_binomial_batch(uint32_t** coeffs_batch, uint32_t batch_size,
                                     uint32_t degree, uint32_t eta, uint32_t modulus);
void sample_polynomial_binomial_batch_avx512(uint32_t** coeffs_batch, uint32_t batch_size,
                                            uint32_t degree, uint32_t eta, uint32_t modulus);

} // namespace clwe

#endif // SAMPLING_HPP