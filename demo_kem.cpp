#include "clwe/clwe.hpp"
#include "src/core/color_kem.hpp"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace clwe;

void print_hex(const std::string& label, const std::vector<uint8_t>& data) {
    std::cout << label << ": ";
    for (size_t i = 0; i < std::min(data.size(), size_t(32)); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(data[i]);
    }
    if (data.size() > 32) std::cout << "...";
    std::cout << std::dec << " (" << data.size() << " bytes)" << std::endl;
}

void print_color_value(const std::string& label, const ColorValue& value) {
    uint32_t val = value.to_precise_value();
    std::cout << label << ": " << val << " (0x" << std::hex << val << std::dec << ")" << std::endl;
}

int main() {
    std::cout << "🎨 CLWE Color KEM Demonstration" << std::endl;
    std::cout << "=================================" << std::endl;

    // Initialize CLWE parameters
    clwe::CLWEParameters params(128);  // 128-bit security
    std::cout << "Security Level: " << params.security_level << "-bit" << std::endl;
    std::cout << "Polynomial Degree: " << params.degree << std::endl;
    std::cout << "Modulus: " << params.modulus << std::endl;
    std::cout << "Module Rank: " << params.module_rank << std::endl;
    std::cout << std::endl;

    // Create Color KEM instance
    ColorKEM kem(params);

    // Step 1: Key Generation
    std::cout << "🔑 Step 1: Key Generation" << std::endl;
    auto [public_key, private_key] = kem.keygen();

    std::cout << "Public Key Matrix Seed: ";
    for (uint8_t byte : public_key.seed) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(byte);
    }
    std::cout << std::dec << std::endl;

    print_hex("Public Key Data", public_key.public_data);
    print_hex("Private Key Data", private_key.secret_data);
    std::cout << std::endl;

    // Step 2: Encapsulation
    std::cout << "📦 Step 2: Encapsulation" << std::endl;
    auto [ciphertext, shared_secret] = kem.encapsulate(public_key);

    print_color_value("Shared Secret (Sender)", shared_secret);
    print_hex("Ciphertext Data", ciphertext.ciphertext_data);
    print_hex("Shared Secret Hint", ciphertext.shared_secret_hint);
    std::cout << std::endl;

    // Step 3: Decapsulation
    std::cout << "🔓 Step 3: Decapsulation" << std::endl;
    ColorValue recovered_secret = kem.decapsulate(public_key, private_key, ciphertext);

    print_color_value("Shared Secret (Receiver)", recovered_secret);

    // Step 4: Verification
    std::cout << "✅ Step 4: Verification" << std::endl;
    uint32_t original_val = shared_secret.to_precise_value();
    uint32_t recovered_val = recovered_secret.to_precise_value();

    bool success = (original_val == recovered_val);
    std::cout << "Key Exchange Success: " << (success ? "✅ YES" : "❌ NO") << std::endl;

    if (success) {
        std::cout << "🎉 CLWE Color KEM working perfectly!" << std::endl;
        std::cout << "Both parties now share the secret: " << original_val << std::endl;
    } else {
        std::cout << "❌ Key exchange failed!" << std::endl;
        std::cout << "Expected: " << original_val << ", Got: " << recovered_val << std::endl;
    }

    std::cout << std::endl;
    std::cout << "🔐 Security Features Demonstrated:" << std::endl;
    std::cout << "  • 128-bit quantum-resistant encryption" << std::endl;
    std::cout << "  • Color-integrated cryptographic primitives" << std::endl;
    std::cout << "  • SHAKE-based deterministic key generation" << std::endl;
    std::cout << "  • Proper Kyber reconciliation" << std::endl;
    std::cout << "  • Multi-architecture SIMD acceleration" << std::endl;

    return success ? 0 : 1;
}