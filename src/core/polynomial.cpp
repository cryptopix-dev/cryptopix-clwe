#include "polynomial.hpp"
#include <cstring>
#include <algorithm>

namespace clwe {

AVXPolynomial::AVXPolynomial(uint32_t degree, uint32_t modulus, AVXNTTEngine* ntt)
    : degree_(degree), modulus_(modulus), coeffs_(nullptr), ntt_(ntt) {
    allocate_coeffs();
    set_zero();
}

AVXPolynomial::AVXPolynomial(const AVXPolynomial& other)
    : degree_(other.degree_), modulus_(other.modulus_), coeffs_(nullptr), ntt_(other.ntt_) {
    allocate_coeffs();
    memcpy(coeffs_, other.coeffs_, (degree_ / 8) * sizeof(__m256i));
}

AVXPolynomial::AVXPolynomial(AVXPolynomial&& other) noexcept
    : degree_(other.degree_), modulus_(other.modulus_), coeffs_(other.coeffs_), ntt_(other.ntt_) {
    other.coeffs_ = nullptr;
    other.degree_ = 0;
}

AVXPolynomial::~AVXPolynomial() {
    deallocate_coeffs();
}

AVXPolynomial& AVXPolynomial::operator=(const AVXPolynomial& other) {
    if (this != &other) {
        if (degree_ != other.degree_ || modulus_ != other.modulus_) {
            deallocate_coeffs();
            degree_ = other.degree_;
            modulus_ = other.modulus_;
            allocate_coeffs();
        }
        ntt_ = other.ntt_;
        memcpy(coeffs_, other.coeffs_, (degree_ / 8) * sizeof(__m256i));
    }
    return *this;
}

AVXPolynomial& AVXPolynomial::operator=(AVXPolynomial&& other) noexcept {
    if (this != &other) {
        deallocate_coeffs();
        degree_ = other.degree_;
        modulus_ = other.modulus_;
        coeffs_ = other.coeffs_;
        ntt_ = other.ntt_;
        other.coeffs_ = nullptr;
        other.degree_ = 0;
    }
    return *this;
}

void AVXPolynomial::allocate_coeffs() {
    if (degree_ > 0) {
        coeffs_ = static_cast<__m256i*>(AVXAllocator::allocate((degree_ / 8) * sizeof(__m256i)));
    }
}

void AVXPolynomial::deallocate_coeffs() {
    if (coeffs_) {
        AVXAllocator::deallocate(coeffs_);
        coeffs_ = nullptr;
    }
}

void AVXPolynomial::add_avx(const AVXPolynomial& other) {
#ifdef HAVE_AVX2
    for (uint32_t i = 0; i < degree_ / 8; ++i) {
        coeffs_[i] = _mm256_add_epi32(coeffs_[i], other.coeffs_[i]);
    }
    mod_reduce_avx();
#else
    // Fallback for non-AVX architectures
    uint32_t* coeffs = new uint32_t[degree_];
    uint32_t* other_coeffs = new uint32_t[degree_];
    copy_to(coeffs);
    other.copy_to(other_coeffs);

    for (uint32_t i = 0; i < degree_; ++i) {
        coeffs[i] = (coeffs[i] + other_coeffs[i]) % modulus_;
    }

    copy_from(coeffs);
    delete[] coeffs;
    delete[] other_coeffs;
#endif
}

void AVXPolynomial::sub_avx(const AVXPolynomial& other) {
#ifdef HAVE_AVX2
    for (uint32_t i = 0; i < degree_ / 8; ++i) {
        coeffs_[i] = _mm256_sub_epi32(coeffs_[i], other.coeffs_[i]);
    }
    mod_reduce_avx();
#else
    // Fallback for non-AVX architectures
    uint32_t* coeffs = new uint32_t[degree_];
    uint32_t* other_coeffs = new uint32_t[degree_];
    copy_to(coeffs);
    other.copy_to(other_coeffs);

    for (uint32_t i = 0; i < degree_; ++i) {
        coeffs[i] = (coeffs[i] - other_coeffs[i] + modulus_) % modulus_;
    }

    copy_from(coeffs);
    delete[] coeffs;
    delete[] other_coeffs;
#endif
}

void AVXPolynomial::scalar_mul_avx(uint32_t scalar) {
#ifdef HAVE_AVX2
    __m256i scalar_vec = _mm256_set1_epi32(scalar);
    for (uint32_t i = 0; i < degree_ / 8; ++i) {
        coeffs_[i] = _mm256_mullo_epi32(coeffs_[i], scalar_vec);
    }
    mod_reduce_avx();
#else
    // Fallback for non-AVX architectures
    uint32_t* coeffs = new uint32_t[degree_];
    copy_to(coeffs);

    for (uint32_t i = 0; i < degree_; ++i) {
        coeffs[i] = (coeffs[i] * scalar) % modulus_;
    }

    copy_from(coeffs);
    delete[] coeffs;
#endif
}

void AVXPolynomial::mod_reduce_avx() {
#ifdef HAVE_AVX2
    __m256i q_vec = _mm256_set1_epi32(modulus_);
    for (uint32_t i = 0; i < degree_ / 8; ++i) {
        __m256i mask = _mm256_cmpgt_epi32(coeffs_[i], q_vec);
        coeffs_[i] = _mm256_sub_epi32(coeffs_[i], _mm256_and_si256(mask, q_vec));
    }
#endif
    // No fallback needed - modular reduction is handled in the scalar operations
}

void AVXPolynomial::multiply_ntt_avx(const AVXPolynomial& other, AVXPolynomial& result) const {
    if (!ntt_) {
        throw std::runtime_error("NTT engine not available for polynomial multiplication");
    }
    ntt_->multiply_avx(coeffs_, other.coeffs_, result.coeffs_);
}

void AVXPolynomial::copy_from(const uint32_t* coeffs) {
#ifdef HAVE_AVX2
    for (uint32_t i = 0; i < degree_; i += 8) {
        uint32_t vals[8];
        for (int j = 0; j < 8; ++j) {
            vals[j] = (i + j < degree_) ? coeffs[i + j] : 0;
        }
        coeffs_[i/8] = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(vals));
    }
#else
    // Fallback: copy directly to our dummy AVX structure
    for (uint32_t i = 0; i < degree_; ++i) {
        uint32_t avx_index = i / 8;
        uint32_t sub_index = i % 8;
        coeffs_[avx_index].m[sub_index] = coeffs[i];
    }
#endif
}

