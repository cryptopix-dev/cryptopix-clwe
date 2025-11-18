#!/bin/bash

# Cryptopix-CLWE Package Testing Script
# Tests package installations across different methods

set -e

# Configuration
PROJECT_NAME="clwe"
VERSION="1.0.0"
TEST_DIR="/tmp/clwe_test_$(date +%s)"
CMAKE_TEST_PROJECT="$TEST_DIR/cmake_test"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Setup test environment
setup_test_env() {
    log_info "Setting up test environment in $TEST_DIR"
    mkdir -p "$TEST_DIR"
    cd "$TEST_DIR"

    # Create test CMake project
    mkdir -p "$CMAKE_TEST_PROJECT"
    cat > "$CMAKE_TEST_PROJECT/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.16)
project(clwe_test VERSION 1.0.0 LANGUAGES CXX)

find_package(CLWE CONFIG REQUIRED)

add_executable(clwe_test main.cpp)
target_link_libraries(clwe_test PRIVATE CLWE::clwe)
EOF

    # Create test source file
    cat > "$CMAKE_TEST_PROJECT/main.cpp" << 'EOF'
#include <clwe/clwe.hpp>
#include <clwe/color_kem.hpp>
#include <iostream>

int main() {
    try {
        std::cout << "Testing CLWE package integration..." << std::endl;

        // Test basic functionality
        clwe::ColorKEM kem{clwe::CLWEParameters(128)};
        std::cout << "CLWE initialized successfully" << std::endl;

        // Test key generation
        auto [public_key, private_key] = kem.keygen();
        std::cout << "Key generation successful" << std::endl;

        // Test encapsulation
        auto [ciphertext, shared_secret] = kem.encapsulate(public_key);
        std::cout << "Encapsulation successful" << std::endl;

        // Test decapsulation
        auto recovered_secret = kem.decapsulate(public_key, private_key, ciphertext);
        std::cout << "Decapsulation successful" << std::endl;

        // Verify
        if (shared_secret.to_precise_value() == recovered_secret.to_precise_value()) {
            std::cout << "SUCCESS: Key exchange verified!" << std::endl;
            return 0;
        } else {
            std::cout << "FAILURE: Key exchange verification failed!" << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}
EOF
}

# Test CMake package installation
test_cmake_package() {
    log_info "Testing CMake package integration..."

    cd "$CMAKE_TEST_PROJECT"

    # Test CMake configuration
    if ! cmake -B build -S .; then
        log_error "CMake configuration failed"
        return 1
    fi

    # Test build
    if ! cmake --build build; then
        log_error "CMake build failed"
        return 1
    fi

    # Test execution
    if ! ./build/clwe_test; then
        log_error "Test execution failed"
        return 1
    fi

    log_success "CMake package integration test passed"
    return 0
}

# Test pkg-config integration
test_pkgconfig() {
    log_info "Testing pkg-config integration..."

    # Check if pkg-config finds CLWE
    if ! pkg-config --exists clwe; then
        log_error "pkg-config cannot find CLWE"
        return 1
    fi

    # Test pkg-config output
    local libs=$(pkg-config --libs clwe)
    local cflags=$(pkg-config --cflags clwe)

    if [ -z "$libs" ] || [ -z "$cflags" ]; then
        log_error "pkg-config returned empty libs or cflags"
        return 1
    fi

    log_info "pkg-config libs: $libs"
    log_info "pkg-config cflags: $cflags"

    log_success "pkg-config integration test passed"
    return 0
}

# Test vcpkg integration (if available)
test_vcpkg() {
    log_info "Testing vcpkg integration..."

    # Check if vcpkg is available
    if ! command -v vcpkg &> /dev/null; then
        log_warning "vcpkg not found, skipping vcpkg test"
        return 0
    fi

    # Try to find CLWE via vcpkg
    local vcpkg_root=$(dirname $(which vcpkg))
    local triplet=""

    case "$(uname -s)" in
        Linux*)     triplet="x64-linux" ;;
        Darwin*)    triplet="x64-osx" ;;
        CYGWIN*|MINGW*|MSYS*) triplet="x64-windows" ;;
        *)          triplet="x64-linux"
    esac

    # Test vcpkg installation
    if vcpkg list | grep -q "^clwe:"; then
        log_success "CLWE found in vcpkg"
    else
        log_warning "CLWE not found in vcpkg (may need to be added to registry)"
    fi

    return 0
}

# Test Conan integration (if available)
test_conan() {
    log_info "Testing Conan integration..."

    # Check if Conan is available
    if ! command -v conan &> /dev/null; then
        log_warning "Conan not found, skipping Conan test"
        return 0
    fi

    # Create temporary Conan test
    local conan_test_dir="$TEST_DIR/conan_test"
    mkdir -p "$conan_test_dir"
    cd "$conan_test_dir"

    # Create conanfile.txt
    cat > conanfile.txt << EOF
[requires]
clwe/${VERSION}@your-org/stable

[generators]
cmake_find_package
EOF

    # Try to install (this may fail if package not published)
    if conan install . --build missing 2>/dev/null; then
        log_success "Conan package installation successful"
        # Test CMake with Conan
        if [ -d "build" ] && cmake -B build -S "$CMAKE_TEST_PROJECT"; then
            log_success "Conan CMake integration successful"
        else
            log_warning "Conan CMake integration failed"
        fi
    else
        log_warning "Conan package installation failed (may need to be published)"
    fi

    return 0
}

