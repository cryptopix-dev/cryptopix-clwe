#include "color_kem.hpp"
#include "shake_sampler.hpp"
#include "utils.hpp"
#include <random>
#include <cstring>
#include <algorithm>

namespace clwe {

ColorKEM::ColorKEM(const CLWEParameters& params)
    : params_(params) {
    color_ntt_engine_ = std::make_unique<ColorNTTEngine>(params_.modulus, params_.degree);
}

ColorKEM::~ColorKEM() = default;


std::vector<std::vector<ColorValue>> ColorKEM::generate_matrix_A(const std::array<uint8_t, 32>& seed) const {
    uint32_t k = params_.module_rank;
    uint32_t n = params_.degree;
    uint32_t q = params_.modulus;

    std::vector<std::vector<ColorValue>> matrix(k, std::vector<ColorValue>(k));

    
    for (uint32_t i = 0; i < k; ++i) {
        for (uint32_t j = 0; j < k; ++j) {
            
            std::vector<uint32_t> coeffs(n);

            
            std::vector<uint8_t> shake_input;
            shake_input.reserve(seed.size() + 2);
            shake_input.insert(shake_input.end(), seed.begin(), seed.end());
            shake_input.push_back(static_cast<uint8_t>(i));
            shake_input.push_back(static_cast<uint8_t>(j));

            
            SHAKE128Sampler shake128;
            shake128.init(shake_input.data(), shake_input.size());

            
            size_t coeff_idx = 0;
            while (coeff_idx < n) {
                std::array<uint8_t, 3> bytes;
                shake128.squeeze(bytes.data(), bytes.size());

                
                uint16_t coeff1 = ((bytes[0] << 4) | (bytes[1] >> 4)) & 0xFFF;
                uint16_t coeff2 = ((bytes[1] << 8) | bytes[2]) & 0xFFF;

                if (coeff1 < q && coeff_idx < n) {
                    coeffs[coeff_idx++] = coeff1;
                }
                if (coeff2 < q && coeff_idx < n) {
                    coeffs[coeff_idx++] = coeff2;
                }
            }

            
            for (uint32_t coeff = 0; coeff < n; ++coeff) {
                matrix[i][j] = ColorValue::from_precise_value(coeffs[coeff]);
            }
        }
    }

    return matrix;
}


std::vector<ColorValue> ColorKEM::generate_error_vector() const {
    std::vector<ColorValue> error_vector(params_.module_rank);

    
    SHAKE256Sampler sampler;
    std::array<uint8_t, 32> seed;
    std::generate(seed.begin(), seed.end(), []() { return rand() % 256; });
    sampler.init(seed.data(), seed.size());

    for (auto& color : error_vector) {
        
        int32_t sample = sampler.sample_binomial_coefficient(params_.eta);
        
        uint32_t value = (sample % static_cast<int32_t>(params_.modulus) + params_.modulus) % params_.modulus;
        color = ColorValue::from_precise_value(value);
    }

    return error_vector;
}


std::vector<ColorValue> ColorKEM::generate_secret_key() const {
    std::vector<ColorValue> secret_key(params_.module_rank);

    
    SHAKE256Sampler sampler;
    std::array<uint8_t, 32> seed;
    std::generate(seed.begin(), seed.end(), []() { return rand() % 256; });
    sampler.init(seed.data(), seed.size());

    for (auto& color : secret_key) {
        
        int32_t sample = sampler.sample_binomial_coefficient(params_.eta);
        
        uint32_t value = (sample % static_cast<int32_t>(params_.modulus) + params_.modulus) % params_.modulus;
        color = ColorValue::from_precise_value(value);
    }

    return secret_key;
}


std::vector<ColorValue> ColorKEM::generate_public_key(const std::vector<ColorValue>& secret_key,
                                                    const std::vector<std::vector<ColorValue>>& matrix_A,
                                                    const std::vector<ColorValue>& error_vector) const {
    
    auto As = this->matrix_vector_mul(matrix_A, secret_key);
    std::vector<ColorValue> public_key(params_.module_rank);

    for (uint32_t i = 0; i < params_.module_rank; ++i) {
        
        uint64_t as_val = As[i].to_precise_value();
        uint64_t e_val = error_vector[i].to_precise_value();
        uint64_t pk_val = (as_val + e_val) % params_.modulus;
        public_key[i] = ColorValue::from_precise_value(pk_val);
    }

    return public_key;
}


std::vector<ColorValue> ColorKEM::matrix_vector_mul(const std::vector<std::vector<ColorValue>>& matrix,
                                                  const std::vector<ColorValue>& vector) const {
    uint32_t k = params_.module_rank;
    std::vector<ColorValue> result(k);

    for (uint32_t i = 0; i < k; ++i) {
        uint64_t sum = 0;
        for (uint32_t j = 0; j < k; ++j) {
            uint64_t m_val = matrix[i][j].to_precise_value();
            uint64_t v_val = vector[j].to_precise_value();
            sum = (sum + m_val * v_val) % params_.modulus;
        }
        result[i] = ColorValue::from_precise_value(sum);
    }

    return result;
}


std::vector<ColorValue> ColorKEM::matrix_transpose_vector_mul(const std::vector<std::vector<ColorValue>>& matrix,
                                                            const std::vector<ColorValue>& vector) const {
    uint32_t k = params_.module_rank;
    std::vector<ColorValue> result(k);

    for (uint32_t i = 0; i < k; ++i) {
        uint64_t sum = 0;
        for (uint32_t j = 0; j < k; ++j) {
            uint64_t m_val = matrix[j][i].to_precise_value();  
            uint64_t v_val = vector[j].to_precise_value();
            sum = (sum + m_val * v_val) % params_.modulus;
        }
        result[i] = ColorValue::from_precise_value(sum);
    }

    return result;
}



ColorValue ColorKEM::decrypt_message(const std::vector<ColorValue>& secret_key,
                                    const std::vector<ColorValue>& ciphertext) const {
    
    

    uint32_t k = params_.module_rank;
    uint32_t q = params_.modulus;

    
    std::vector<ColorValue> c1(ciphertext.begin(), ciphertext.begin() + k);
    ColorValue c2 = ciphertext[k];

    
    uint64_t s_dot_c1 = 0;
    for (uint32_t i = 0; i < k; ++i) {
        uint64_t sk_val = secret_key[i].to_precise_value();
        uint64_t c1_val = c1[i].to_precise_value();
        s_dot_c1 = (s_dot_c1 + sk_val * c1_val) % q;
    }

    
    uint64_t c2_val = c2.to_precise_value();
    uint64_t v = (c2_val >= s_dot_c1) ? (c2_val - s_dot_c1) : (c2_val + q - s_dot_c1);
    v %= q;

    
    
    uint64_t q_half = q / 2;
    uint32_t m = (v > q_half) ? 1 : 0;

    return ColorValue::from_precise_value(m);
}


ColorValue ColorKEM::generate_shared_secret() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, params_.modulus - 1);

    return ColorValue::from_precise_value(dist(gen));
}


