#ifndef COLOR_KEM_HPP
#define COLOR_KEM_HPP

#include "color_value.hpp"
#include "color_ntt_engine.hpp"
#include "clwe/clwe.hpp"
#include <vector>
#include <array>

namespace clwe {

class ColorKEM {
private:
    CLWEParameters params_;
    std::unique_ptr<ColorNTTEngine> color_ntt_engine_;

    struct ColorPublicKey {
        std::array<uint8_t, 32> seed;
        std::vector<uint8_t> public_data;
        CLWEParameters params;

        std::vector<uint8_t> serialize() const;
        static ColorPublicKey deserialize(const std::vector<uint8_t>& data);
    };

    struct ColorPrivateKey {
        std::vector<uint8_t> secret_data;
        CLWEParameters params;

        std::vector<uint8_t> serialize() const;
        static ColorPrivateKey deserialize(const std::vector<uint8_t>& data);
    };

    struct ColorCiphertext {
        std::vector<uint8_t> ciphertext_data;
        std::vector<uint8_t> shared_secret_hint;
        CLWEParameters params;

        std::vector<uint8_t> serialize() const;
        static ColorCiphertext deserialize(const std::vector<uint8_t>& data);
    };

    std::vector<std::vector<ColorValue>> generate_matrix_A(const std::array<uint8_t, 32>& seed) const;
    std::vector<ColorValue> generate_secret_key() const;
    std::vector<ColorValue> generate_error_vector() const;
    std::vector<ColorValue> generate_public_key(const std::vector<ColorValue>& secret_key,
                                               const std::vector<std::vector<ColorValue>>& matrix_A,
                                               const std::vector<ColorValue>& error_vector) const;
    std::vector<ColorValue> encrypt_message(const std::vector<std::vector<ColorValue>>& matrix_A,
                                          const std::vector<ColorValue>& public_key,
                                          const ColorValue& message) const;
    ColorValue decrypt_message(const std::vector<ColorValue>& secret_key,
                             const std::vector<ColorValue>& ciphertext) const;

    std::vector<ColorValue> matrix_vector_mul(const std::vector<std::vector<ColorValue>>& matrix,
                                            const std::vector<ColorValue>& vector) const;
    std::vector<ColorValue> matrix_transpose_vector_mul(const std::vector<std::vector<ColorValue>>& matrix,
                                                      const std::vector<ColorValue>& vector) const;

    ColorValue generate_shared_secret() const;
    std::vector<uint8_t> encode_color_secret(const ColorValue& secret) const;
    ColorValue decode_color_secret(const std::vector<uint8_t>& encoded) const;

public:
    ColorKEM(const CLWEParameters& params = CLWEParameters());
    ~ColorKEM();

    ColorKEM(const ColorKEM&) = delete;
    ColorKEM& operator=(const ColorKEM&) = delete;

    std::pair<ColorPublicKey, ColorPrivateKey> keygen();

    std::pair<ColorCiphertext, ColorValue> encapsulate(const ColorPublicKey& public_key);

    ColorValue decapsulate(const ColorPublicKey& public_key,
                          const ColorPrivateKey& private_key,
                          const ColorCiphertext& ciphertext);

    bool verify_keypair(const ColorPublicKey& public_key, const ColorPrivateKey& private_key) const;

    const CLWEParameters& params() const { return params_; }

    static std::vector<uint8_t> color_secret_to_bytes(const ColorValue& secret);
    static ColorValue bytes_to_color_secret(const std::vector<uint8_t>& bytes);
};

} // namespace clwe

#endif // COLOR_KEM_HPP