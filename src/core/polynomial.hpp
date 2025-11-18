#ifndef POLYNOMIAL_HPP
#define POLYNOMIAL_HPP

#include "utils.hpp"
#include "ntt_avx.hpp"
#include <cstdint>
#include <vector>

namespace clwe {

class AVXPolynomial {
private:
    uint32_t degree_;
    uint32_t modulus_;
    __m256i* coeffs_;      // AVX-aligned coefficient array
    AVXNTTEngine* ntt_;    // NTT engine for multiplication

public:
    AVXPolynomial(uint32_t degree, uint32_t modulus, AVXNTTEngine* ntt = nullptr);
    AVXPolynomial(const AVXPolynomial& other);
    AVXPolynomial(AVXPolynomial&& other) noexcept;
    ~AVXPolynomial();

    // Assignment operators
    AVXPolynomial& operator=(const AVXPolynomial& other);
    AVXPolynomial& operator=(AVXPolynomial&& other) noexcept;

    // AVX vectorized operations
    void add_avx(const AVXPolynomial& other);
    void sub_avx(const AVXPolynomial& other);
    void scalar_mul_avx(uint32_t scalar);
    void mod_reduce_avx();

    // NTT-based multiplication
    void multiply_ntt_avx(const AVXPolynomial& other, AVXPolynomial& result) const;

    // Utility functions
    void copy_from(const uint32_t* coeffs);
    void copy_to(uint32_t* coeffs) const;
    void set_zero();
    void set_coeff(uint32_t index, uint32_t value);

    uint32_t infinity_norm() const;
    uint32_t degree() const { return degree_; }
    uint32_t modulus() const { return modulus_; }

    // Access to AVX coefficients (for NTT operations)
    __m256i* avx_coeffs() { return coeffs_; }
    const __m256i* avx_coeffs() const { return coeffs_; }

private:
    void allocate_coeffs();
    void deallocate_coeffs();
};

} // namespace clwe

#endif // POLYNOMIAL_HPP