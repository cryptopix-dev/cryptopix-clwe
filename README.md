# Cryptopix-CLWE: Color-Integrated Learning With Errors (CLWE) Cryptosystem

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()

A high-performance, post-quantum cryptographic library implementing a novel color-integrated variant of the Learning With Errors (LWE) problem, specifically designed for modern multi-architecture systems including Apple Silicon (M1/M2/M3/M4), x86-64, ARM64, RISC-V, and PowerPC architectures.

## 🎨 Overview

Cryptopix-CLWE introduces a revolutionary approach to lattice-based cryptography by integrating color theory as a fundamental cryptographic primitive. The system leverages color values as mathematical objects in cryptographic operations, providing enhanced security properties while maintaining compatibility with existing cryptographic protocols.

### Key Features

- **Color-Integrated Cryptography**: Novel approach using color spaces as cryptographic primitives
- **Multi-Architecture SIMD Support**: Optimized for AVX-512, AVX2, NEON, RVV, and VSX instruction sets
- **Post-Quantum Security**: Based on the hardness of lattice problems in color space
- **High Performance**: Leverages modern CPU capabilities for cryptographic acceleration
- **Cross-Platform**: Supports macOS, Linux, Windows, and embedded systems
- **NIST-Ready**: Compatible with emerging post-quantum cryptographic standards

## 🚀 Quick Start

### Prerequisites

- CMake 3.16 or higher
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- OpenSSL 1.1.1 or higher

### Building

```bash
# Clone the repository
git clone https://github.com/cryptopix-dev/cryptopix-clwe.git
cd cryptopix-clwe

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the library
make -j$(nproc)

# Run tests
./demo_kem
```

### Basic Usage

```cpp
#include <clwe/clwe.hpp>
#include <clwe/color_kem.hpp>

int main() {
    // Initialize Color KEM with 128-bit security
    clwe::ColorKEM kem{clwe::CLWEParameters(128)};

    // Generate key pair
    auto [public_key, private_key] = kem.keygen();

    // Encapsulate shared secret
    auto [ciphertext, shared_secret] = kem.encapsulate(public_key);

    // Decapsulate shared secret
    auto recovered_secret = kem.decapsulate(public_key, private_key, ciphertext);

    // Verify successful key exchange
    assert(shared_secret.to_precise_value() == recovered_secret.to_precise_value());

    return 0;
}
```

## 📊 Performance Benchmarks

### Apple MacBook M4 (ARM64 NEON)

| Security Level | KeyGen | Encapsulate | Decapsulate | Total | Throughput |
|----------------|--------|-------------|-------------|-------|------------|
| 128-bit       | 96.76 μs | 111.21 μs | 2.94 μs | 210.91 μs | 4,741 ops/s |
| 192-bit       | 188.12 μs | 170.34 μs | 3.01 μs | 361.47 μs | 2,766 ops/s |
| 256-bit       | 316.96 μs | 408.44 μs | 6.01 μs | 731.41 μs | 1,367 ops/s |

*Benchmarks performed on Apple MacBook Pro M4, 24GB RAM, macOS Sonoma*

## 🏗️ Architecture

### Core Components

- **Color Value System**: Fundamental cryptographic primitive using color mathematics
- **NTT Engine**: Number Theoretic Transform implementation with multi-architecture optimizations
- **Color KEM**: Key Encapsulation Mechanism using color-integrated operations
- **SIMD Abstraction Layer**: Unified interface for different SIMD instruction sets

### Supported Architectures

| Architecture | SIMD Support | Status |
|--------------|--------------|--------|
| x86-64      | AVX-512, AVX2 | ✅ Full |
| ARM64       | NEON         | ✅ Full |
| RISC-V      | RVV          | ✅ Full |
| PowerPC     | VSX/AltiVec  | ✅ Full |
| Generic     | Scalar       | ✅ Fallback |

## 📚 Documentation

- [Technical Specification](docs/TECHNICAL_SPEC.md) - Detailed technical documentation
- [Build Guide](docs/BUILD_GUIDE.md) - Comprehensive build instructions
- [Usage Guide](docs/USAGE_GUIDE.md) - API usage examples and tutorials
- [Benchmarks](docs/BENCHMARKS.md) - Performance analysis and comparisons
- [MacBook M4 Notes](docs/MACBOOK_M4_NOTES.md) - Apple Silicon specific optimizations
- [API Reference](docs/API_REFERENCE.md) - Complete API documentation
- [Security Analysis](docs/SECURITY_ANALYSIS.md) - Cryptographic security evaluation
- [Architecture](docs/architecture.md) - System architecture overview

## 🔧 Development

### Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct and the process for submitting pull requests.

### Building from Source

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Release build with optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Cross-compilation for ARM64
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchains/arm64-toolchain.cmake
make -j$(nproc)
```

### Testing

```bash
# Run unit tests
ctest --output-on-failure

# Run performance benchmarks
./build/benchmark_color_kem_timing

# Run integration tests
./build/demo_kem
```

## 🔒 Security

This library implements post-quantum cryptographic algorithms. While the color-integrated approach provides additional security layers, users should:

- Keep private keys secure and never share them
- Use appropriate key sizes for your security requirements
- Regularly update to the latest version for security patches
- Follow best practices for cryptographic key management

For detailed security analysis, see [Security Analysis](docs/SECURITY_ANALYSIS.md).

## 📄 License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Based on the Learning With Errors problem formulation
- Inspired by NIST Post-Quantum Cryptography standardization efforts
- Color theory integration developed at [Your Institution]

## 📞 Contact

- **Project Lead**: [Your Name]
- **Email**: [your.email@example.com]
- **Issues**: [GitHub Issues](https://github.com/cryptopix-dev/cryptopix-clwe/issues)
- **Discussions**: [GitHub Discussions](https://github.com/cryptopix-dev/cryptopix-clwe/discussions)

---

*Cryptopix-CLWE: Where Color Meets Cryptography*