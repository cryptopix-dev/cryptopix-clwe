#ifndef CLWE_HPP
#define CLWE_HPP

#include <cstdint>
#include <string>

// Forward declarations
struct CLWEParameters;

// Main CLWE namespace
namespace clwe {

// Version information
const std::string VERSION = "1.0.0";

// Parameter structure for CLWE operations
struct CLWEParameters {
    uint32_t security_level;  // Security level in bits (128, 192, 256)
    uint32_t degree;          // Ring degree (power of 2)
    uint32_t module_rank;     // Module rank k
    uint32_t modulus;         // Prime modulus q
    uint32_t eta;            // Binomial distribution parameter
    uint32_t beta;           // Signature bound parameter

    // Constructor with defaults - NIST-standard parameters
    CLWEParameters(uint32_t sec_level = 128)
        : security_level(sec_level), degree(256), module_rank(2),
          modulus(3329), eta(2), beta(120) {  // Kyber modulus: 2^12 + 1
        // Set parameters based on security level (Kyber-inspired)
        switch (sec_level) {
            case 128:  // Kyber-512 equivalent
                degree = 256;
                module_rank = 2;
                modulus = 3329;  // q = 2^12 + 1
                eta = 2;
                beta = 120;
                break;
            case 192:  // Kyber-768 equivalent
                degree = 256;
                module_rank = 3;
                modulus = 3329;
                eta = 2;
                beta = 200;
                break;
            case 256:  // Kyber-1024 equivalent
                degree = 256;
                module_rank = 4;
                modulus = 3329;
                eta = 2;
                beta = 280;
                break;
            default:
                // Keep defaults
                break;
        }
    }
};

// Error codes
enum class CLWEError {
    SUCCESS = 0,
    INVALID_PARAMETERS = 1,
    MEMORY_ALLOCATION_FAILED = 2,
    AVX_NOT_SUPPORTED = 3,
    INVALID_KEY = 4,
    VERIFICATION_FAILED = 5,
    UNKNOWN_ERROR = 6
};

// Utility function to get error message
std::string get_error_message(CLWEError error);

} // namespace clwe

#endif // CLWE_HPP