# API Reference: Cryptopix-CLWE

This document provides comprehensive API reference for Cryptopix-CLWE, including all public classes, functions, and types.

## Table of Contents

- [Core Types](#core-types)
- [CLWEParameters](#clweparameters)
- [ColorKEM](#colorkem)
- [ColorValue](#colorvalue)
- [Error Handling](#error-handling)
- [Utility Functions](#utility-functions)
- [Examples](#examples)

## Core Types

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

**Parameters:**
- `security_level`: Target security level in bits
- `degree`: Degree of the polynomial ring (power of 2)
- `module_rank`: Rank of the module (affects key sizes)
- `modulus`: Prime modulus for arithmetic
- `eta`: Parameter for binomial error distribution
- `beta`: Bound for signature operations

**Constructors:**
- `CLWEParameters(sec_level)`: Initialize with security level
- Default constructor uses 128-bit security

### CLWEError

```cpp
enum class CLWEError {
    SUCCESS = 0,
    INVALID_PARAMETERS = 1,
    MEMORY_ALLOCATION_FAILED = 2,
    AVX_NOT_SUPPORTED = 3,
    INVALID_KEY = 4,
    VERIFICATION_FAILED = 5,
    UNKNOWN_ERROR = 6
};
```

Error codes returned by cryptographic operations.

## ColorKEM

Main class for color-integrated key encapsulation mechanism.

### Constructor

```cpp
ColorKEM(const CLWEParameters& params = CLWEParameters());
```

**Parameters:**
- `params`: Cryptographic parameters (default: 128-bit security)

**Exceptions:**
- May throw `std::bad_alloc` if memory allocation fails

### Destructor

```cpp
~ColorKEM();
```

Automatically cleans up resources.

### Key Generation

```cpp
std::pair<ColorPublicKey, ColorPrivateKey> keygen();
```

**Returns:**
- Pair of public and private keys

**Time Complexity:** O(k²·n·log n) where k is module rank, n is degree

**Example:**
```cpp
clwe::ColorKEM kem;
auto [public_key, private_key] = kem.keygen();
```

### Encapsulation

```cpp
std::pair<ColorCiphertext, ColorValue> encapsulate(const ColorPublicKey& public_key);
```

**Parameters:**
- `public_key`: Recipient's public key

**Returns:**
- Pair of (ciphertext, shared_secret)

**Time Complexity:** O(k·n·log n)

**Exceptions:**
- `std::invalid_argument` if public key is invalid

**Example:**
```cpp
auto [ciphertext, shared_secret] = kem.encapsulate(public_key);
```

### Decapsulation

```cpp
ColorValue decapsulate(const ColorPublicKey& public_key,
                      const ColorPrivateKey& private_key,
                      const ColorCiphertext& ciphertext);
```

**Parameters:**
- `public_key`: Sender's public key
- `private_key`: Recipient's private key
- `ciphertext`: Encrypted data

**Returns:**
- Decapsulated shared secret

**Time Complexity:** O(k·n)

**Exceptions:**
- `std::invalid_argument` if keys or ciphertext are invalid

**Example:**
```cpp
clwe::ColorValue secret = kem.decapsulate(public_key, private_key, ciphertext);
```

### Key Verification

```cpp
bool verify_keypair(const ColorPublicKey& public_key,
                   const ColorPrivateKey& private_key) const;
```

**Parameters:**
- `public_key`: Public key to verify
- `private_key`: Corresponding private key

**Returns:**
- `true` if keypair is valid, `false` otherwise

**Example:**
```cpp
if (kem.verify_keypair(public_key, private_key)) {
    std::cout << "Keypair is valid" << std::endl;
}
```

### Parameters Access

```cpp
const CLWEParameters& params() const;
```

**Returns:**
- Reference to current cryptographic parameters

**Example:**
```cpp
const auto& params = kem.params();
std::cout << "Security level: " << params.security_level << std::endl;
```

## Key Structures

### ColorPublicKey

```cpp
struct ColorPublicKey {
    std::array<uint8_t, 32> seed;      // Matrix generation seed
    std::vector<uint8_t> public_data;  // Serialized public key data
    CLWEParameters params;             // Associated parameters

    std::vector<uint8_t> serialize() const;
    static ColorPublicKey deserialize(const std::vector<uint8_t>& data);
};
```

**Members:**
- `seed`: 32-byte seed for matrix generation
- `public_data`: Serialized polynomial data
- `params`: Cryptographic parameters

**Methods:**
- `serialize()`: Convert to byte array for storage/transmission
- `deserialize(data)`: Reconstruct from byte array

### ColorPrivateKey

```cpp
struct ColorPrivateKey {
    std::vector<uint8_t> secret_data;  // Serialized private key data
    CLWEParameters params;             // Associated parameters

    std::vector<uint8_t> serialize() const;
    static ColorPrivateKey deserialize(const std::vector<uint8_t>& data);
};
```

**Members:**
- `secret_data`: Serialized polynomial data
- `params`: Cryptographic parameters

**Methods:**
- `serialize()`: Convert to byte array for storage
- `deserialize(data)`: Reconstruct from byte array

### ColorCiphertext

```cpp
struct ColorCiphertext {
    std::vector<uint8_t> ciphertext_data;     // Encrypted data
    std::vector<uint8_t> shared_secret_hint;  // Hint for decapsulation
    CLWEParameters params;                    // Associated parameters

    std::vector<uint8_t> serialize() const;
    static ColorCiphertext deserialize(const std::vector<uint8_t>& data);
};
```

**Members:**
- `ciphertext_data`: Encrypted polynomial data
- `shared_secret_hint`: Additional data for FO transform
- `params`: Cryptographic parameters

**Methods:**
- `serialize()`: Convert to byte array for transmission
- `deserialize(data)`: Reconstruct from byte array

## ColorValue

Fundamental color value type for cryptographic operations.

### Constructors

```cpp
ColorValue();  // Default constructor
explicit ColorValue(std::vector<uint32_t> coeffs);  // From coefficients
static ColorValue from_precise_value(uint32_t value);  // From single value
```

### Arithmetic Operations

```cpp
ColorValue operator+(const ColorValue& other) const;
ColorValue operator-(const ColorValue& other) const;
ColorValue operator*(const ColorValue& other) const;
ColorValue operator/(const ColorValue& other) const;  // Modular inverse
```

### Utility Functions

```cpp
uint32_t to_precise_value() const;  // Convert to single value
std::vector<uint32_t> coefficients() const;  // Get all coefficients
size_t degree() const;  // Get polynomial degree
```

### NTT Operations

```cpp
void ntt_forward();   // Forward NTT transform
void ntt_inverse();   // Inverse NTT transform
bool is_ntt_form() const;  // Check if in NTT domain
```

## Utility Functions

### Global Functions

```cpp
std::string get_error_message(CLWEError error);
```

Convert error code to human-readable string.

**Parameters:**
- `error`: Error code to convert

**Returns:**
- Descriptive error message

**Example:**
```cpp
clwe::CLWEError err = clwe::CLWEError::INVALID_PARAMETERS;
std::cout << clwe::get_error_message(err) << std::endl;
// Output: "Invalid parameters"
```

### Color Conversion Utilities

```cpp
static std::vector<uint8_t> color_secret_to_bytes(const ColorValue& secret);
static ColorValue bytes_to_color_secret(const std::vector<uint8_t>& bytes);
```

Convert between ColorValue and byte representations.

**Parameters:**
- `secret`: Color value to convert
- `bytes`: Byte array to convert

**Returns:**
- Converted value

## Error Handling

### Exception Types

Cryptopix-CLWE uses standard C++ exceptions:

- `std::invalid_argument`: Invalid input parameters
- `std::bad_alloc`: Memory allocation failure
- `std::runtime_error`: Runtime operation failure

### Error Checking Pattern

```cpp
try {
    clwe::ColorKEM kem(params);
    auto [pk, sk] = kem.keygen();

    // Operations that may fail
    auto [ct, ss] = kem.encapsulate(pk);
    auto recovered = kem.decapsulate(pk, sk, ct);

} catch (const std::invalid_argument& e) {
    std::cerr << "Invalid argument: " << e.what() << std::endl;
} catch (const std::bad_alloc& e) {
    std::cerr << "Memory allocation failed: " << e.what() << std::endl;
} catch (const std::runtime_error& e) {
    std::cerr << "Runtime error: " << e.what() << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
}
```

## Memory Management

### Automatic Resource Management

All classes follow RAII principles:

```cpp
{
    clwe::ColorKEM kem;  // Resources allocated
    // Use kem...
}  // Resources automatically freed
```

### Secure Memory

Sensitive data is automatically zeroed on destruction:

```cpp
class ColorPrivateKey {
    // ...
    ~ColorPrivateKey() {
        // Secure cleanup
        memset(secret_data.data(), 0, secret_data.size());
    }
};
```

## Thread Safety

### Thread Safety Guarantees

- **ColorKEM**: Not thread-safe, use one instance per thread
- **Key objects**: Immutable after construction, thread-safe for reading
- **Static functions**: Thread-safe

### Threading Example

```cpp
#include <thread>
#include <vector>

void worker_thread(const clwe::ColorPublicKey& pk) {
    clwe::ColorKEM local_kem;  // Each thread has its own instance

    for (int i = 0; i < 100; ++i) {
        auto [ct, ss] = local_kem.encapsulate(pk);
        // Process results...
    }
}

int main() {
    clwe::ColorKEM kem;
    auto [pk, sk] = kem.keygen();

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker_thread, std::ref(pk));
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
```

## Constants and Limits

### Size Limits

| Component | 128-bit | 192-bit | 256-bit | Unit |
|-----------|---------|---------|---------|------|
| Public Key | 768 | 1152 | 1536 | bytes |
| Private Key | 768 | 1152 | 1536 | bytes |
| Ciphertext | 992 | 1376 | 1760 | bytes |
| Shared Secret | 4 | 4 | 4 | bytes |

### Performance Limits

| Operation | Min Time | Max Time | Typical Time |
|-----------|----------|----------|--------------|
| Key Generation | 50 μs | 500 μs | 100-200 μs |
| Encapsulation | 50 μs | 500 μs | 100-200 μs |
| Decapsulation | 1 μs | 10 μs | 2-5 μs |

*Times measured on Apple M4, may vary by platform*

## Examples

### Complete Key Exchange

```cpp
#include <clwe/clwe.hpp>
#include <clwe/color_kem.hpp>
#include <iostream>

int main() {
    // Initialize KEM instances
    clwe::ColorKEM alice_kem(clwe::CLWEParameters(128));
    clwe::ColorKEM bob_kem(clwe::CLWEParameters(128));

    // Key generation
    auto [alice_pk, alice_sk] = alice_kem.keygen();
    auto [bob_pk, bob_sk] = bob_kem.keygen();

    // Alice encapsulates for Bob
    auto [ciphertext, alice_secret] = alice_kem.encapsulate(bob_pk);

    // Bob decapsulates
    clwe::ColorValue bob_secret = bob_kem.decapsulate(alice_pk, bob_sk, ciphertext);

    // Verify shared secret
    if (alice_secret.to_precise_value() == bob_secret.to_precise_value()) {
        std::cout << "Key exchange successful!" << std::endl;
        std::cout << "Shared secret: " << alice_secret.to_precise_value() << std::endl;
    } else {
        std::cout << "Key exchange failed!" << std::endl;
        return 1;
    }

    return 0;
}
```

### Hybrid Encryption

```cpp
#include <clwe/color_kem.hpp>
#include <openssl/evp.h>
#include <vector>

std::pair<std::vector<uint8_t>, clwe::ColorCiphertext>
encrypt_hybrid(const clwe::ColorKEM& kem,
               const clwe::ColorPublicKey& recipient_key,
               const std::string& plaintext) {

    // KEM encapsulation
    auto [kem_ct, shared_secret] = kem.encapsulate(recipient_key);

    // Derive AES key from shared secret
    std::vector<uint8_t> shared_bytes = clwe::ColorKEM::color_secret_to_bytes(shared_secret);
    std::vector<uint8_t> aes_key(32);  // 256-bit key

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
    EVP_PKEY_derive_init(ctx);
    EVP_PKEY_CTX_set_hkdf_md(ctx, EVP_sha256());
    EVP_PKEY_CTX_set1_hkdf_key(ctx, shared_bytes.data(), shared_bytes.size());
    EVP_PKEY_CTX_set1_hkdf_info(ctx, (const unsigned char*)"aes-key", 7);
    size_t key_len = 32;
    EVP_PKEY_derive(ctx, aes_key.data(), &key_len);
    EVP_PKEY_CTX_free(ctx);

    // AES-GCM encryption
    EVP_CIPHER_CTX* cipher_ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(cipher_ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);

    std::vector<uint8_t> iv(12);
    RAND_bytes(iv.data(), iv.size());

    EVP_EncryptInit_ex(cipher_ctx, nullptr, nullptr, aes_key.data(), iv.data());

    std::vector<uint8_t> ciphertext(plaintext.size());
    int out_len;
    EVP_EncryptUpdate(cipher_ctx, ciphertext.data(), &out_len,
                     (const unsigned char*)plaintext.data(), plaintext.size());

    std::vector<uint8_t> auth_tag(16);
    EVP_CIPHER_CTX_ctrl(cipher_ctx, EVP_CTRL_GCM_GET_TAG, 16, auth_tag.data());

    EVP_CIPHER_CTX_free(cipher_ctx);

    // Combine: IV + ciphertext + auth_tag
    std::vector<uint8_t> encrypted;
    encrypted.insert(encrypted.end(), iv.begin(), iv.end());
    encrypted.insert(encrypted.end(), ciphertext.begin(), ciphertext.begin() + out_len);
    encrypted.insert(encrypted.end(), auth_tag.begin(), auth_tag.end());

    return {encrypted, kem_ct};
}

std::string decrypt_hybrid(const clwe::ColorKEM& kem,
                          const clwe::ColorPublicKey& sender_key,
                          const clwe::ColorPrivateKey& recipient_key,
                          const clwe::ColorCiphertext& kem_ct,
                          const std::vector<uint8_t>& encrypted) {

    // KEM decapsulation
    clwe::ColorValue shared_secret = kem.decapsulate(sender_key, recipient_key, kem_ct);

    // Derive AES key
    std::vector<uint8_t> shared_bytes = clwe::ColorKEM::color_secret_to_bytes(shared_secret);
    std::vector<uint8_t> aes_key(32);

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
    EVP_PKEY_derive_init(ctx);
    EVP_PKEY_CTX_set_hkdf_md(ctx, EVP_sha256());
    EVP_PKEY_CTX_set1_hkdf_key(ctx, shared_bytes.data(), shared_bytes.size());
    EVP_PKEY_CTX_set1_hkdf_info(ctx, (const unsigned char*)"aes-key", 7);
    size_t key_len = 32;
    EVP_PKEY_derive(ctx, aes_key.data(), &key_len);
    EVP_PKEY_CTX_free(ctx);

    // Parse encrypted data
    std::vector<uint8_t> iv(encrypted.begin(), encrypted.begin() + 12);
    std::vector<uint8_t> auth_tag(encrypted.end() - 16, encrypted.end());
    std::vector<uint8_t> ciphertext(encrypted.begin() + 12, encrypted.end() - 16);

    // AES-GCM decryption
    EVP_CIPHER_CTX* cipher_ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(cipher_ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_DecryptInit_ex(cipher_ctx, nullptr, nullptr, aes_key.data(), iv.data());

    std::string plaintext(ciphertext.size(), '\0');
    int out_len;
    EVP_DecryptUpdate(cipher_ctx, (unsigned char*)plaintext.data(), &out_len,
                     ciphertext.data(), ciphertext.size());

    EVP_CIPHER_CTX_ctrl(cipher_ctx, EVP_CTRL_GCM_SET_TAG, 16, auth_tag.data());

    int ret = EVP_DecryptFinal_ex(cipher_ctx, nullptr, nullptr);
    EVP_CIPHER_CTX_free(cipher_ctx);

    if (ret <= 0) {
        throw std::runtime_error("Authentication failed");
    }

    plaintext.resize(out_len);
    return plaintext;
}
```

### Benchmarking

```cpp
#include <clwe/color_kem.hpp>
#include <chrono>
#include <iostream>

void benchmark_kem(const clwe::CLWEParameters& params, int iterations = 1000) {
    clwe::ColorKEM kem(params);
    auto [pk, sk] = kem.keygen();

    // Benchmark key generation
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto [test_pk, test_sk] = kem.keygen();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto keygen_time = std::chrono::duration_cast<std::chrono::microseconds>
                      (end - start).count() / static_cast<double>(iterations);

    // Benchmark encapsulation
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto [ct, ss] = kem.encapsulate(pk);
    }
    end = std::chrono::high_resolution_clock::now();
    auto encap_time = std::chrono::duration_cast<std::chrono::microseconds>
                     (end - start).count() / static_cast<double>(iterations);

    // Benchmark decapsulation
    auto [test_ct, test_ss] = kem.encapsulate(pk);
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto recovered = kem.decapsulate(pk, sk, test_ct);
    }
    end = std::chrono::high_resolution_clock::now();
    auto decap_time = std::chrono::duration_cast<std::chrono::microseconds>
                     (end - start).count() / static_cast<double>(iterations);

    std::cout << "Benchmark Results (" << params.security_level << "-bit, "
              << iterations << " iterations):" << std::endl;
    std::cout << "Key Generation: " << keygen_time << " μs" << std::endl;
    std::cout << "Encapsulation:  " << encap_time << " μs" << std::endl;
    std::cout << "Decapsulation:  " << decap_time << " μs" << std::endl;
    std::cout << "Total KEM time: " << (keygen_time + encap_time + decap_time) << " μs" << std::endl;
    std::cout << "Throughput: " << (1000000.0 / (keygen_time + encap_time + decap_time))
              << " ops/s" << std::endl;
}

int main() {
    benchmark_kem(clwe::CLWEParameters(128));
    benchmark_kem(clwe::CLWEParameters(192));
    benchmark_kem(clwe::CLWEParameters(256));
    return 0;
}
```

---

*API version: 1.0.0*
*Last updated: 2025-11-18*