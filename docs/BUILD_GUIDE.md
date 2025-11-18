# Build Guide: Cryptopix-CLWE

This guide provides comprehensive instructions for building Cryptopix-CLWE on various platforms and architectures.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Platform-Specific Instructions](#platform-specific-instructions)
- [Cross-Compilation](#cross-compilation)
- [Build Options](#build-options)
- [Troubleshooting](#troubleshooting)
- [Advanced Configuration](#advanced-configuration)

## Prerequisites

### Required Tools

| Tool | Version | Description |
|------|---------|-------------|
| CMake | 3.16+ | Build system generator |
| C++ Compiler | C++17 | GCC 7+, Clang 5+, MSVC 2017+ |
| OpenSSL | 1.1.1+ | Cryptographic library |
| Git | 2.0+ | Version control |

### System Requirements

- **RAM**: Minimum 4GB, recommended 8GB+
- **Disk Space**: 500MB for source and build artifacts
- **OS**: Linux, macOS, Windows, or compatible Unix-like systems

## Quick Start

### Linux/macOS

```bash
# Clone repository
git clone https://github.com/your-org/cryptopix-clwe.git
cd cryptopix-clwe

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# Test
./demo_kem
./benchmark_color_kem_timing
```

### Windows (Visual Studio)

```bash
# Clone repository
git clone https://github.com/your-org/cryptopix-clwe.git
cd cryptopix-clwe

# Create build directory
mkdir build && cd build

# Configure with Visual Studio generator
cmake .. -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Test
Release\demo_kem.exe
Release\benchmark_color_kem_timing.exe
```

## Platform-Specific Instructions

### macOS

#### Homebrew Installation

```bash
# Install dependencies
brew install cmake openssl

# Clone and build
git clone https://github.com/your-org/cryptopix-clwe.git
cd cryptopix-clwe
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

#### Apple Silicon (M1/M2/M3/M4) Notes

Cryptopix-CLWE automatically detects Apple Silicon and enables NEON optimizations:

```bash
# Build with Apple Silicon optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_OSX_ARCHITECTURES=arm64

# Verify NEON usage
./benchmark_color_kem_timing
# Should show: CPU: Architecture: ARM64, SIMD: NEON
```

#### Intel macOS

```bash
# Force x86_64 architecture
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_OSX_ARCHITECTURES=x86_64

# Enable AVX optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_OSX_ARCHITECTURES=x86_64 \
         -DENABLE_AVX=ON
```

### Linux

#### Ubuntu/Debian

```bash
# Install dependencies
sudo apt update
sudo apt install -y cmake build-essential libssl-dev git

# Clone and build
git clone https://github.com/your-org/cryptopix-clwe.git
cd cryptopix-clwe
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

#### CentOS/RHEL

```bash
# Install dependencies
sudo yum install -y cmake3 gcc-c++ openssl-devel git

# Use cmake3 instead of cmake
git clone https://github.com/your-org/cryptopix-clwe.git
cd cryptopix-clwe
mkdir build && cd build
cmake3 .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

#### Arch Linux

```bash
# Install dependencies
sudo pacman -S cmake gcc openssl git

# Clone and build
git clone https://github.com/your-org/cryptopix-clwe.git
cd cryptopix-clwe
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Windows

#### Visual Studio 2019/2022

```bash
# Install dependencies via vcpkg
vcpkg install openssl

# Configure
cmake .. -G "Visual Studio 16 2019" \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build from Visual Studio or command line
cmake --build . --config Release
```

#### MinGW

```bash
# Install MSYS2 and dependencies
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-openssl

# Configure
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# Build
mingw32-make -j$(nproc)
```

#### Windows Subsystem for Linux (WSL)

Follow Linux instructions above.

### Embedded Systems

#### Raspberry Pi (ARM64)

```bash
# Install dependencies
sudo apt install cmake build-essential libssl-dev

# Build with NEON optimizations
git clone https://github.com/your-org/cryptopix-clwe.git
cd cryptopix-clwe
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

#### RISC-V

```bash
# Cross-compile for RISC-V
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_TOOLCHAIN_FILE=../toolchains/riscv64-toolchain.cmake

make -j$(nproc)
```

## Cross-Compilation

### ARM64 Cross-Compilation

Create `toolchains/arm64-toolchain.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

Build:

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchains/arm64-toolchain.cmake \
         -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)
```

### Windows Cross-Compilation from Linux

```bash
# Install mingw
sudo apt install gcc-mingw-w64

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_TOOLCHAIN_FILE=../toolchains/mingw-toolchain.cmake

make -j$(nproc)
```

## Build Options

### CMake Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Release | Build type: Debug, Release, RelWithDebInfo |
| `BUILD_PYTHON_BINDINGS` | OFF | Build Python bindings |
| `ENABLE_AVX` | AUTO | Enable AVX optimizations |
| `ENABLE_NEON` | AUTO | Enable NEON optimizations |
| `ENABLE_RVV` | AUTO | Enable RISC-V vector extensions |
| `ENABLE_TESTS` | OFF | Build unit tests |
| `ENABLE_BENCHMARKS` | ON | Build benchmark executables |

### SIMD Feature Control

```bash
# Force specific SIMD features
cmake .. -DENABLE_AVX=ON \
         -DENABLE_AVX512=ON \
         -DENABLE_NEON=OFF

# Disable all SIMD (scalar fallback)
cmake .. -DENABLE_AVX=OFF \
         -DENABLE_NEON=OFF \
         -DENABLE_RVV=OFF \
         -DENABLE_VSX=OFF
```

### Debug Build

```bash
# Build with debug symbols and assertions
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DCMAKE_CXX_FLAGS="-g -O0"

# Enable address sanitizer
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DCMAKE_CXX_FLAGS="-fsanitize=address -g"

# Enable undefined behavior sanitizer
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DCMAKE_CXX_FLAGS="-fsanitize=undefined -g"
```

### Optimized Release Build

```bash
# Maximum optimization
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-O3 -march=native -flto"

# Profile-guided optimization
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-O3 -fprofile-generate"

# Build with PGO data
make -j$(nproc)
# Run benchmarks to generate profile
./benchmark_color_kem_timing

# Rebuild with PGO
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-O3 -fprofile-use"
make -j$(nproc)
```

## Testing

### Unit Tests

```bash
# Build with tests
cmake .. -DENABLE_TESTS=ON
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Run specific test
ctest -R ColorKEMTest
```

### Integration Tests

```bash
# Run demo
./demo_kem

# Run benchmarks
./benchmark_color_kem_timing

# Verify SIMD detection
./demo_kem | grep "SIMD:"
```

### Performance Validation

```bash
# Run comprehensive benchmarks
./benchmark_color_kem_timing > benchmark_results.txt

# Compare with reference values
python3 scripts/validate_benchmarks.py benchmark_results.txt
```

## Troubleshooting

### Common Issues

#### CMake Errors

**Problem**: CMake cannot find OpenSSL
```
Could NOT find OpenSSL (missing: OPENSSL_INCLUDE_DIR)
```

**Solution**:
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

**Problem**: C++17 features not supported
```
error: 'filesystem' is not a member of 'std'
```

**Solution**: Update compiler or use older standard
```bash
# Check compiler version
g++ --version

# Use C++14 fallback
cmake .. -DCMAKE_CXX_STANDARD=14
```

#### Linker Errors

**Problem**: Undefined references to OpenSSL functions

**Solution**:
```bash
# Check OpenSSL installation
pkg-config --libs openssl

# Specify library path
cmake .. -DOPENSSL_LIBRARIES=/usr/lib/x86_64-linux-gnu/libssl.so \
         -DOPENSSL_INCLUDE_DIRS=/usr/include/openssl
```

### Architecture-Specific Issues

#### AVX Detection Issues

```bash
# Force AVX support
cmake .. -DCMAKE_CXX_FLAGS="-mavx2 -mfma"

# Check CPU capabilities
grep avx /proc/cpuinfo
```

#### NEON on ARM64

```bash
# Verify NEON support
grep neon /proc/cpuinfo

# Force NEON compilation
cmake .. -DCMAKE_CXX_FLAGS="-march=armv8-a+simd"
```

### Build Performance

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
strip demo_kem
```

## Advanced Configuration

### Custom Toolchains

Create custom toolchain files for specific targets:

```cmake
# toolchains/custom-toolchain.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR custom)

set(CMAKE_C_COMPILER custom-gcc)
set(CMAKE_CXX_COMPILER custom-g++)

# Custom flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcustom-arch")

# Library paths
set(CMAKE_FIND_ROOT_PATH /opt/custom-toolchain)
```

### Build System Integration

#### Makefile Integration

```makefile
# External Makefile
CRYPTOPIX_DIR = /path/to/cryptopix-clwe
CRYPTOPIX_BUILD = $(CRYPTOPIX_DIR)/build

$(CRYPTOPIX_BUILD)/libclwe_avx.so:
    cd $(CRYPTOPIX_DIR) && mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)

my_app: $(CRYPTOPIX_BUILD)/libclwe_avx.so
    gcc my_app.c -L$(CRYPTOPIX_BUILD) -lclwe_avx -o my_app
```

#### Bazel Integration

```python
# WORKSPACE
new_git_repository(
    name = "cryptopix_clwe",
    remote = "https://github.com/your-org/cryptopix-clwe.git",
    tag = "v1.0.0",
)

# BUILD
cc_library(
    name = "clwe",
    srcs = ["@cryptopix_clwe//:libclwe_avx.so"],
    hdrs = glob(["@cryptopix_clwe//src/include/**/*.hpp"]),
)
```

### Continuous Integration

#### GitHub Actions Example

```yaml
# .github/workflows/build.yml
name: Build
on: [push, pull_request]

jobs:
  build:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]

    steps:
    - uses: actions/checkout@v2

    - name: Install dependencies (Ubuntu)
      if: matrix.os == 'ubuntu-latest'
      run: sudo apt install cmake libssl-dev

    - name: Install dependencies (macOS)
      if: matrix.os == 'macos-latest'
      run: brew install cmake openssl

    - name: Configure
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release

    - name: Build
      run: cmake --build build --config Release

    - name: Test
      run: |
        cd build
        ctest --output-on-failure
```

### Performance Monitoring

```bash
# Profile build time
time make -j$(nproc)

# Analyze binary size
ls -lh build/libclwe_avx.so

# Check for optimization opportunities
objdump -d build/libclwe_avx.so | grep -i avx
```

## Support

If you encounter build issues:

1. Check this guide for your platform
2. Verify all prerequisites are installed
3. Try a clean build: `rm -rf build && mkdir build`
4. Check GitHub Issues for similar problems
5. Create a new issue with:
   - OS and version
   - Compiler version
   - CMake version
   - Full error output
   - Steps to reproduce

---

*For the latest build instructions, check the repository's README.md*