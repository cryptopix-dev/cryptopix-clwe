#include "ntt_avx.hpp"
#include <algorithm>
#include <iostream>
#include <cstring>
#include <unordered_map>

#ifdef HAVE_AVX2
#include <immintrin.h>
#endif

#ifdef HAVE_AVX512
#include <immintrin.h>
#endif

uint32_t mod_pow(uint32_t base, uint32_t exp, uint32_t mod) {
    uint32_t result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1) {
            result = (static_cast<uint64_t>(result) * base) % mod;
        }
        base = (static_cast<uint64_t>(base) * base) % mod;
        exp >>= 1;
    }
    return result;
}

namespace clwe {

AVXNTTEngine::AVXNTTEngine(uint32_t q, uint32_t n)
    : q_(q), n_(n), log_n_(bit_length(n) - 1),
      zetas_(nullptr), zetas_inv_(nullptr), bitrev_(nullptr),
      zetas_avx512_(nullptr), zetas_inv_avx512_(nullptr),
      montgomery_r_(0), montgomery_r_inv_(0) {

    if (!is_power_of_two(n)) {
        throw std::invalid_argument("NTT degree must be a power of 2");
    }

    montgomery_r_ = (1ULL << 32) % q_;
    montgomery_r_inv_ = mod_inverse(montgomery_r_, q_);

#ifdef HAVE_AVX512
    size_t avx512_count = n / 16;
    zetas_avx512_ = static_cast<__m512i*>(AVXAllocator::allocate(avx512_count * sizeof(__m512i)));
    zetas_inv_avx512_ = static_cast<__m512i*>(AVXAllocator::allocate(avx512_count * sizeof(__m512i)));
#endif

#ifdef HAVE_AVX2
    size_t avx_count = n / 8;
    zetas_ = static_cast<__m256i*>(AVXAllocator::allocate(avx_count * sizeof(__m256i)));
    zetas_inv_ = static_cast<__m256i*>(AVXAllocator::allocate(avx_count * sizeof(__m256i)));
#else
    zetas_ = static_cast<__m256i*>(AVXAllocator::allocate(n * sizeof(uint32_t)));
    zetas_inv_ = static_cast<__m256i*>(AVXAllocator::allocate(n * sizeof(uint32_t)));
#endif

    bitrev_ = static_cast<uint32_t*>(AVXAllocator::allocate(n * sizeof(uint32_t)));

    if ((!zetas_ || !zetas_inv_ || !bitrev_)
#ifdef HAVE_AVX512
        || (!zetas_avx512_ || !zetas_inv_avx512_)
#endif
        ) {
        throw std::bad_alloc();
    }

    precompute_zetas();
    precompute_bitrev();
}

AVXNTTEngine::~AVXNTTEngine() {
    if (zetas_) AVXAllocator::deallocate(zetas_);
    if (zetas_inv_) AVXAllocator::deallocate(zetas_inv_);
    if (bitrev_) AVXAllocator::deallocate(bitrev_);
#ifdef HAVE_AVX512
    if (zetas_avx512_) AVXAllocator::deallocate(zetas_avx512_);
    if (zetas_inv_avx512_) AVXAllocator::deallocate(zetas_inv_avx512_);
#endif
}

void AVXNTTEngine::precompute_zetas() {
    uint32_t g = 3;
    uint32_t zeta = mod_pow(g, (q_ - 1) / n_, q_);

    uint32_t* zetas = reinterpret_cast<uint32_t*>(zetas_);
    zetas[0] = 1;
    for (uint32_t i = 1; i < n_; ++i) {
        zetas[i] = (static_cast<uint64_t>(zetas[i-1]) * zeta) % q_;
    }

    uint32_t zeta_inv = mod_inverse(zeta, q_);
    uint32_t* zetas_inv = reinterpret_cast<uint32_t*>(zetas_inv_);
    zetas_inv[0] = 1;
    for (uint32_t i = 1; i < n_; ++i) {
        zetas_inv[i] = (static_cast<uint64_t>(zetas_inv[i-1]) * zeta_inv) % q_;
    }

#ifdef HAVE_AVX512
    uint32_t* zetas_avx512 = reinterpret_cast<uint32_t*>(zetas_avx512_);
    uint32_t* zetas_inv_avx512 = reinterpret_cast<uint32_t*>(zetas_inv_avx512_);

    for (uint32_t i = 0; i < n_; ++i) {
        zetas_avx512[i] = zetas[i];
        zetas_inv_avx512[i] = zetas_inv[i];
    }
#endif
}

void AVXNTTEngine::precompute_bitrev() {
    for (uint32_t i = 0; i < n_; ++i) {
        uint32_t rev = 0;
        for (uint32_t j = 0; j < log_n_; ++j) {
            rev |= ((i >> j) & 1) << (log_n_ - 1 - j);
        }
        bitrev_[i] = rev;
    }
}

void AVXNTTEngine::butterfly_avx(__m256i& a, __m256i& b, __m256i zeta) const {
    uint32_t* a_vals = reinterpret_cast<uint32_t*>(&a);
    uint32_t* b_vals = reinterpret_cast<uint32_t*>(&b);
    uint32_t* zeta_vals = reinterpret_cast<uint32_t*>(&zeta);

    for (int i = 0; i < 8; ++i) {
        uint32_t sum = (a_vals[i] + b_vals[i]) % q_;
        uint32_t diff = (a_vals[i] >= b_vals[i]) ? (a_vals[i] - b_vals[i]) : (a_vals[i] + q_ - b_vals[i]);
        uint32_t prod = (static_cast<uint64_t>(diff) * zeta_vals[i]) % q_;
        a_vals[i] = sum;
        b_vals[i] = prod;
    }
}

void AVXNTTEngine::butterfly_inv_avx(__m256i& a, __m256i& b, __m256i zeta) const {
    butterfly_avx(a, b, zeta);
}

__m256i AVXNTTEngine::mod_reduce_avx(__m256i val) const {
    uint32_t* vals = reinterpret_cast<uint32_t*>(&val);
    for (int i = 0; i < 8; ++i) {
        if (vals[i] >= q_) {
            vals[i] -= q_;
        }
    }
    return val;
}

void AVXNTTEngine::butterfly_avx512(avx512_int& a, avx512_int& b, avx512_int zeta) const {
#ifdef HAVE_AVX512
    avx512_int sum = _mm512_add_epi32(a, b);
    avx512_int diff = _mm512_sub_epi32(a, b);
    avx512_int prod = _mm512_mullo_epi32(diff, zeta);
    prod = mod_reduce_avx512(prod);
    a = sum;
    b = prod;
#else
    uint32_t* a_vals = reinterpret_cast<uint32_t*>(&a);
    uint32_t* b_vals = reinterpret_cast<uint32_t*>(&b);
    uint32_t* zeta_vals = reinterpret_cast<uint32_t*>(&zeta);

    for (int i = 0; i < 16; ++i) {
        uint32_t sum = (a_vals[i] + b_vals[i]) % q_;
        uint32_t diff = (a_vals[i] >= b_vals[i]) ? (a_vals[i] - b_vals[i]) : (a_vals[i] + q_ - b_vals[i]);
        uint32_t prod = (static_cast<uint64_t>(diff) * zeta_vals[i]) % q_;
        a_vals[i] = sum;
        b_vals[i] = prod;
    }
#endif
}

void AVXNTTEngine::butterfly_inv_avx512(avx512_int& a, avx512_int& b, avx512_int zeta) const {
    butterfly_avx512(a, b, zeta);
}

avx512_int AVXNTTEngine::mod_reduce_avx512(avx512_int val) const {
#ifdef HAVE_AVX512
    return montgomery_reduce_avx512(val);
#else
    uint32_t* vals = reinterpret_cast<uint32_t*>(&val);
    for (int i = 0; i < 16; ++i) {
        if (vals[i] >= q_) {
            vals[i] -= q_;
        }
    }
    return val;
#endif
}

uint32_t AVXNTTEngine::montgomery_reduce(uint64_t val) const {
    uint64_t t = val * montgomery_r_inv_;
    uint32_t k = t % (1ULL << 32);
    uint64_t res = val - static_cast<uint64_t>(k) * q_;
    return res >> 32;
}

__m256i AVXNTTEngine::montgomery_reduce_avx(__m256i val) const {
#ifdef HAVE_AVX2
    __m256i r_inv_vec = _mm256_set1_epi32(montgomery_r_inv_);
    __m256i prod = _mm256_mullo_epi32(val, r_inv_vec);
    __m256i k = _mm256_and_si256(prod, _mm256_set1_epi32(0xFFFFFFFF));
    __m256i q_vec = _mm256_set1_epi32(q_);
    __m256i kq = _mm256_mullo_epi32(k, q_vec);
    __m256i diff = _mm256_sub_epi32(val, kq);
    return _mm256_srli_epi32(diff, 32);
#else
    uint32_t* vals = reinterpret_cast<uint32_t*>(&val);
    for (int i = 0; i < 8; ++i) {
        vals[i] = montgomery_reduce(vals[i]);
    }
    return val;
#endif
}

avx512_int AVXNTTEngine::montgomery_reduce_avx512(avx512_int val) const {
#ifdef HAVE_AVX512
    avx512_int r_inv_vec = _mm512_set1_epi32(montgomery_r_inv_);
    avx512_int prod = _mm512_mullo_epi32(val, r_inv_vec);
    avx512_int k = _mm512_and_epi32(prod, _mm512_set1_epi32(0xFFFFFFFF));
    avx512_int q_vec = _mm512_set1_epi32(q_);
    avx512_int kq = _mm512_mullo_epi32(k, q_vec);
    avx512_int diff = _mm512_sub_epi32(val, kq);
    return _mm512_srli_epi32(diff, 32);
#else
    uint32_t* vals = reinterpret_cast<uint32_t*>(&val);
    for (int i = 0; i < 16; ++i) {
        vals[i] = montgomery_reduce(vals[i]);
    }
    return val;
#endif
}

void AVXNTTEngine::ntt_forward_avx(__m256i* poly) const {
    uint32_t* zetas = reinterpret_cast<uint32_t*>(zetas_);
    uint32_t m = 1;
    uint32_t k = n_ / 2;

    for (uint32_t stage = 0; stage < log_n_; ++stage) {
        uint32_t j = 0;
        for (uint32_t i = 0; i < k; ++i) {
            uint32_t zeta = zetas[j];
            __m256i zeta_vec;
            uint32_t* zeta_ptr = reinterpret_cast<uint32_t*>(&zeta_vec);
            for (int x = 0; x < 8; ++x) zeta_ptr[x] = zeta;
            butterfly_avx(poly[i], poly[i + k], zeta_vec);
            j += m;
        }
        m *= 2;
        k /= 2;
    }
}

void AVXNTTEngine::ntt_inverse_avx(__m256i* poly) const {
    uint32_t* zetas_inv = reinterpret_cast<uint32_t*>(zetas_inv_);
    uint32_t m = n_ / 2;
    uint32_t k = 1;

    for (uint32_t stage = 0; stage < log_n_; ++stage) {
        uint32_t j = 0;
        for (uint32_t i = 0; i < k; ++i) {
            uint32_t zeta = zetas_inv[j];
            __m256i zeta_vec;
            uint32_t* zeta_ptr = reinterpret_cast<uint32_t*>(&zeta_vec);
            for (int x = 0; x < 8; ++x) zeta_ptr[x] = zeta;
            butterfly_inv_avx(poly[i], poly[i + k], zeta_vec);
            j += m;
        }
        m /= 2;
        k *= 2;
    }

    uint32_t n_inv = mod_inverse(n_, q_);
    for (uint32_t i = 0; i < n_/8; ++i) {
        uint32_t* poly_ptr = reinterpret_cast<uint32_t*>(&poly[i]);
        for (int x = 0; x < 8; ++x) {
            poly_ptr[x] = (static_cast<uint64_t>(poly_ptr[x]) * n_inv) % q_;
        }
    }
}

void AVXNTTEngine::ntt_forward_avx512(avx512_int* poly) const {
#ifdef HAVE_AVX512
    uint32_t m = 1;
    uint32_t k = n_ / 2;

    for (uint32_t stage = 0; stage < log_n_; ++stage) {
        uint32_t j = 0;
        for (uint32_t i = 0; i < k; i += 16) {
            avx512_int zeta_vec = _mm512_loadu_si512(&zetas_avx512_[j]);
            butterfly_avx512(poly[i/16], poly[(i + k)/16], zeta_vec);
            j += m;
        }
        m *= 2;
        k /= 2;
    }
#else
    ntt_forward_avx(reinterpret_cast<__m256i*>(poly));
#endif
}

void AVXNTTEngine::ntt_inverse_avx512(avx512_int* poly) const {
#ifdef HAVE_AVX512
    uint32_t m = n_ / 2;
    uint32_t k = 1;

    for (uint32_t stage = 0; stage < log_n_; ++stage) {
        uint32_t j = 0;
        for (uint32_t i = 0; i < k; i += 16) {
            avx512_int zeta_vec = _mm512_loadu_si512(&zetas_inv_avx512_[j]);
            butterfly_inv_avx512(poly[i/16], poly[(i + k)/16], zeta_vec);
            j += m;
        }
        m /= 2;
        k *= 2;
    }

    avx512_int n_inv_vec = _mm512_set1_epi32(mod_inverse(n_, q_));
    for (uint32_t i = 0; i < n_/16; ++i) {
        poly[i] = _mm512_mullo_epi32(poly[i], n_inv_vec);
        poly[i] = mod_reduce_avx512(poly[i]);
    }
#else
    ntt_inverse_avx(reinterpret_cast<__m256i*>(poly));
#endif
}

void AVXNTTEngine::multiply_avx(const __m256i* a, const __m256i* b, __m256i* result) const {
    __m256i* a_ntt = static_cast<__m256i*>(AVXAllocator::allocate(n_/8 * sizeof(__m256i)));
    __m256i* b_ntt = static_cast<__m256i*>(AVXAllocator::allocate(n_/8 * sizeof(__m256i)));

    memcpy(a_ntt, a, n_/8 * sizeof(__m256i));
    memcpy(b_ntt, b, n_/8 * sizeof(__m256i));

    ntt_forward_avx(a_ntt);
    ntt_forward_avx(b_ntt);

    for (uint32_t i = 0; i < n_/8; ++i) {
        uint32_t* a_ptr = reinterpret_cast<uint32_t*>(&a_ntt[i]);
        uint32_t* b_ptr = reinterpret_cast<uint32_t*>(&b_ntt[i]);
        uint32_t* r_ptr = reinterpret_cast<uint32_t*>(&result[i]);
        for (int j = 0; j < 8; ++j) {
            r_ptr[j] = montgomery_reduce(static_cast<uint64_t>(a_ptr[j]) * b_ptr[j]);
        }
    }

    ntt_inverse_avx(result);

    AVXAllocator::deallocate(a_ntt);
    AVXAllocator::deallocate(b_ntt);
}

void AVXNTTEngine::multiply_avx512(const avx512_int* a, const avx512_int* b, avx512_int* result) const {
#ifdef HAVE_AVX512
    avx512_int* a_ntt = static_cast<avx512_int*>(AVXAllocator::allocate(n_/16 * sizeof(avx512_int)));
    avx512_int* b_ntt = static_cast<avx512_int*>(AVXAllocator::allocate(n_/16 * sizeof(avx512_int)));

    memcpy(a_ntt, a, n_/16 * sizeof(avx512_int));
    memcpy(b_ntt, b, n_/16 * sizeof(avx512_int));

    ntt_forward_avx512(a_ntt);
    ntt_forward_avx512(b_ntt);

    for (uint32_t i = 0; i < n_/16; ++i) {
        result[i] = _mm512_mullo_epi32(a_ntt[i], b_ntt[i]);
        result[i] = montgomery_reduce_avx512(result[i]);
    }

    ntt_inverse_avx512(result);

    AVXAllocator::deallocate(a_ntt);
    AVXAllocator::deallocate(b_ntt);
#else
    multiply_avx(reinterpret_cast<const __m256i*>(a), reinterpret_cast<const __m256i*>(b),
                 reinterpret_cast<__m256i*>(result));
#endif
}

void AVXNTTEngine::bit_reverse_avx(__m256i* poly) const {
    AVXVector<__m256i> temp(n_/8);
    for (uint32_t i = 0; i < n_; ++i) {
        uint32_t rev_i = bitrev_[i];
        if (i < rev_i) {
            std::swap(poly[i/8], temp[rev_i/8]);
        }
    }
}

void AVXNTTEngine::copy_from_uint32(const uint32_t* coeffs, __m256i* avx_coeffs) const {
    for (uint32_t i = 0; i < n_; i += 8) {
        uint32_t* dest = reinterpret_cast<uint32_t*>(&avx_coeffs[i/8]);
        for (int j = 0; j < 8; ++j) {
            dest[j] = coeffs[i + j];
        }
    }
}

void AVXNTTEngine::copy_to_uint32(const __m256i* avx_coeffs, uint32_t* coeffs) const {
    for (uint32_t i = 0; i < n_; i += 8) {
        const uint32_t* src = reinterpret_cast<const uint32_t*>(&avx_coeffs[i/8]);
        for (int j = 0; j < 8; ++j) {
            coeffs[i + j] = src[j];
        }
    }
}

void AVXNTTEngine::bit_reverse_avx512(avx512_int* poly) const {
    AVXVector<avx512_int> temp(n_/16);
    for (uint32_t i = 0; i < n_; ++i) {
        uint32_t rev_i = bitrev_[i];
        if (i < rev_i) {
            std::swap(poly[i/16], temp[rev_i/16]);
        }
    }
}

void AVXNTTEngine::copy_from_uint32_avx512(const uint32_t* coeffs, avx512_int* avx512_coeffs) const {
    for (uint32_t i = 0; i < n_; i += 16) {
        uint32_t vals[16];
        for (int j = 0; j < 16; ++j) {
            vals[j] = (i + j < n_) ? coeffs[i + j] : 0;
        }
#ifdef HAVE_AVX512
        avx512_coeffs[i/16] = _mm512_loadu_si512(vals);
#else
        memcpy(&avx512_coeffs[i/16], vals, 16 * sizeof(uint32_t));
#endif
    }
}

void AVXNTTEngine::copy_to_uint32_avx512(const avx512_int* avx512_coeffs, uint32_t* coeffs) const {
    for (uint32_t i = 0; i < n_; i += 16) {
        uint32_t vals[16];
#ifdef HAVE_AVX512
        _mm512_storeu_si512(vals, avx512_coeffs[i/16]);
#else
        memcpy(vals, &avx512_coeffs[i/16], 16 * sizeof(uint32_t));
#endif
        for (int j = 0; j < 16 && i + j < n_; ++j) {
            coeffs[i + j] = vals[j];
        }
    }
}

} // namespace clwe