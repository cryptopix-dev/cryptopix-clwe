#ifndef SIGN_HPP
#define SIGN_HPP

#include "clwe.hpp"
#include "ring_operations.hpp"
#include <vector>
#include <array>
#include <memory>

namespace clwe {

// Forward declarations
class RingOperations;

// Key structures
struct SignPublicKey {
    std::array<uint8_t, 32> seed;
    std::vector<AVXPolynomial> public_polys;
    CLWEParameters params;

    std::vector<uint8_t> serialize() const;
    static SignPublicKey deserialize(const std::vector<uint8_t>& data);
};

struct SignPrivateKey {
    std::array<uint8_t, 32> seed;
    std::vector<AVXPolynomial> secret_polys;
    CLWEParameters params;

    std::vector<uint8_t> serialize() const;
    static SignPrivateKey deserialize(const std::vector<uint8_t>& data);
};

struct Signature {
    std::vector<AVXPolynomial> z_polys;
    uint32_t c;
    CLWEParameters params;

    std::vector<uint8_t> serialize() const;
    static Signature deserialize(const std::vector<uint8_t>& data);
};

// Fiat-Shamir signature scheme with rejection sampling
class Sign {
private:
    CLWEParameters params_;
    std::unique_ptr<RingOperations> ring_ops_;
    std::unique_ptr<AVXNTTEngine> ntt_engine_;

public:
    Sign(const CLWEParameters& params);
    ~Sign();

    // Disable copy and assignment
    Sign(const Sign&) = delete;
    Sign& operator=(const Sign&) = delete;

    // Key generation
    std::pair<SignPublicKey, SignPrivateKey> keygen();

    // Signing
    Signature sign(const SignPrivateKey& private_key, const std::vector<uint8_t>& message);

    // Verification
    bool verify(const SignPublicKey& public_key, const std::vector<uint8_t>& message,
               const Signature& signature) const;

    // Key verification
    bool verify_keypair(const SignPublicKey& public_key, const SignPrivateKey& private_key) const;

    // Helper functions (public for benchmarking)
    uint32_t compute_challenge(const std::vector<AVXPolynomial>& w_polys,
                              const std::vector<uint8_t>& message) const;
    int poly_infty_norm(const std::vector<AVXPolynomial>& polys) const;

    // Getters
    const CLWEParameters& params() const { return params_; }
};

} // namespace clwe

#endif // SIGN_HPP