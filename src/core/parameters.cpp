#include "clwe/clwe.hpp"
#include <string>

namespace clwe {

// Utility function to get error message
std::string get_error_message(CLWEError error) {
    switch (error) {
        case CLWEError::SUCCESS:
            return "Success";
        case CLWEError::INVALID_PARAMETERS:
            return "Invalid parameters";
        case CLWEError::MEMORY_ALLOCATION_FAILED:
            return "Memory allocation failed";
        case CLWEError::AVX_NOT_SUPPORTED:
            return "AVX not supported on this CPU";
        case CLWEError::INVALID_KEY:
            return "Invalid key";
        case CLWEError::VERIFICATION_FAILED:
            return "Verification failed";
        case CLWEError::UNKNOWN_ERROR:
        default:
            return "Unknown error";
    }
}

} // namespace clwe