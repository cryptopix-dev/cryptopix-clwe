#include "clwe/ring_operations.hpp"
#include "polynomial.hpp"
#include "ntt_avx.hpp"
#include "utils.hpp"
#include <cstring>
#include <algorithm>
#include <random>
#include <array>

namespace clwe {

RingOperations::RingOperations(const CLWEParameters& params, AVXNTTEngine* ntt_engine)
    : params_(params), ntt_engine_(ntt_engine) {
    if (!ntt_engine_) {
        throw std::invalid_argument("NTT engine cannot be null");
    }
}

RingOperations::~RingOperations() = default;

// Simple deterministic hash function for seed expansion
uint32_t simple_hash(const uint8_t* data, size_t len, uint32_t counter) {
    uint32_t hash = 0x9e3779b9;  // Golden ratio constant
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash = (hash << 5) | (hash >> 27);  // Rotate left by 5
        hash += counter;
    }
    return hash;
}

// Deterministic matrix A generation from seed
std::vector<std::vector<AVXPolynomial>> RingOperations::generate_matrix_A(const std::array<uint8_t, 32>& seed) const {
    uint32_t k = params_.module_rank;
    uint32_t d = params_.degree;
    uint32_t q = params_.modulus;

    // Generate matrix
    std::vector<std::vector<AVXPolynomial>> matrix(k, std::vector<AVXPolynomial>(k, AVXPolynomial(d, q, ntt_engine_)));

    for (uint32_t i = 0; i < k; ++i) {
        for (uint32_t j = 0; j < k; ++j) {
            std::vector<uint32_t> coeffs(d);
            for (uint32_t c = 0; c < d; ++c) {
                // Use seed + position as input to hash
                uint32_t counter = (i * k * d) + (j * d) + c;
                coeffs[c] = simple_hash(seed.data(), seed.size(), counter) % q;
            }
            matrix[i][j].copy_from(coeffs.data());
        }
    }

    return matrix;
}

// Binomial sampling implementation
AVXPolynomial RingOperations::sample_binomial(uint32_t eta, const std::array<uint8_t, 32>& randomness) const {
    AVXPolynomial result(params_.degree, params_.modulus, ntt_engine_);

    std::vector<uint32_t> coeffs(params_.degree);

    for (uint32_t i = 0; i < params_.degree; ++i) {
        int32_t a = 0, b = 0;

        // Generate pseudo-random bits using simple hash
        for (uint32_t e = 0; e < eta; ++e) {
            uint32_t hash_a = simple_hash(randomness.data(), randomness.size(), (i << 16) | (e << 8) | 0);
            a += (hash_a >> 31) & 1;  // Use MSB as bit
        }

        for (uint32_t e = 0; e < eta; ++e) {
            uint32_t hash_b = simple_hash(randomness.data(), randomness.size(), (i << 16) | (e << 8) | 1);
            b += (hash_b >> 31) & 1;  // Use MSB as bit
        }

        int32_t coeff = a - b;
        coeffs[i] = (coeff % static_cast<int32_t>(params_.modulus) + params_.modulus) % params_.modulus;
    }

    result.copy_from(coeffs.data());
    return result;
}

std::vector<AVXPolynomial> RingOperations::sample_binomial_batch(uint32_t eta, uint32_t count,
                                                                 const std::array<uint8_t, 32>& seed) const {
    std::vector<AVXPolynomial> result;
    result.reserve(count);

    // Use AVX-512 accelerated batch sampling if available
#ifdef HAVE_AVX512
    if (count >= 4) {  // Only use batch sampling for larger batches
        // Prepare batch of coefficient arrays
        std::vector<uint32_t*> coeffs_batch(count);
        std::vector<std::vector<uint32_t>> coeffs_data(count, std::vector<uint32_t>(params_.degree));

        for (uint32_t i = 0; i < count; ++i) {
            coeffs_batch[i] = coeffs_data[i].data();
        }

        // Create sampler and sample batch
        SHAKE256Sampler sampler;
        sampler.init(seed.data(), seed.size());
        sampler.sample_polynomial_binomial_batch_avx512(coeffs_batch.data(), count,
                                                       params_.degree, eta, params_.modulus);

        // Convert to polynomials
        for (uint32_t i = 0; i < count; ++i) {
            AVXPolynomial poly(params_.degree, params_.modulus, ntt_engine_);
            poly.copy_from(coeffs_data[i].data());
            result.push_back(std::move(poly));
        }
        return result;
    }
#endif

    // Fallback to individual sampling
    for (uint32_t i = 0; i < count; ++i) {
        std::array<uint8_t, 32> derived_seed;
        std::copy(seed.begin(), seed.end(), derived_seed.begin());

        // Derive unique seed for each polynomial
        derived_seed[0] ^= static_cast<uint8_t>(i);
        derived_seed[1] ^= static_cast<uint8_t>(i >> 8);

        result.push_back(sample_binomial(eta, derived_seed));
    }

    return result;
}

// AVX-optimized polynomial operations
void RingOperations::poly_add_avx(const AVXPolynomial& a, const AVXPolynomial& b, AVXPolynomial& result) const {
    result.add_avx(a);
    result.add_avx(b);
}

void RingOperations::poly_sub_avx(const AVXPolynomial& a, const AVXPolynomial& b, AVXPolynomial& result) const {
    result = a;
    result.sub_avx(b);
}

void RingOperations::poly_scalar_mul_avx(const AVXPolynomial& a, uint32_t scalar, AVXPolynomial& result) const {
    result = a;
    result.scalar_mul_avx(scalar);
}

