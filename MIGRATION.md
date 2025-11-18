# Migration Guide

This guide helps existing users of Cryptopix-CLWE migrate to the new universal C++ library version with package manager support.

## Overview

Version 1.0.0 introduces significant changes to make CLWE a universal C++ library that can be easily integrated via popular package managers like vcpkg, Conan, and direct CMake integration. The core cryptographic functionality remains unchanged, but the build system and integration methods have been enhanced.

## What's Changed

### Library Name
- **Old**: `cryptopix-clwe` (internal project name)
- **New**: `CLWE` (universal library name for package managers)

### Build System
- Enhanced CMake configuration with proper package installation
- Added CMake package configuration files (`CLWEConfig.cmake`)
- Improved cross-platform and cross-architecture support

### Package Manager Support
- **New**: vcpkg port for Windows/Linux/macOS
- **New**: Conan recipe for cross-platform dependency management
- **New**: pkg-config support for direct system integration

## Migration Steps

### For Existing CMake Users

#### Before (Manual Integration)
```cmake
# Old way - manual source integration
add_subdirectory(external/cryptopix-clwe)
target_link_libraries(your_target cryptopix_clwe)
```

#### After (Package Manager Integration)
```cmake
# New way - using find_package
find_package(CLWE CONFIG REQUIRED)
target_link_libraries(your_target CLWE::clwe)
```

### For Existing Direct Users

#### Before
```cpp
// Old include paths
#include "cryptopix-clwe/include/clwe/clwe.hpp"
#include "cryptopix-clwe/include/clwe/color_kem.hpp"

// Old linking
// -lcryptopix-clwe
```

#### After
```cpp
// New include paths
#include <clwe/clwe.hpp>
#include <clwe/color_kem.hpp>

// New linking
// -lclwe_avx
```

### API Changes

The core API remains largely compatible, but some changes have been made for consistency:

#### Namespace Changes
```cpp
// Before
using namespace cryptopix_clwe;

// After
using namespace clwe;
```

#### Parameter Structure Changes
```cpp
// Before
CLWEParameters params(128);  // Direct constructor

// After (unchanged - still works)
CLWEParameters params(128);  // Constructor with defaults
```

#### Key Structure Changes
```cpp
// Before
struct KEMPublicKey { /* internal structure */ };

// After (API compatible)
struct KEMPublicKey { /* same interface */ };
```

## Installation Methods

### Option 1: vcpkg (Recommended for Windows)

```bash
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg && bootstrap-vcpkg.bat

# Install CLWE
vcpkg install clwe

# Use in CMake
find_package(CLWE CONFIG REQUIRED)
target_link_libraries(your_target CLWE::clwe)
```

### Option 2: Conan (Cross-platform)

```bash
# Install Conan
pip install conan

# Create conanfile.txt
[requires]
clwe/1.0.0@your-org/stable

[generators]
cmake_find_package

# Install and use
conan install . --build missing
```

### Option 3: Direct CMake Integration

```cmake
# Add as submodule
git submodule add https://github.com/your-org/cryptopix-clwe.git external/clwe
git submodule update --init --recursive

# In CMakeLists.txt
add_subdirectory(external/clwe)
target_link_libraries(your_target CLWE::clwe)
```

### Option 4: System Installation

```bash
# Build and install
git clone https://github.com/your-org/cryptopix-clwe.git
cd cryptopix-clwe
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install

# Use in projects
find_package(CLWE CONFIG REQUIRED)
target_link_libraries(your_target CLWE::clwe)
```

## Breaking Changes

### Build System Changes

1. **CMake Version Requirement**
   - **Old**: CMake 3.10+
   - **New**: CMake 3.16+ (for modern package configuration)

2. **Library Name**
   - **Old**: `cryptopix_clwe` target
   - **New**: `CLWE::clwe` imported target

3. **Installation Directories**
   - **Old**: Manual installation
   - **New**: Standard CMake package installation with config files

### File Structure Changes

```
cryptopix-clwe/
├── CMakeLists.txt           # Enhanced with package config
├── cmake/
│   ├── CLWEConfig.cmake.in  # NEW: Package configuration
│   └── clwe.pc.in          # NEW: pkg-config support
├── conanfile.py            # NEW: Conan recipe
├── vcpkg/                  # NEW: vcpkg port
├── scripts/                # NEW: Build and release scripts
├── LICENSE                 # NEW: Apache 2.0 license
├── AUTHORS                 # NEW: Author information
├── CHANGELOG.md           # NEW: Version history
├── INSTALL.md             # NEW: Installation guide
└── MIGRATION.md           # NEW: This file
```

