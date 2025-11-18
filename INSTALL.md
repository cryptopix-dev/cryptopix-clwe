# Installation Guide

Cryptopix-CLWE supports multiple installation methods for different development environments and package management systems.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Package Manager Installation](#package-manager-installation)
  - [vcpkg (Windows/Linux/macOS)](#vcpkg-windowslinuxmacos)
  - [Conan (Cross-platform)](#conan-cross-platform)
- [Direct CMake Integration](#direct-cmake-integration)
- [Manual Installation](#manual-installation)
- [System Integration](#system-integration)
- [Platform-Specific Notes](#platform-specific-notes)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### Required Dependencies
- **C++ Compiler**: GCC 7+, Clang 5+, MSVC 2017+, or compatible C++17 compiler
- **CMake**: Version 3.16 or higher
- **OpenSSL**: Version 1.1.1 or higher (development headers required)
- **Git**: For cloning repositories

### System Requirements
- **RAM**: Minimum 4GB, recommended 8GB+
- **Disk Space**: 500MB for source and build artifacts
- **Supported Platforms**: Linux, macOS, Windows, and compatible Unix-like systems

## Quick Start

### Linux/macOS (System Package)
```bash
# Install dependencies
sudo apt install cmake libssl-dev build-essential  # Ubuntu/Debian
# OR
brew install cmake openssl                        # macOS

# Clone and build
git clone https://github.com/your-org/cryptopix-clwe.git
cd cryptopix-clwe
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### Windows (vcpkg)
```bash
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg && bootstrap-vcpkg.bat

# Install CLWE
vcpkg install clwe
```

## Package Manager Installation

### vcpkg (Windows/Linux/macOS)

vcpkg is Microsoft's cross-platform package manager that works on Windows, Linux, and macOS.

#### Installation
```bash
# Clone vcpkg (if not already installed)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
bootstrap-vcpkg.bat  # Windows
./bootstrap-vcpkg.sh  # Linux/macOS

# Install CLWE
./vcpkg install clwe
```

#### Integration with Visual Studio
```bash
# For Visual Studio integration
./vcpkg integrate install
```

#### Usage in CMake Projects
```cmake
find_package(CLWE CONFIG REQUIRED)
target_link_libraries(your_target CLWE::clwe)
```

#### Custom vcpkg Registry
If CLWE is not in the main vcpkg registry, add it to your custom registry:

```json
// vcpkg-configuration.json
{
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/your-org/cryptopix-clwe",
      "reference": "v1.0.0",
      "baseline": "main",
      "packages": ["clwe"]
    }
  ]
}
```

### Conan (Cross-platform)

Conan is a decentralized package manager that works across all platforms.

#### Installation
```bash
# Install Conan
pip install conan

# Add CLWE remote (if using custom repository)
conan remote add your-org https://your-org.jfrog.io/artifactory/api/conan/conan-local

# Install CLWE
conan install clwe/1.0.0@your-org/stable
```

#### Usage in CMake Projects
```cmake
# conanfile.txt
[requires]
clwe/1.0.0@your-org/stable

[generators]
cmake_find_package
```

```cmake
find_package(CLWE CONFIG REQUIRED)
target_link_libraries(your_target CLWE::clwe)
```

#### Profile Configuration
```bash
# Create a profile for optimized builds
conan profile new clwe-release --detect
conan profile update settings.build_type=Release clwe-release
conan profile update options.clwe:shared=False clwe-release
```

## Direct CMake Integration

For projects that prefer direct CMake integration without package managers.

### Add as Submodule
```bash
# Add as git submodule
git submodule add https://github.com/your-org/cryptopix-clwe.git external/clwe
git submodule update --init --recursive

# In your CMakeLists.txt
add_subdirectory(external/clwe)
target_link_libraries(your_target clwe_avx)
```

### FetchContent (CMake 3.11+)
```cmake
include(FetchContent)

FetchContent_Declare(
    clwe
    GIT_REPOSITORY https://github.com/your-org/cryptopix-clwe.git
    GIT_TAG v1.0.0
)

FetchContent_MakeAvailable(clwe)

target_link_libraries(your_target CLWE::clwe)
```

### External Project
```cmake
include(ExternalProject)

ExternalProject_Add(
    clwe
    GIT_REPOSITORY https://github.com/your-org/cryptopix-clwe.git
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)

add_dependencies(your_target clwe)
```

## Manual Installation

For systems without package managers or custom requirements.

### Build from Source
```bash
# Clone repository
git clone https://github.com/your-org/cryptopix-clwe.git
cd cryptopix-clwe

# Create build directory
mkdir build && cd build

# Configure
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_PYTHON_BINDINGS=OFF

# Build
make -j$(nproc)

# Install
sudo make install
```

### Custom Installation Directory
```bash
# Install to custom directory
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/clwe
make -j$(nproc)
make install

# Update environment
export CMAKE_PREFIX_PATH=/opt/clwe:$CMAKE_PREFIX_PATH
export PKG_CONFIG_PATH=/opt/clwe/lib/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/opt/clwe/lib:$LD_LIBRARY_PATH
```

## System Integration

### pkg-config Integration
```bash
# After installation, verify pkg-config
pkg-config --exists clwe
pkg-config --libs clwe
pkg-config --cflags clwe

# Example usage in Makefile
CFLAGS += $(shell pkg-config --cflags clwe)
LDFLAGS += $(shell pkg-config --libs clwe)
```

### CMake find_package Integration
```cmake
# After installation
find_package(CLWE CONFIG REQUIRED)
target_link_libraries(your_target CLWE::clwe)
```

### Direct Linking
```cpp
// Include headers
#include <clwe/clwe.hpp>
#include <clwe/color_kem.hpp>

// Link with: -lclwe_avx -lssl -lcrypto
```

## Platform-Specific Notes

### macOS
```bash
# Using Homebrew
brew install cmake openssl
brew link openssl --force  # If needed

# Build with Apple Silicon support
cmake .. -DCMAKE_OSX_ARCHITECTURES=arm64

# Or universal binary
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
```

### Windows (MSVC)
```bash
# Using vcpkg with Visual Studio
vcpkg install clwe --triplet=x64-windows

# Integrate with Visual Studio
vcpkg integrate install

# Or use CMake with vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### Linux Distributions

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install cmake build-essential libssl-dev git
```

#### CentOS/RHEL
```bash
sudo yum install cmake3 gcc-c++ openssl-devel git
# Use cmake3 instead of cmake
```

#### Arch Linux
```bash
sudo pacman -S cmake gcc openssl git
```

### Embedded Systems

#### Raspberry Pi
```bash
# Install dependencies
sudo apt install cmake build-essential libssl-dev

# Build with NEON optimizations (automatic)
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

#### Cross-Compilation
```bash
# For ARM64
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchains/arm64-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release
```

## Troubleshooting

### Common Issues

#### CMake Cannot Find OpenSSL
```bash
# Ubuntu/Debian
sudo apt install libssl-dev

# macOS
brew install openssl
cmake .. -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl

# CentOS/RHEL
sudo yum install openssl-devel
```

#### Compiler Errors
```
error: 'filesystem' is not a member of 'std'
```
**Solution**: Update compiler or use C++14 fallback
```bash
cmake .. -DCMAKE_CXX_STANDARD=14
```

#### SIMD Detection Issues
```bash
# Force specific SIMD
cmake .. -DENABLE_AVX=ON -DENABLE_NEON=OFF

# Disable all SIMD (scalar fallback)
cmake .. -DENABLE_AVX=OFF -DENABLE_NEON=OFF -DENABLE_RVV=OFF
```

#### Permission Errors During Install
```bash
# Use sudo for system installation
sudo make install

# Or install to user directory
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
```

### Build Performance Issues

#### Slow Builds
```bash
# Use more cores
make -j$(nproc)

# Use ccache for faster rebuilds
sudo apt install ccache
cmake .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

#### Large Binary Sizes
```bash
# Enable link-time optimization
cmake .. -DCMAKE_CXX_FLAGS="-flto" \
         -DCMAKE_EXE_LINKER_FLAGS="-flto"

# Strip debug symbols
strip libclwe_avx.so
```

### Runtime Issues

#### Library Not Found
```bash
# Update library cache
sudo ldconfig

# Or set LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

#### SIMD Mismatch
If you get runtime errors related to SIMD instructions:
```bash
# Check CPU capabilities
grep -E "(avx|neon|rvv|vsx)" /proc/cpuinfo

# Rebuild with appropriate flags
cmake .. -DENABLE_AVX=OFF  # Disable if not supported
```

## Verification

After installation, verify the setup:

```bash
# Check CMake configuration
find /usr/local/lib/cmake/CLWE -name "*.cmake"

# Check pkg-config
pkg-config --exists clwe && echo "pkg-config: OK"

# Check library
ls -la /usr/local/lib/libclwe_avx*

# Test basic functionality
cd /path/to/clwe/build
./demo_kem
```

## Support

If you encounter installation issues:

1. Check this guide for your platform
2. Verify all prerequisites are installed
3. Try a clean build: `rm -rf build && mkdir build`
4. Check GitHub Issues for similar problems
5. Create an issue with:
   - OS and version
   - Compiler version
   - CMake version
   - Full error output
   - Installation method used

---

*For the latest installation instructions, check the project repository.*