// AVX-optimized matrix-vector multiplication
void RingOperations::matrix_vector_mul_avx(const std::vector<std::vector<AVXPolynomial>>& A,
                                          const std::vector<AVXPolynomial>& v,
                                          std::vector<AVXPolynomial>& result) const {
    uint32_t k = params_.module_rank;

    for (uint32_t i = 0; i < k; ++i) {
        result[i].set_zero();
        for (uint32_t j = 0; j < k; ++j) {
            AVXPolynomial product(params_.degree, params_.modulus, ntt_engine_);
            A[i][j].multiply_ntt_avx(v[j], product);
            result[i].add_avx(product);
        }
        result[i].mod_reduce_avx();
    }
}

std::vector<AVXPolynomial> RingOperations::matrix_vector_mul(const std::vector<std::vector<AVXPolynomial>>& A,
                                                           const std::vector<AVXPolynomial>& v) const {
    uint32_t k = params_.module_rank;
    std::vector<AVXPolynomial> result(k, AVXPolynomial(params_.degree, params_.modulus, ntt_engine_));
    matrix_vector_mul_avx(A, v, result);
    return result;
}

// AVX-optimized matrix transpose-vector multiplication
void RingOperations::matrix_transpose_vector_mul_avx(const std::vector<std::vector<AVXPolynomial>>& A,
                                                   const std::vector<AVXPolynomial>& v,
                                                   std::vector<AVXPolynomial>& result) const {
    uint32_t k = params_.module_rank;

    for (uint32_t i = 0; i < k; ++i) {
        result[i].set_zero();
        for (uint32_t j = 0; j < k; ++j) {
            AVXPolynomial product(params_.degree, params_.modulus, ntt_engine_);
            A[j][i].multiply_ntt_avx(v[j], product);
            result[i].add_avx(product);
        }
        result[i].mod_reduce_avx();
    }
}

std::vector<AVXPolynomial> RingOperations::matrix_transpose_vector_mul(const std::vector<std::vector<AVXPolynomial>>& A,
                                                                     const std::vector<AVXPolynomial>& v) const {
    uint32_t k = params_.module_rank;
    std::vector<AVXPolynomial> result(k, AVXPolynomial(params_.degree, params_.modulus, ntt_engine_));
    matrix_transpose_vector_mul_avx(A, v, result);
    return result;
}

// AVX-optimized inner product
void RingOperations::inner_product_avx(const std::vector<AVXPolynomial>& a,
                                      const std::vector<AVXPolynomial>& b,
                                      AVXPolynomial& result) const {
    result.set_zero();
    uint32_t k = params_.module_rank;

    for (uint32_t i = 0; i < k; ++i) {
        AVXPolynomial product(params_.degree, params_.modulus, ntt_engine_);
        a[i].multiply_ntt_avx(b[i], product);
        result.add_avx(product);
    }
    result.mod_reduce_avx();
}

AVXPolynomial RingOperations::inner_product(const std::vector<AVXPolynomial>& a,
                                          const std::vector<AVXPolynomial>& b) const {
    AVXPolynomial result(params_.degree, params_.modulus, ntt_engine_);
    inner_product_avx(a, b, result);
    return result;
}

// Message encoding/decoding
AVXPolynomial RingOperations::encode_message_to_poly(const std::vector<uint8_t>& message) const {
    AVXPolynomial result(params_.degree, params_.modulus, ntt_engine_);
    std::vector<uint32_t> coeffs(params_.degree, 0);

    size_t msg_len = std::min(message.size(), static_cast<size_t>(params_.degree));
    for (size_t i = 0; i < msg_len; ++i) {
        coeffs[i] = message[i] % params_.modulus;
    }

    result.copy_from(coeffs.data());
    return result;
}

std::vector<uint8_t> RingOperations::decode_poly_to_message(const AVXPolynomial& poly) const {
    std::vector<uint32_t> coeffs(params_.degree);
    poly.copy_to(coeffs.data());

    std::vector<uint8_t> message;
    for (uint32_t coeff : coeffs) {
        if (coeff != 0 || !message.empty()) {
            message.push_back(coeff % 256);
        }
        if (message.size() >= 32) break;  // Limit to 32 bytes like Python
    }
    return message;
}

// Serialization
std::vector<uint8_t> RingOperations::serialize_polynomial(const AVXPolynomial& poly) const {
    std::vector<uint32_t> coeffs(params_.degree);
    poly.copy_to(coeffs.data());

    std::vector<uint8_t> data;
    data.reserve(params_.degree * 4);
    for (uint32_t coeff : coeffs) {
        // Big-endian serialization
        data.push_back((coeff >> 24) & 0xFF);
        data.push_back((coeff >> 16) & 0xFF);
        data.push_back((coeff >> 8) & 0xFF);
        data.push_back(coeff & 0xFF);
    }
    return data;
}

AVXPolynomial RingOperations::deserialize_polynomial(const std::vector<uint8_t>& data) const {
    AVXPolynomial result(params_.degree, params_.modulus, ntt_engine_);
    std::vector<uint32_t> coeffs(params_.degree, 0);

    size_t max_coeffs = std::min(data.size() / 4, static_cast<size_t>(params_.degree));
    for (size_t i = 0; i < max_coeffs; ++i) {
        size_t offset = i * 4;
        if (offset + 3 < data.size()) {
            coeffs[i] = (static_cast<uint32_t>(data[offset]) << 24) |
                       (static_cast<uint32_t>(data[offset + 1]) << 16) |
                       (static_cast<uint32_t>(data[offset + 2]) << 8) |
                       static_cast<uint32_t>(data[offset + 3]);
        }
    }

    result.copy_from(coeffs.data());
    return result;
}

} // namespace clwe