## Compatibility Matrix

| Feature | Before | After | Status |
|---------|--------|-------|--------|
| Core Crypto API | ✅ | ✅ | Compatible |
| Key Generation | ✅ | ✅ | Compatible |
| Encapsulation | ✅ | ✅ | Compatible |
| Decapsulation | ✅ | ✅ | Compatible |
| SIMD Support | ✅ | ✅ | Enhanced |
| CMake Integration | ⚠️ | ✅ | Improved |
| Package Managers | ❌ | ✅ | New Feature |
| Cross-platform | ✅ | ✅ | Enhanced |

## Testing Migration

### Automated Testing
```bash
# Run the new build script
./scripts/build.sh --enable-tests

# Test package installation
./scripts/build.sh install
```

### Manual Testing
```cpp
// Test basic functionality
#include <clwe/clwe.hpp>
#include <clwe/color_kem.hpp>

int main() {
    clwe::ColorKEM kem{clwe::CLWEParameters(128)};

    // Generate key pair
    auto [public_key, private_key] = kem.keygen();

    // Test encapsulation/decapsulation
    auto [ciphertext, shared_secret] = kem.encapsulate(public_key);
    auto recovered_secret = kem.decapsulate(public_key, private_key, ciphertext);

    // Verify
    assert(shared_secret.to_precise_value() == recovered_secret.to_precise_value());

    return 0;
}
```

### Benchmark Comparison
```bash
# Run benchmarks with new version
./build/benchmark_color_kem_timing

# Compare with old version benchmarks
# Performance should be similar or improved
```

## Troubleshooting

### CMake Find Package Issues
```bash
# Check if CLWE is installed
find /usr/local/lib/cmake/CLWE -name "*.cmake"

# Update CMake package cache
cmake --regenerate

# Check pkg-config
pkg-config --exists clwe
```

### Linker Errors
```
undefined reference to 'clwe_avx'
```
**Solution**: Update library linking
```cmake
# Old
target_link_libraries(your_target cryptopix_clwe)

# New
find_package(CLWE CONFIG REQUIRED)
target_link_libraries(your_target CLWE::clwe)
```

### Include Path Issues
```
fatal error: clwe/clwe.hpp: No such file or directory
```
**Solution**: Update include paths or installation
```cmake
# Ensure proper include directories
find_package(CLWE CONFIG REQUIRED)
# This automatically sets up include paths
```

### SIMD Detection Issues
If you encounter runtime SIMD errors:
```bash
# Check CPU capabilities
grep -E "(avx|neon|rvv|vsx)" /proc/cpuinfo

# Rebuild with specific SIMD support
cmake .. -DENABLE_AVX=OFF  # Disable if not supported
```

## Performance Considerations

### Build Optimizations
- **New**: Automatic SIMD detection and optimization
- **New**: Profile-guided optimization support
- **New**: Link-time optimization (LTO) support

### Runtime Performance
- Core cryptographic performance unchanged
- SIMD optimizations enhanced
- Memory management improved

## Support and Help

### Getting Help
1. Check this migration guide
2. Review the new [INSTALL.md](INSTALL.md)
3. Check [GitHub Issues](https://github.com/your-org/cryptopix-clwe/issues)
4. Create new issues for migration problems

### Reporting Issues
When reporting migration issues, please include:
- Old version and integration method
- New version and target package manager
- Error messages and logs
- Platform and compiler information
- Steps to reproduce

## Timeline

- **Phase 1** (Completed): Core library enhancements
- **Phase 2** (Completed): Package manager integration
- **Phase 3** (Ongoing): User migration and testing
- **Phase 4** (Future): Advanced features and optimizations

## Future Compatibility

The new universal library architecture ensures:
- **Forward Compatibility**: Future versions will maintain API compatibility
- **Package Manager Support**: Continued support for vcpkg, Conan, and other managers
- **Cross-platform**: Enhanced support for new platforms and architectures
- **Security**: Regular updates with latest cryptographic best practices

---

*This migration guide will be updated as new features are added and issues are resolved.*