std::vector<uint8_t> ColorKEM::encode_color_secret(const ColorValue& secret) const {
    uint32_t value = static_cast<uint32_t>(secret.to_precise_value());
    return {
        static_cast<uint8_t>((value >> 24) & 0xFF),
        static_cast<uint8_t>((value >> 16) & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>(value & 0xFF)
    };
}


ColorValue ColorKEM::decode_color_secret(const std::vector<uint8_t>& encoded) const {
    if (encoded.size() < 4) return ColorValue::from_precise_value(0);

    uint32_t value = (static_cast<uint32_t>(encoded[0]) << 24) |
                    (static_cast<uint32_t>(encoded[1]) << 16) |
                    (static_cast<uint32_t>(encoded[2]) << 8) |
                    static_cast<uint32_t>(encoded[3]);

    return ColorValue::from_precise_value(value);
}


std::pair<ColorKEM::ColorPublicKey, ColorKEM::ColorPrivateKey> ColorKEM::keygen() {
    
    std::array<uint8_t, 32> matrix_seed;
    std::random_device rd;
    std::generate(matrix_seed.begin(), matrix_seed.end(), [&rd]() { return rd() % 256; });

    
    auto matrix_A = generate_matrix_A(matrix_seed);

    
    auto secret_key_colors = generate_secret_key();

    
    auto error_vector = generate_error_vector();

    
    auto public_key_colors = generate_public_key(secret_key_colors, matrix_A, error_vector);

    
    std::vector<uint8_t> secret_data;
    for (const auto& color : secret_key_colors) {
        auto bytes = color_secret_to_bytes(color);
        secret_data.insert(secret_data.end(), bytes.begin(), bytes.end());
    }

    std::vector<uint8_t> public_data;
    for (const auto& color : public_key_colors) {
        auto bytes = color_secret_to_bytes(color);
        public_data.insert(public_data.end(), bytes.begin(), bytes.end());
    }

    ColorPublicKey public_key{matrix_seed, public_data, params_};
    ColorPrivateKey private_key{secret_data, params_};

    return {public_key, private_key};
}


std::pair<ColorKEM::ColorCiphertext, ColorValue> ColorKEM::encapsulate(const ColorPublicKey& public_key) {
    
    ColorValue shared_secret = ColorValue::from_precise_value(rand() % 2);  

    
    auto matrix_A = generate_matrix_A(public_key.seed);

    
    std::vector<ColorValue> public_key_colors;
    for (size_t i = 0; i < public_key.public_data.size(); i += 4) {
        std::vector<uint8_t> bytes(public_key.public_data.begin() + i,
                                  public_key.public_data.begin() + std::min(i + 4, public_key.public_data.size()));
        public_key_colors.push_back(bytes_to_color_secret(bytes));
    }

    
    auto ciphertext_colors = encrypt_message(matrix_A, public_key_colors, shared_secret);

    
    std::vector<uint8_t> ciphertext_data;
    for (const auto& color : ciphertext_colors) {
        auto bytes = color_secret_to_bytes(color);
        ciphertext_data.insert(ciphertext_data.end(), bytes.begin(), bytes.end());
    }

    
    auto shared_secret_hint = encode_color_secret(shared_secret);

    ColorCiphertext ciphertext{ciphertext_data, shared_secret_hint, params_};

    return {ciphertext, shared_secret};
}


ColorValue ColorKEM::decapsulate(const ColorPublicKey& public_key,
                                const ColorPrivateKey& private_key,
                                const ColorCiphertext& ciphertext) {
    
    std::vector<ColorValue> secret_key_colors;
    for (size_t i = 0; i < private_key.secret_data.size(); i += 4) {
        std::vector<uint8_t> bytes(private_key.secret_data.begin() + i,
                                  private_key.secret_data.begin() + std::min(i + 4, private_key.secret_data.size()));
        secret_key_colors.push_back(bytes_to_color_secret(bytes));
    }

    
    std::vector<ColorValue> ciphertext_colors;
    for (size_t i = 0; i < ciphertext.ciphertext_data.size(); i += 4) {
        std::vector<uint8_t> bytes(ciphertext.ciphertext_data.begin() + i,
                                  ciphertext.ciphertext_data.begin() + std::min(i + 4, ciphertext.ciphertext_data.size()));
        ciphertext_colors.push_back(bytes_to_color_secret(bytes));
    }

    
    ColorValue recovered_secret = decrypt_message(secret_key_colors, ciphertext_colors);

    return recovered_secret;
}


bool ColorKEM::verify_keypair(const ColorPublicKey& public_key, const ColorPrivateKey& private_key) const {
    
    return public_key.params.security_level == private_key.params.security_level &&
           public_key.params.modulus == private_key.params.modulus;
}


std::vector<uint8_t> ColorKEM::color_secret_to_bytes(const ColorValue& secret) {
    uint32_t value = static_cast<uint32_t>(secret.to_precise_value());
    return {
        static_cast<uint8_t>((value >> 24) & 0xFF),
        static_cast<uint8_t>((value >> 16) & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>(value & 0xFF)
    };
}

ColorValue ColorKEM::bytes_to_color_secret(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 4) return ColorValue::from_precise_value(0);

    uint32_t value = (static_cast<uint32_t>(bytes[0]) << 24) |
                    (static_cast<uint32_t>(bytes[1]) << 16) |
                    (static_cast<uint32_t>(bytes[2]) << 8) |
                    static_cast<uint32_t>(bytes[3]);

    return ColorValue::from_precise_value(value);
}


std::vector<uint8_t> ColorKEM::ColorPublicKey::serialize() const {
    std::vector<uint8_t> data;
    
    data.insert(data.end(), seed.begin(), seed.end());
    
    data.insert(data.end(), public_data.begin(), public_data.end());
    return data;
}

ColorKEM::ColorPublicKey ColorKEM::ColorPublicKey::deserialize(const std::vector<uint8_t>& data) {
    ColorPublicKey key;
    if (data.size() >= 32) {
        std::copy(data.begin(), data.begin() + 32, key.seed.begin());
        key.public_data.assign(data.begin() + 32, data.end());
    }
    return key;
}

std::vector<uint8_t> ColorKEM::ColorPrivateKey::serialize() const {
    return secret_data;
}

ColorKEM::ColorPrivateKey ColorKEM::ColorPrivateKey::deserialize(const std::vector<uint8_t>& data) {
    ColorPrivateKey key;
    key.secret_data = data;
    return key;
}

std::vector<uint8_t> ColorKEM::ColorCiphertext::serialize() const {
    std::vector<uint8_t> data;
    data.insert(data.end(), ciphertext_data.begin(), ciphertext_data.end());
    data.insert(data.end(), shared_secret_hint.begin(), shared_secret_hint.end());
    return data;
}

ColorKEM::ColorCiphertext ColorKEM::ColorCiphertext::deserialize(const std::vector<uint8_t>& data) {
    ColorCiphertext ct;
    
    size_t split = data.size() / 2;
    ct.ciphertext_data.assign(data.begin(), data.begin() + split);
    ct.shared_secret_hint.assign(data.begin() + split, data.end());
    return ct;
}


std::vector<ColorValue> ColorKEM::encrypt_message(const std::vector<std::vector<ColorValue>>& matrix_A,
                                                const std::vector<ColorValue>& public_key,
                                                const ColorValue& message) const {
    
    std::vector<ColorValue> ciphertext(params_.module_rank + 1);

    
    auto r_vector = generate_secret_key();

    
    auto e1_vector = generate_error_vector();
    auto e2 = generate_error_vector()[0];  

    
    for (uint32_t i = 0; i < params_.module_rank; ++i) {
        uint64_t r_val = r_vector[i].to_precise_value();
        uint64_t e1_val = e1_vector[i].to_precise_value();
        uint64_t c1_val = (r_val + e1_val) % params_.modulus;
        ciphertext[i] = ColorValue::from_precise_value(c1_val);
    }

    
    uint64_t inner_product = 0;
    for (uint32_t i = 0; i < params_.module_rank; ++i) {
        uint64_t t_val = public_key[i].to_precise_value();
        uint64_t r_val = r_vector[i].to_precise_value();
        inner_product = (inner_product + t_val * r_val) % params_.modulus;
    }

    uint64_t e2_val = e2.to_precise_value();
    uint64_t m_val = message.to_precise_value();
    
    uint64_t q_fourth = params_.modulus / 4;
    uint64_t encoded_m = m_val * q_fourth;
    uint64_t c2_val = (inner_product + e2_val + encoded_m) % params_.modulus;

    ciphertext[params_.module_rank] = ColorValue::from_precise_value(c2_val);

    return ciphertext;
}

} 