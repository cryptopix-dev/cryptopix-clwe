#ifndef KEM_HPP
#define KEM_HPP

#include "clwe.hpp"
#include "ring_operations.hpp"
#include <vector>
#include <array>
#include <memory>
#include <unordered_map>
#include <string>

namespace clwe {

// Forward declarations
class RingOperations;

// Key structures
struct KEMPublicKey {
    std::array<uint8_t, 32> matrix_seed;
    std::vector<AVXPolynomial> public_vector;
    std::array<uint8_t, 32> color_seed;
    CLWEParameters params;

    std::vector<uint8_t> serialize() const;
    static KEMPublicKey deserialize(const std::vector<uint8_t>& data);
};

struct KEMPrivateKey {
    std::vector<AVXPolynomial> secret_vector;
    CLWEParameters params;

    std::vector<uint8_t> serialize() const;
    static KEMPrivateKey deserialize(const std::vector<uint8_t>& data);
};

struct KEMCiphertext {
    std::vector<AVXPolynomial> ciphertext_vector;
    std::vector<uint8_t> shared_secret_hint;
    CLWEParameters params;

    std::vector<uint8_t> serialize() const;
    static KEMCiphertext deserialize(const std::vector<uint8_t>& data);
};

// IND-CCA2 KEM with FO transform
class KEM {
private:
    CLWEParameters params_;
    std::unique_ptr<RingOperations> ring_ops_;
    std::unique_ptr<AVXNTTEngine> ntt_engine_;

    // FO verification cache to avoid redundant re-encapsulations
    mutable std::unordered_map<std::string, std::vector<uint8_t>> fo_cache_;

    // SIMD-accelerated ciphertext comparison
    bool compare_ciphertexts_simd(const std::vector<uint8_t>& ct1, const std::vector<uint8_t>& ct2) const;

public:
    KEM(const CLWEParameters& params);
    ~KEM();

    // Disable copy and assignment
    KEM(const KEM&) = delete;
    KEM& operator=(const KEM&) = delete;

    // Key generation
    std::pair<KEMPublicKey, KEMPrivateKey> keygen();

    // Encapsulation: returns (ciphertext, shared_secret)
    std::pair<KEMCiphertext, std::vector<uint8_t>> encapsulate(const KEMPublicKey& public_key,
                                                              const std::vector<uint8_t>& random_m);

    // Decapsulation
    std::vector<uint8_t> decapsulate(const KEMPublicKey& public_key,
                                   const KEMPrivateKey& private_key,
                                   const KEMCiphertext& ciphertext);

    // Key verification
    bool verify_keypair(const KEMPublicKey& public_key, const KEMPrivateKey& private_key) const;

    // FO transform helpers (public for benchmarking)
    std::array<uint8_t, 32> derive_seed(const std::vector<uint8_t>& base, uint32_t index) const;
    std::vector<uint8_t> encode_secret_deterministic(const std::vector<uint8_t>& secret,
                                                   const std::vector<uint8_t>& key_material) const;
    std::vector<uint8_t> decode_secret_deterministic(const std::vector<uint8_t>& encoded_hint,
                                                   const std::vector<uint8_t>& key_material) const;

    // Getters
    const CLWEParameters& params() const { return params_; }
};

} // namespace clwe

#endif // KEM_HPP