# Test library functionality
test_library_functionality() {
    log_info "Testing library functionality..."

    cd "$CMAKE_TEST_PROJECT"

    # Build and run comprehensive test
    if cmake -B build -S . && cmake --build build && ./build/clwe_test; then
        log_success "Library functionality test passed"
        return 0
    else
        log_error "Library functionality test failed"
        return 1
    fi
}

# Test different security levels
test_security_levels() {
    log_info "Testing different security levels..."

    local test_script="$TEST_DIR/security_test.cpp"
    cat > "$test_script" << 'EOF'
#include <clwe/clwe.hpp>
#include <clwe/color_kem.hpp>
#include <iostream>

int main() {
    std::vector<int> security_levels = {128, 192, 256};

    for (int level : security_levels) {
        try {
            std::cout << "Testing security level " << level << "..." << std::endl;

            clwe::ColorKEM kem{clwe::CLWEParameters(level)};
            auto [public_key, private_key] = kem.keygen();
            auto [ciphertext, shared_secret] = kem.encapsulate(public_key);
            auto recovered_secret = kem.decapsulate(public_key, private_key, ciphertext);

            if (shared_secret.to_precise_value() == recovered_secret.to_precise_value()) {
                std::cout << "Security level " << level << ": PASSED" << std::endl;
            } else {
                std::cout << "Security level " << level << ": FAILED" << std::endl;
                return 1;
            }
        } catch (const std::exception& e) {
            std::cout << "Security level " << level << ": ERROR - " << e.what() << std::endl;
            return 1;
        }
    }

    std::cout << "All security levels tested successfully!" << std::endl;
    return 0;
}
EOF

    # Compile and run
    local test_exe="$TEST_DIR/security_test"
    if g++ -std=c++17 -o "$test_exe" "$test_script" $(pkg-config --cflags --libs clwe) && "$test_exe"; then
        log_success "Security levels test passed"
        return 0
    else
        log_error "Security levels test failed"
        return 1
    fi
}

# Cleanup function
cleanup() {
    log_info "Cleaning up test environment..."
    rm -rf "$TEST_DIR"
}

# Print usage
usage() {
    cat << EOF
Cryptopix-CLWE Package Testing Script

USAGE:
    $0 [OPTIONS]

OPTIONS:
    --cmake-only       Test only CMake integration
    --pkgconfig-only   Test only pkg-config integration
    --vcpkg-only       Test only vcpkg integration
    --conan-only       Test only Conan integration
    --no-cleanup       Don't clean up test files
    --help, -h         Show this help

EXAMPLES:
    $0                      # Run all tests
    $0 --cmake-only         # Test only CMake integration
    $0 --no-cleanup         # Keep test files for debugging

EOF
}

# Main function
main() {
    local test_cmake=true
    local test_pkgconfig=true
    local test_vcpkg=true
    local test_conan=true
    local cleanup_after=true

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --cmake-only)
                test_pkgconfig=false
                test_vcpkg=false
                test_conan=false
                shift
                ;;
            --pkgconfig-only)
                test_cmake=false
                test_vcpkg=false
                test_conan=false
                shift
                ;;
            --vcpkg-only)
                test_cmake=false
                test_pkgconfig=false
                test_conan=false
                shift
                ;;
            --conan-only)
                test_cmake=false
                test_pkgconfig=false
                test_vcpkg=false
                shift
                ;;
            --no-cleanup)
                cleanup_after=false
                shift
                ;;
            --help|-h)
                usage
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                usage
                exit 1
                ;;
        esac
    done

    # Trap for cleanup
    if [ "$cleanup_after" = true ]; then
        trap cleanup EXIT
    fi

    log_info "Starting CLWE package integration tests"
    log_info "Test directory: $TEST_DIR"

    # Setup
    setup_test_env

    local all_passed=true

    # Run tests
    if [ "$test_cmake" = true ]; then
        if ! test_cmake_package; then
            all_passed=false
        fi
    fi

    if [ "$test_pkgconfig" = true ]; then
        if ! test_pkgconfig; then
            all_passed=false
        fi
    fi

    if [ "$test_vcpkg" = true ]; then
        if ! test_vcpkg; then
            all_passed=false
        fi
    fi

    if [ "$test_conan" = true ]; then
        if ! test_conan; then
            all_passed=false
        fi
    fi

    # Additional tests
    if ! test_library_functionality; then
        all_passed=false
    fi

    if ! test_security_levels; then
        all_passed=false
    fi

    # Summary
    echo
    if [ "$all_passed" = true ]; then
        log_success "All package integration tests PASSED!"
        exit 0
    else
        log_error "Some package integration tests FAILED!"
        exit 1
    fi
}

# Run main function with all arguments
main "$@"