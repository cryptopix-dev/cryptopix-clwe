# Usage Guide: Cryptopix-CLWE

This guide provides comprehensive examples and tutorials for using Cryptopix-CLWE in your applications.

## Table of Contents

- [Quick Start](#quick-start)
- [Basic Concepts](#basic-concepts)
- [API Reference](#api-reference)
- [Code Examples](#code-examples)
- [Integration Patterns](#integration-patterns)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)

## Quick Start

### Hello World Example

```cpp
#include <iostream>
#include <clwe/clwe.hpp>
#include <clwe/color_kem.hpp>

int main() {
    // Initialize Color KEM with 128-bit security
    clwe::ColorKEM kem{clwe::CLWEParameters(128)};

    // Generate key pair
    auto [public_key, private_key] = kem.keygen();
    std::cout << "Keys generated successfully!" << std::endl;

    // Encapsulate shared secret
    auto [ciphertext, shared_secret] = kem.encapsulate(public_key);
    std::cout << "Shared secret encapsulated!" << std::endl;

    // Decapsulate shared secret
    auto recovered_secret = kem.decapsulate(public_key, private_key, ciphertext);

    // Verify successful key exchange
    if (shared_secret.to_precise_value() == recovered_secret.to_precise_value()) {
        std::cout << "Key exchange successful!" << std::endl;
        return 0;
    } else {
        std::cout << "Key exchange failed!" << std::endl;
        return 1;
    }
}
```

Compile and run:

```bash
g++ -std=c++17 -I/path/to/cryptopix-clwe/src/include \
    -L/path/to/cryptopix-clwe/build -lclwe_avx \
    hello_world.cpp -o hello_world

./hello_world
```

## Basic Concepts

### Color Values

Color values are the fundamental cryptographic primitives in Cryptopix-CLWE:

```cpp
#include <clwe/color_value.hpp>

// Create color values
clwe::ColorValue c1 = clwe::ColorValue::from_precise_value(42);
clwe::ColorValue c2 = clwe::ColorValue::from_precise_value(1337);

// Arithmetic operations
clwe::ColorValue sum = c1 + c2;
clwe::ColorValue product = c1 * c2;

// Convert back to integers
uint32_t value = sum.to_precise_value();
```

### Parameters

Cryptopix-CLWE supports multiple security levels:

```cpp
// 128-bit security (recommended for most applications)
clwe::CLWEParameters params_128(128);

// 192-bit security (high-security applications)
clwe::CLWEParameters params_192(192);

// 256-bit security (maximum security)
clwe::CLWEParameters params_256(256);

// Custom parameters
clwe::CLWEParameters custom_params;
custom_params.security_level = 128;
custom_params.degree = 256;
custom_params.module_rank = 2;
custom_params.modulus = 3329;
custom_params.eta = 2;
```

### Key Management

```cpp
#include <clwe/color_kem.hpp>

// Initialize KEM
clwe::ColorKEM kem(params_128);

// Generate keys
auto [public_key, private_key] = kem.keygen();

// Serialize keys for storage/transmission
std::vector<uint8_t> pk_bytes = public_key.serialize();
std::vector<uint8_t> sk_bytes = private_key.serialize();

// Deserialize keys
clwe::ColorKEM::ColorPublicKey restored_pk =
    clwe::ColorKEM::ColorPublicKey::deserialize(pk_bytes);
clwe::ColorKEM::ColorPrivateKey restored_sk =
    clwe::ColorKEM::ColorPrivateKey::deserialize(sk_bytes);
```

## API Reference

### CLWEParameters

```cpp
struct CLWEParameters {
    uint32_t security_level;  // 128, 192, or 256
    uint32_t degree;          // Ring degree (default: 256)
    uint32_t module_rank;     // Module rank k (default: 2)
    uint32_t modulus;         // Prime modulus q (default: 3329)
    uint32_t eta;            // Error distribution parameter (default: 2)
    uint32_t beta;           // Signature bound (default: 120)

    CLWEParameters(uint32_t sec_level = 128);
};
```

### ColorKEM

```cpp
class ColorKEM {
public:
    ColorKEM(const CLWEParameters& params = CLWEParameters());

    // Key management
    std::pair<ColorPublicKey, ColorPrivateKey> keygen();
    bool verify_keypair(const ColorPublicKey& pk, const ColorPrivateKey& sk) const;

    // Key encapsulation
    std::pair<ColorCiphertext, ColorValue> encapsulate(const ColorPublicKey& pk);
    ColorValue decapsulate(const ColorPublicKey& pk,
                          const ColorPrivateKey& sk,
                          const ColorCiphertext& ct);

    // Utilities
    const CLWEParameters& params() const;
    static std::vector<uint8_t> color_secret_to_bytes(const ColorValue& secret);
    static ColorValue bytes_to_color_secret(const std::vector<uint8_t>& bytes);
};
```

## Code Examples

### Secure Communication Channel

```cpp
#include <clwe/color_kem.hpp>
#include <iostream>
#include <vector>

class SecureChannel {
private:
    clwe::ColorKEM kem_;
    clwe::ColorKEM::ColorPublicKey peer_public_key_;
    clwe::ColorKEM::ColorPrivateKey my_private_key_;

public:
    SecureChannel(const clwe::CLWEParameters& params = clwe::CLWEParameters(128))
        : kem_(params) {}

    // Initialize with peer's public key
    void set_peer_key(const clwe::ColorKEM::ColorPublicKey& peer_key) {
        peer_public_key_ = peer_key;
    }

    // Generate my key pair
    clwe::ColorKEM::ColorPublicKey generate_keys() {
        auto [pk, sk] = kem_.keygen();
        my_private_key_ = sk;
        return pk;
    }

    // Send encrypted message
    std::pair<clwe::ColorKEM::ColorCiphertext, clwe::ColorValue>
    send_message(const std::string& message) {
        // Encapsulate shared secret
        auto [ciphertext, shared_secret] = kem_.encapsulate(peer_public_key_);

        // Use shared secret to encrypt message (simplified)
        // In practice, use HKDF + AES-GCM
        std::cout << "Sending encrypted message with shared secret: "
                  << shared_secret.to_precise_value() << std::endl;

        return {ciphertext, shared_secret};
    }

    // Receive encrypted message
    clwe::ColorValue receive_message(const clwe::ColorKEM::ColorCiphertext& ciphertext) {
        // Decapsulate shared secret
        clwe::ColorValue shared_secret = kem_.decapsulate(peer_public_key_,
                                                         my_private_key_,
                                                         ciphertext);

        std::cout << "Received message with shared secret: "
                  << shared_secret.to_precise_value() << std::endl;

        return shared_secret;
    }
};

int main() {
    SecureChannel alice, bob;

    // Key exchange
    auto alice_pk = alice.generate_keys();
    auto bob_pk = bob.generate_keys();

    alice.set_peer_key(bob_pk);
    bob.set_peer_key(alice_pk);

    // Alice sends message
    auto [ciphertext, alice_secret] = alice.send_message("Hello Bob!");

    // Bob receives message
    auto bob_secret = bob.receive_message(ciphertext);

    // Verify shared secrets match
    if (alice_secret.to_precise_value() == bob_secret.to_precise_value()) {
        std::cout << "Secure communication established!" << std::endl;
    }

    return 0;
}
```

### Hybrid Encryption

```cpp
#include <clwe/color_kem.hpp>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <vector>
#include <string>

class HybridEncryptor {
private:
    clwe::ColorKEM kem_;

    // Derive symmetric key from shared secret
    std::vector<uint8_t> derive_key(const clwe::ColorValue& secret,
                                   size_t key_length = 32) {
        // Use HKDF to derive AES key from shared secret
        std::vector<uint8_t> shared_bytes = clwe::ColorKEM::color_secret_to_bytes(secret);

        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
        EVP_PKEY_derive_init(ctx);
        EVP_PKEY_CTX_set_hkdf_md(ctx, EVP_sha256());
        EVP_PKEY_CTX_set1_hkdf_salt(ctx, nullptr, 0);  // No salt
        EVP_PKEY_CTX_set1_hkdf_key(ctx, shared_bytes.data(), shared_bytes.size());
        EVP_PKEY_CTX_set1_hkdf_info(ctx, (const unsigned char*)"aes-key", 7);

        std::vector<uint8_t> derived_key(key_length);
        size_t out_len = key_length;
        EVP_PKEY_derive(ctx, derived_key.data(), &out_len);

        EVP_PKEY_CTX_free(ctx);
        return derived_key;
    }

public:
    HybridEncryptor(const clwe::CLWEParameters& params = clwe::CLWEParameters(128))
        : kem_(params) {}

    // Encrypt data using hybrid scheme
    std::pair<clwe::ColorKEM::ColorCiphertext, std::vector<uint8_t>>
    encrypt(const clwe::ColorKEM::ColorPublicKey& recipient_key,
            const std::string& plaintext) {

        // 1. Encapsulate shared secret
        auto [kem_ciphertext, shared_secret] = kem_.encapsulate(recipient_key);

        // 2. Derive AES key from shared secret
        std::vector<uint8_t> aes_key = derive_key(shared_secret);

        // 3. Encrypt data with AES-GCM
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);

        // Generate random IV
        std::vector<uint8_t> iv(12);
        RAND_bytes(iv.data(), iv.size());

        EVP_EncryptInit_ex(ctx, nullptr, nullptr, aes_key.data(), iv.data());

        // Encrypt
        std::vector<uint8_t> ciphertext(plaintext.size() + 16);  // +16 for auth tag
        int out_len;
        EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len,
                         (const unsigned char*)plaintext.data(), plaintext.size());

        // Finalize with auth tag
        int final_len;
        EVP_EncryptFinal_ex(ctx, ciphertext.data() + out_len, &final_len);

        // Get auth tag
        std::vector<uint8_t> auth_tag(16);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, auth_tag.data());

        EVP_CIPHER_CTX_free(ctx);

        // Combine IV + ciphertext + auth tag
        std::vector<uint8_t> encrypted_data;
        encrypted_data.insert(encrypted_data.end(), iv.begin(), iv.end());
        encrypted_data.insert(encrypted_data.end(), ciphertext.begin(),
                             ciphertext.begin() + out_len + final_len);
        encrypted_data.insert(encrypted_data.end(), auth_tag.begin(), auth_tag.end());

        return {kem_ciphertext, encrypted_data};
    }

    // Decrypt data using hybrid scheme
    std::string decrypt(const clwe::ColorKEM::ColorPublicKey& sender_key,
                       const clwe::ColorKEM::ColorPrivateKey& recipient_key,
                       const clwe::ColorKEM::ColorCiphertext& kem_ciphertext,
                       const std::vector<uint8_t>& encrypted_data) {

        // 1. Decapsulate shared secret
        clwe::ColorValue shared_secret = kem_.decapsulate(sender_key,
                                                         recipient_key,
                                                         kem_ciphertext);

        // 2. Derive AES key from shared secret
        std::vector<uint8_t> aes_key = derive_key(shared_secret);

        // 3. Parse encrypted data: IV (12) + ciphertext + auth_tag (16)
        if (encrypted_data.size() < 28) {  // 12 + 16 minimum
            throw std::runtime_error("Invalid encrypted data");
        }

        std::vector<uint8_t> iv(encrypted_data.begin(), encrypted_data.begin() + 12);
        std::vector<uint8_t> auth_tag(encrypted_data.end() - 16, encrypted_data.end());
        std::vector<uint8_t> ciphertext(encrypted_data.begin() + 12,
                                       encrypted_data.end() - 16);

        // 4. Decrypt with AES-GCM
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
        EVP_DecryptInit_ex(ctx, nullptr, nullptr, aes_key.data(), iv.data());

        std::string plaintext(ciphertext.size(), '\0');
        int out_len;
        EVP_DecryptUpdate(ctx, (unsigned char*)plaintext.data(), &out_len,
                         ciphertext.data(), ciphertext.size());

        // Set expected auth tag
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, auth_tag.data());

        // Finalize
        int final_len;
        int ret = EVP_DecryptFinal_ex(ctx, nullptr, &final_len);

        EVP_CIPHER_CTX_free(ctx);

        if (ret <= 0) {
            throw std::runtime_error("Authentication failed");
        }

        plaintext.resize(out_len);
        return plaintext;
    }
};

int main() {
    HybridEncryptor alice, bob;

    // Setup keys
    auto alice_pk = alice.generate_keys();
    auto bob_pk = bob.generate_keys();

    alice.set_peer_key(bob_pk);
    bob.set_peer_key(alice_pk);

    // Encrypt message
    std::string message = "This is a secret message!";
    auto [kem_ct, encrypted] = alice.encrypt(bob_pk, message);

    // Decrypt message
    std::string decrypted = bob.decrypt(alice_pk, bob.get_private_key(),
                                       kem_ct, encrypted);

    if (message == decrypted) {
        std::cout << "Hybrid encryption successful!" << std::endl;
    }

    return 0;
}
```

### Performance Monitoring

```cpp
#include <clwe/color_kem.hpp>
#include <chrono>
#include <iostream>

class PerformanceMonitor {
private:
    clwe::ColorKEM kem_;

public:
    PerformanceMonitor(const clwe::CLWEParameters& params = clwe::CLWEParameters(128))
        : kem_(params) {}

    void benchmark_operations(size_t iterations = 1000) {
        auto [pk, sk] = kem_.keygen();

        // Benchmark key generation
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < iterations; ++i) {
            auto [test_pk, test_sk] = kem_.keygen();
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto keygen_time = std::chrono::duration_cast<std::chrono::microseconds>
                          (end - start).count() / static_cast<double>(iterations);

        // Benchmark encapsulation
        start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < iterations; ++i) {
            auto [ct, ss] = kem_.encapsulate(pk);
        }
        end = std::chrono::high_resolution_clock::now();
        auto encap_time = std::chrono::duration_cast<std::chrono::microseconds>
                         (end - start).count() / static_cast<double>(iterations);

        // Benchmark decapsulation
        auto [test_ct, test_ss] = kem_.encapsulate(pk);
        start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < iterations; ++i) {
            auto recovered_ss = kem_.decapsulate(pk, sk, test_ct);
        }
        end = std::chrono::high_resolution_clock::now();
        auto decap_time = std::chrono::duration_cast<std::chrono::microseconds>
                         (end - start).count() / static_cast<double>(iterations);

        std::cout << "Performance Results (" << iterations << " iterations):" << std::endl;
        std::cout << "Key Generation: " << keygen_time << " μs" << std::endl;
        std::cout << "Encapsulation:  " << encap_time << " μs" << std::endl;
        std::cout << "Decapsulation:  " << decap_time << " μs" << std::endl;
        std::cout << "Total KEM time: " << (keygen_time + encap_time + decap_time) << " μs" << std::endl;
        std::cout << "Throughput: " << (1000000.0 / (keygen_time + encap_time + decap_time)) << " ops/s" << std::endl;
    }
};

int main() {
    PerformanceMonitor monitor;
    monitor.benchmark_operations(100);  // Warm up
    monitor.benchmark_operations(1000); // Actual benchmark

    return 0;
}
```

## Integration Patterns

### C++ Applications

#### CMake Integration

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(my_app)

# Find Cryptopix-CLWE
find_package(CryptopixCLWE REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app CryptopixCLWE::clwe_avx)
```

#### Makefile Integration

```makefile
CXXFLAGS = -std=c++17 -O3
INCLUDES = -I/path/to/cryptopix-clwe/src/include
LIBS = -L/path/to/cryptopix-clwe/build -lclwe_avx

my_app: main.cpp
    $(CXX) $(CXXFLAGS) $(INCLUDES) main.cpp $(LIBS) -o my_app
```

### System Integration

#### Shared Library Usage

```cpp
// Load library at runtime
#include <dlfcn.h>

void* handle = dlopen("libclwe_avx.so", RTLD_LAZY);
if (!handle) {
    std::cerr << "Cannot load library: " << dlerror() << std::endl;
    return 1;
}

// Get function pointers
typedef clwe::ColorKEM* (*create_kem_func)(const clwe::CLWEParameters&);
create_kem_func create_kem = (create_kem_func)dlsym(handle, "create_color_kem");

// Use the library
clwe::CLWEParameters params(128);
clwe::ColorKEM* kem = create_kem(params);

// ... use kem ...

// Cleanup
dlclose(handle);
```

## Best Practices

### Security

1. **Key Management**
   - Generate fresh keys for each session
   - Store private keys securely (encrypted at rest)
   - Use hardware security modules when available
   - Implement proper key rotation

2. **Parameter Selection**
   - Use 128-bit security for most applications
   - Use 256-bit security for long-term protection
   - Consider performance vs. security trade-offs

3. **Input Validation**
   - Validate all cryptographic inputs
   - Check key lengths and formats
   - Implement proper error handling

### Performance

1. **Memory Management**
   - Reuse ColorKEM instances when possible
   - Pre-allocate buffers for frequent operations
   - Use memory pools for high-throughput applications

2. **SIMD Optimization**
   - Ensure CPU supports required SIMD instructions
   - Use appropriate security levels for your hardware
   - Monitor performance with built-in benchmarks

3. **Threading**
   - ColorKEM is not thread-safe by default
   - Use one instance per thread
   - Consider lock-free designs for high concurrency

### Error Handling

```cpp
#include <clwe/clwe.hpp>

clwe::ColorKEM kem(params);

try {
    auto [pk, sk] = kem.keygen();

    // Verify key pair
    if (!kem.verify_keypair(pk, sk)) {
        throw std::runtime_error("Key pair verification failed");
    }

    auto [ct, ss] = kem.encapsulate(pk);
    auto recovered_ss = kem.decapsulate(pk, sk, ct);

    if (ss.to_precise_value() != recovered_ss.to_precise_value()) {
        throw std::runtime_error("Shared secret mismatch");
    }

} catch (const std::exception& e) {
    std::cerr << "Cryptographic operation failed: " << e.what() << std::endl;
    // Implement proper error recovery
}
```

## Troubleshooting

### Common Issues

1. **Library Not Found**
   ```bash
   # Set library path
   export LD_LIBRARY_PATH=/path/to/cryptopix-clwe/build:$LD_LIBRARY_PATH

   # Or copy to system location
   sudo cp libclwe_avx.so /usr/local/lib/
   sudo ldconfig
   ```

2. **SIMD Not Detected**
   ```cpp
   // Check CPU features
   #include <clwe/cpu_features.hpp>
   clwe::CPUFeatures features;
   std::cout << "AVX2: " << features.has_avx2() << std::endl;
   std::cout << "NEON: " << features.has_neon() << std::endl;
   ```

3. **Performance Issues**
   - Ensure Release build is used
   - Check that appropriate SIMD instructions are enabled
   - Profile with benchmark tools

4. **Memory Issues**
   - Verify sufficient RAM (4GB+ recommended)
   - Check for memory leaks with valgrind
   - Monitor memory usage in production

### Debug Mode

```cpp
// Enable debug logging
#define CLWE_DEBUG
#include <clwe/color_kem.hpp>

// Debug builds show additional information
clwe::ColorKEM kem(params, true);  // verbose mode
```

### Getting Help

- Check existing GitHub issues
- Review documentation in `docs/` directory
- Run built-in tests and benchmarks
- Provide detailed error information when reporting issues

---

*For more examples, see the `examples/` directory in the repository.*