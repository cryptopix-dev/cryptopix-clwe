# Contributing to Cryptopix-CLWE

Thank you for your interest in contributing to Cryptopix-CLWE! This document provides guidelines and information for contributors.

## 📋 Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Contributing Guidelines](#contributing-guidelines)
- [Pull Request Process](#pull-request-process)
- [Testing](#testing)
- [Documentation](#documentation)
- [Security](#security)

## 🤝 Code of Conduct

This project adheres to a code of conduct to ensure a welcoming environment for all contributors. By participating, you agree to:

- Be respectful and inclusive
- Focus on constructive feedback
- Accept responsibility for mistakes
- Show empathy towards other contributors
- Help create a positive community

## 🚀 Getting Started

### Prerequisites

- C++17 compatible compiler
- CMake 3.16+
- Git
- OpenSSL development libraries

### Fork and Clone

1. Fork the repository on GitHub
2. Clone your fork locally:
   ```bash
   git clone https://github.com/your-username/cryptopix-clwe.git
   cd cryptopix-clwe
   ```
3. Add upstream remote:
   ```bash
   git remote add upstream https://github.com/original-org/cryptopix-clwe.git
   ```

## 🛠️ Development Setup

### Building for Development

```bash
# Create build directory
mkdir build && cd build

# Configure with debug symbols and tests
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON

# Build
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

### IDE Setup

#### Visual Studio Code
- Install C++ extension
- Use CMake Tools extension for building
- Configure IntelliSense for C++17

#### CLion
- Import as CMake project
- Configure toolchain for your platform
- Enable C++17 standard

## 📝 Contributing Guidelines

### Code Style

- **Language**: C++17 standard
- **Formatting**: Use clang-format with the provided `.clang-format` file
- **Naming**: Use `snake_case` for variables/functions, `PascalCase` for classes/types
- **Comments**: Use `//` for single-line comments, `/* */` for multi-line
- **Documentation**: Use Doxygen-style comments for public APIs

### Commit Messages

Follow conventional commit format:
```
type(scope): description

[optional body]

[optional footer]
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `style`: Code style changes
- `refactor`: Code refactoring
- `test`: Testing
- `chore`: Maintenance

Examples:
```
feat(color_kem): add support for custom color spaces
fix(ntt_engine): resolve memory leak in AVX implementation
docs(api): update parameter documentation
```

### Branch Naming

- `feature/description`: New features
- `bugfix/issue-description`: Bug fixes
- `hotfix/critical-fix`: Critical fixes
- `docs/update-documentation`: Documentation updates

## 🔄 Pull Request Process

1. **Create Feature Branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make Changes**
   - Write clear, focused commits
   - Add tests for new functionality
   - Update documentation
   - Ensure code compiles and tests pass

3. **Run Quality Checks**
   ```bash
   # Format code
   clang-format -i src/**/*.cpp src/**/*.hpp

   # Build and test
   cd build
   make -j$(nproc)
   ctest --output-on-failure

   # Run benchmarks
   ./benchmark_color_kem_timing
   ```

4. **Update Documentation**
   - Update relevant documentation files
   - Add API documentation for new functions
   - Update benchmarks if performance changes

5. **Create Pull Request**
   - Use descriptive title and detailed description
   - Reference related issues
   - Include screenshots/demo for UI changes
   - Request review from maintainers

### PR Checklist

- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] Documentation updated
- [ ] Benchmarks run successfully
- [ ] Code formatted with clang-format
- [ ] Commit messages follow conventional format
- [ ] PR description includes context and testing notes

## 🧪 Testing

### Unit Tests

Add unit tests for new functionality in the `tests/` directory. Use Google Test framework.

```cpp
TEST(ColorKEMTest, KeyGeneration) {
    CLWEParameters params(128);
    ColorKEM kem(params);

    auto [public_key, private_key] = kem.keygen();

    EXPECT_TRUE(kem.verify_keypair(public_key, private_key));
}
```

### Performance Tests

For performance-critical changes, add benchmarks:

```cpp
BENCHMARK(ColorKEMBenchmark, KeyGeneration128) {
    CLWEParameters params(128);
    ColorKEM kem(params);

    for (auto _ : state) {
        auto [public_key, private_key] = kem.keygen();
    }
}
```

### Running Tests

```bash
# Run all tests
ctest --output-on-failure

# Run specific test
ctest -R ColorKEMTest

# Run benchmarks
./benchmark_color_kem_timing
```

## 📚 Documentation

### API Documentation

Use Doxygen comments for public APIs:

```cpp
/**
 * @brief Generate a key pair for Color KEM
 *
 * @param params Cryptographic parameters
 * @return Pair of public and private keys
 *
 * @note This function uses cryptographically secure random generation
 */
std::pair<ColorPublicKey, ColorPrivateKey> keygen(const CLWEParameters& params);
```

### User Documentation

- Update relevant `.md` files in `docs/`
- Add examples in `USAGE_GUIDE.md`
- Update benchmarks in `BENCHMARKS.md`

## 🔒 Security

### Security Considerations

- **Cryptographic Review**: All cryptographic changes require security review
- **Side Channels**: Be aware of timing and cache side-channel attacks
- **Random Generation**: Use cryptographically secure random number generators
- **Input Validation**: Validate all inputs to prevent attacks

### Reporting Security Issues

- **DO NOT** create public GitHub issues for security vulnerabilities
- Email security issues to: security@cryptopix-clwe.org
- Include detailed reproduction steps and impact assessment

## 🎯 Areas for Contribution

### High Priority

- [ ] Additional SIMD optimizations (AVX-512, SVE)
- [ ] Hardware security module (HSM) integration
- [ ] Formal verification of cryptographic proofs
- [ ] Additional post-quantum algorithms

### Medium Priority

- [ ] Python bindings
- [ ] WebAssembly support
- [ ] GPU acceleration (CUDA/OpenCL)
- [ ] Additional test vectors

### Low Priority

- [ ] Alternative color space implementations
- [ ] Performance profiling tools
- [ ] Integration with popular crypto libraries

## 📞 Getting Help

- **Discussions**: Use [GitHub Discussions](https://github.com/org/cryptopix-clwe/discussions) for questions
- **Issues**: Report bugs via [GitHub Issues](https://github.com/org/cryptopix-clwe/issues)
- **Slack**: Join our community Slack (invite link in discussions)

## 🙏 Recognition

Contributors are recognized in:
- `CONTRIBUTORS.md` file
- Release notes
- GitHub contributor statistics

Thank you for contributing to Cryptopix-CLWE! 🎨🔐