#ifndef NTT_AVX_HPP
#define NTT_AVX_HPP

#include "utils.hpp"
#include <cstdint>
#include <vector>

#ifdef HAVE_AVX512
#include <immintrin.h>
typedef __m512i avx512_int;
#else
typedef __m256i avx512_int;
#endif

namespace clwe {

class AVXNTTEngine {
private:
    uint32_t q_;
    uint32_t n_;
    uint32_t log_n_;
    __m256i* zetas_;
    __m256i* zetas_inv_;
    uint32_t* bitrev_;

    avx512_int* zetas_avx512_;
    avx512_int* zetas_inv_avx512_;

    uint32_t montgomery_r_;
    uint32_t montgomery_r_inv_;

    void precompute_zetas();
    void precompute_bitrev();

    void butterfly_avx(__m256i& a, __m256i& b, __m256i zeta) const;
    void butterfly_inv_avx(__m256i& a, __m256i& b, __m256i zeta) const;

    void butterfly_avx512(avx512_int& a, avx512_int& b, avx512_int zeta) const;
    void butterfly_inv_avx512(avx512_int& a, avx512_int& b, avx512_int zeta) const;

    __m256i mod_reduce_avx(__m256i val) const;
    avx512_int mod_reduce_avx512(avx512_int val) const;

    uint32_t montgomery_reduce(uint64_t val) const;
    __m256i montgomery_reduce_avx(__m256i val) const;
    avx512_int montgomery_reduce_avx512(avx512_int val) const;

public:
    AVXNTTEngine(uint32_t q, uint32_t n);
    ~AVXNTTEngine();

    bool has_avx512() const {
#ifdef HAVE_AVX512
        return true;
#else
        return false;
#endif
    }

    AVXNTTEngine(const AVXNTTEngine&) = delete;
    AVXNTTEngine& operator=(const AVXNTTEngine&) = delete;

    void ntt_forward_avx(__m256i* poly) const;
    void ntt_inverse_avx(__m256i* poly) const;

    void ntt_forward_avx512(avx512_int* poly) const;
    void ntt_inverse_avx512(avx512_int* poly) const;

    void multiply_avx(const __m256i* a, const __m256i* b, __m256i* result) const;
    void multiply_avx512(const avx512_int* a, const avx512_int* b, avx512_int* result) const;

    void bit_reverse_avx(__m256i* poly) const;
    void bit_reverse_avx512(avx512_int* poly) const;
    void copy_from_uint32(const uint32_t* coeffs, __m256i* avx_coeffs) const;
    void copy_to_uint32(const __m256i* avx_coeffs, uint32_t* coeffs) const;
    void copy_from_uint32_avx512(const uint32_t* coeffs, avx512_int* avx512_coeffs) const;
    void copy_to_uint32_avx512(const avx512_int* avx512_coeffs, uint32_t* coeffs) const;

    uint32_t modulus() const { return q_; }
    uint32_t degree() const { return n_; }
    uint32_t log_degree() const { return log_n_; }
};

} // namespace clwe

#endif // NTT_AVX_HPP