#ifndef RING_OPERATIONS_HPP
#define RING_OPERATIONS_HPP

#include "clwe.hpp"
#include "polynomial.hpp"
#include <vector>
#include <cstdint>
#include <array>

namespace clwe {

// Forward declarations
class AVXPolynomial;
class AVXNTTEngine;

// Ring operations class for CLWE cryptographic primitives
class RingOperations {
private:
    CLWEParameters params_;
    AVXNTTEngine* ntt_engine_;

    // AVX-optimized matrix operations
    void matrix_vector_mul_avx(const std::vector<std::vector<AVXPolynomial>>& A,
                              const std::vector<AVXPolynomial>& v,
                              std::vector<AVXPolynomial>& result) const;

    void matrix_transpose_vector_mul_avx(const std::vector<std::vector<AVXPolynomial>>& A,
                                        const std::vector<AVXPolynomial>& v,
                                        std::vector<AVXPolynomial>& result) const;

    void inner_product_avx(const std::vector<AVXPolynomial>& a,
                          const std::vector<AVXPolynomial>& b,
                          AVXPolynomial& result) const;

public:
    RingOperations(const CLWEParameters& params, AVXNTTEngine* ntt_engine);
    ~RingOperations();

    // Disable copy and assignment
    RingOperations(const RingOperations&) = delete;
    RingOperations& operator=(const RingOperations&) = delete;

    // Deterministic matrix A generation from seed
    std::vector<std::vector<AVXPolynomial>> generate_matrix_A(const std::array<uint8_t, 32>& seed) const;

    // Binomial sampling
    AVXPolynomial sample_binomial(uint32_t eta, const std::array<uint8_t, 32>& randomness) const;
    std::vector<AVXPolynomial> sample_binomial_batch(uint32_t eta, uint32_t count,
                                                   const std::array<uint8_t, 32>& seed) const;

    // AVX-optimized polynomial operations
    void poly_add_avx(const AVXPolynomial& a, const AVXPolynomial& b, AVXPolynomial& result) const;
    void poly_sub_avx(const AVXPolynomial& a, const AVXPolynomial& b, AVXPolynomial& result) const;
    void poly_scalar_mul_avx(const AVXPolynomial& a, uint32_t scalar, AVXPolynomial& result) const;

    // Matrix-vector operations
    std::vector<AVXPolynomial> matrix_vector_mul(const std::vector<std::vector<AVXPolynomial>>& A,
                                                const std::vector<AVXPolynomial>& v) const;
    std::vector<AVXPolynomial> matrix_transpose_vector_mul(const std::vector<std::vector<AVXPolynomial>>& A,
                                                          const std::vector<AVXPolynomial>& v) const;

    // Inner product
    AVXPolynomial inner_product(const std::vector<AVXPolynomial>& a,
                               const std::vector<AVXPolynomial>& b) const;

    // Utility functions
    AVXPolynomial encode_message_to_poly(const std::vector<uint8_t>& message) const;
    std::vector<uint8_t> decode_poly_to_message(const AVXPolynomial& poly) const;

    // Serialization
    std::vector<uint8_t> serialize_polynomial(const AVXPolynomial& poly) const;
    AVXPolynomial deserialize_polynomial(const std::vector<uint8_t>& data) const;

    // Getters
    const CLWEParameters& params() const { return params_; }
};

} // namespace clwe

#endif // RING_OPERATIONS_HPP