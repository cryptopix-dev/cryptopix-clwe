#ifndef SHAKE_SAMPLER_HPP
#define SHAKE_SAMPLER_HPP

#include <cstdint>
#include <vector>
#include <array>

namespace clwe {

// SHAKE-128 based sampler for matrix generation
class SHAKE128Sampler {
private:
    std::vector<uint8_t> state_;
    size_t position_;

    // SHAKE-128 internal state
    void reset();

public:
    SHAKE128Sampler();
    ~SHAKE128Sampler();

    // Initialize with seed
    void init(const uint8_t* seed, size_t seed_len);

    // Squeeze bytes from SHAKE-128
    void squeeze(uint8_t* out, size_t len);
};

// SHAKE-256 based sampler for Kyber/ML-KEM
class SHAKE256Sampler {
private:
    std::vector<uint8_t> state_;
    size_t position_;

    // SHAKE-256 internal state
    void absorb(const uint8_t* data, size_t len);
    void squeeze(uint8_t* out, size_t len);
    void reset();

public:
    SHAKE256Sampler();
    ~SHAKE256Sampler();

    // Initialize with seed
    void init(const uint8_t* seed, size_t seed_len);

    // Sample a single coefficient from centered binomial distribution
    int32_t sample_binomial_coefficient(uint32_t eta);

    // Sample polynomial with binomial distribution
    void sample_polynomial_binomial(uint32_t* coeffs, size_t degree, uint32_t eta, uint32_t modulus);

    // Batch sampling for efficiency
    void sample_polynomial_binomial_batch(uint32_t** coeffs_batch, size_t count,
                                        size_t degree, uint32_t eta, uint32_t modulus);

    // AVX-512 accelerated batch sampling
    void sample_polynomial_binomial_batch_avx512(uint32_t** coeffs_batch, size_t count,
                                                size_t degree, uint32_t eta, uint32_t modulus);

    // Sample from uniform distribution [0, modulus)
    uint32_t sample_uniform(uint32_t modulus);

    // Sample polynomial from uniform distribution
    void sample_polynomial_uniform(uint32_t* coeffs, size_t degree, uint32_t modulus);

    // Generate random bytes
    void random_bytes(uint8_t* out, size_t len);
};

} // namespace clwe

#endif // SHAKE_SAMPLER_HPP