void AVXPolynomial::copy_to(uint32_t* coeffs) const {
#ifdef HAVE_AVX2
    for (uint32_t i = 0; i < degree_; i += 8) {
        uint32_t vals[8];
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(vals), coeffs_[i/8]);
        for (int j = 0; j < 8 && i + j < degree_; ++j) {
            coeffs[i + j] = vals[j];
        }
    }
#else
    // Fallback: copy directly from our dummy AVX structure
    for (uint32_t i = 0; i < degree_; ++i) {
        uint32_t avx_index = i / 8;
        uint32_t sub_index = i % 8;
        coeffs[i] = coeffs_[avx_index].m[sub_index];
    }
#endif
}

void AVXPolynomial::set_zero() {
    memset(coeffs_, 0, (degree_ / 8) * sizeof(__m256i));
}

void AVXPolynomial::set_coeff(uint32_t index, uint32_t value) {
    if (index >= degree_) return;

#ifdef HAVE_AVX2
    uint32_t avx_index = index / 8;
    uint32_t sub_index = index % 8;

    uint32_t vals[8];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(vals), coeffs_[avx_index]);
    vals[sub_index] = value % modulus_;
    coeffs_[avx_index] = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(vals));
#else
    uint32_t avx_index = index / 8;
    uint32_t sub_index = index % 8;
    coeffs_[avx_index].m[sub_index] = value % modulus_;
#endif
}

uint32_t AVXPolynomial::infinity_norm() const {
    uint32_t max_norm = 0;
    for (uint32_t i = 0; i < degree_; ++i) {
        uint32_t coeff = 0;
#ifdef HAVE_AVX2
        // Extract coefficient (simplified - should be more efficient)
        uint32_t avx_index = i / 8;
        uint32_t sub_index = i % 8;
        uint32_t vals[8];
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(vals), coeffs_[avx_index]);
        coeff = vals[sub_index];
#else
        uint32_t avx_index = i / 8;
        uint32_t sub_index = i % 8;
        coeff = coeffs_[avx_index].m[sub_index];
#endif

        // Compute |coeff - q/2| for centered norm
        uint32_t centered = (coeff > modulus_/2) ? modulus_ - coeff : coeff;
        max_norm = std::max(max_norm, centered);
    }
    return max_norm;
}

} // namespace clwe