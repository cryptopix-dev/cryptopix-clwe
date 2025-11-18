#!/bin/bash

# Cryptopix-CLWE Build Script
# This script provides automated building and packaging for multiple platforms

set -e

# Configuration
PROJECT_NAME="cryptopix-clwe"
VERSION="1.0.0"
BUILD_TYPE="${BUILD_TYPE:-Release}"
ENABLE_TESTS="${ENABLE_TESTS:-OFF}"
ENABLE_BENCHMARKS="${ENABLE_BENCHMARKS:-ON}"
BUILD_PYTHON="${BUILD_PYTHON:-OFF}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# Detect OS
detect_os() {
    case "$(uname -s)" in
        Linux*)     OS=linux;;
        Darwin*)    OS=macos;;
        CYGWIN*|MINGW*|MSYS*) OS=windows;;
        *)          OS=unknown
    esac
    log_info "Detected OS: $OS"
}

# Detect architecture
detect_arch() {
    ARCH=$(uname -m)
    case "$ARCH" in
        x86_64)  ARCH="x86_64";;
        aarch64) ARCH="arm64";;
        armv7l)  ARCH="arm32";;
        *)       ARCH="$ARCH"
    esac
    log_info "Detected architecture: $ARCH"
}

# Check dependencies
check_dependencies() {
    log_info "Checking dependencies..."

    # Check CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake not found. Please install CMake 3.16 or higher."
        exit 1
    fi

    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    log_info "CMake version: $CMAKE_VERSION"

    # Check C++ compiler
    if command -v g++ &> /dev/null; then
        CXX_COMPILER="g++"
    elif command -v clang++ &> /dev/null; then
        CXX_COMPILER="clang++"
    elif command -v cl &> /dev/null; then
        CXX_COMPILER="cl"
    else
        log_error "No C++ compiler found."
        exit 1
    fi

    log_info "C++ compiler: $CXX_COMPILER"

    # Check OpenSSL
    if ! pkg-config --exists openssl 2>/dev/null && ! [ -d "/usr/include/openssl" ]; then
        log_warning "OpenSSL development headers not found via pkg-config."
        log_warning "Make sure OpenSSL is installed and pkg-config is configured."
    else
        log_info "OpenSSL found"
    fi
}

# Setup build directory
setup_build_dir() {
    BUILD_DIR="build"
    if [ -d "$BUILD_DIR" ]; then
        log_info "Removing existing build directory..."
        rm -rf "$BUILD_DIR"
    fi

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    log_info "Build directory: $(pwd)"
}

# Configure CMake
configure_cmake() {
    log_info "Configuring CMake..."

    CMAKE_ARGS=(
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DBUILD_PYTHON_BINDINGS="$BUILD_PYTHON"
        -DENABLE_TESTS="$ENABLE_TESTS"
        -DENABLE_BENCHMARKS="$ENABLE_BENCHMARKS"
    )

    # OS-specific configuration
    case "$OS" in
        macos)
            CMAKE_ARGS+=(-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64")
            ;;
        linux)
            CMAKE_ARGS+=(-DCMAKE_CXX_FLAGS="-march=native")
            ;;
    esac

    log_info "CMake arguments: ${CMAKE_ARGS[*]}"

    cmake .. "${CMAKE_ARGS[@]}"

    if [ $? -eq 0 ]; then
        log_success "CMake configuration completed"
    else
        log_error "CMake configuration failed"
        exit 1
    fi
}

# Build project
build_project() {
    log_info "Building project..."

    # Detect number of cores
    if command -v nproc &> /dev/null; then
        JOBS=$(nproc)
    elif [ "$OS" = "macos" ]; then
        JOBS=$(sysctl -n hw.ncpu | cut -d' ' -f2)
    else
        JOBS=4
    fi

    log_info "Using $JOBS parallel jobs"

    make -j"$JOBS"

    if [ $? -eq 0 ]; then
        log_success "Build completed successfully"
    else
        log_error "Build failed"
        exit 1
    fi
}

# Run tests
run_tests() {
    if [ "$ENABLE_TESTS" = "ON" ]; then
        log_info "Running tests..."
        if command -v ctest &> /dev/null; then
            ctest --output-on-failure
        else
            log_warning "ctest not found, skipping tests"
        fi
    fi
}

# Run benchmarks
run_benchmarks() {
    if [ "$ENABLE_BENCHMARKS" = "ON" ]; then
        log_info "Running benchmarks..."
        if [ -f "./benchmark_color_kem_timing" ]; then
            ./benchmark_color_kem_timing --quick
        else
            log_warning "Benchmark executable not found"
        fi
    fi
}

# Run demo
run_demo() {
    log_info "Running demo..."
    if [ -f "./demo_kem" ]; then
        ./demo_kem
    else
        log_warning "Demo executable not found"
    fi
}

# Create package
create_package() {
    log_info "Creating package..."

    if command -v cpack &> /dev/null; then
        cpack
        log_success "Package created"
    else
        log_warning "CPack not found, skipping package creation"
    fi
}

# Install locally
install_local() {
    log_info "Installing locally..."
    make install

    if [ $? -eq 0 ]; then
        log_success "Installation completed"
        log_info "You may need to run 'sudo ldconfig' on Linux to update library cache"
    else
        log_error "Installation failed"
        exit 1
    fi
}

# Print usage
usage() {
    cat << EOF
Cryptopix-CLWE Build Script

USAGE:
    $0 [OPTIONS] [COMMAND]

COMMANDS:
    all             Build, test, and install (default)
    configure       Configure CMake only
    build           Build only
    test            Run tests only
    benchmark       Run benchmarks only
    demo            Run demo only
    package         Create package
    install         Install locally
    clean           Clean build directory

OPTIONS:
    --build-type TYPE   Build type: Debug, Release, RelWithDebInfo (default: Release)
    --enable-tests       Enable unit tests (default: off)
    --enable-benchmarks  Enable benchmarks (default: on)
    --enable-python      Enable Python bindings (default: off)
    --help, -h           Show this help

EXAMPLES:
    $0                          # Build and install release version
    $0 --build-type Debug       # Build debug version
    $0 --enable-tests test      # Build with tests and run them
    $0 configure                # Configure only
    $0 build                    # Build only

EOF
}

# Parse command line arguments
parse_args() {
    COMMAND="all"

    while [[ $# -gt 0 ]]; do
        case $1 in
            --build-type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            --enable-tests)
                ENABLE_TESTS="ON"
                shift
                ;;
            --enable-benchmarks)
                ENABLE_BENCHMARKS="ON"
                shift
                ;;
            --enable-python)
                BUILD_PYTHON="ON"
                shift
                ;;
            --help|-h)
                usage
                exit 0
                ;;
            configure|build|test|benchmark|demo|package|install|clean)
                COMMAND="$1"
                shift
                ;;
            *)
                log_error "Unknown option: $1"
                usage
                exit 1
                ;;
        esac
    done
}

# Main function
main() {
    parse_args "$@"

    detect_os
    detect_arch
    check_dependencies

    case "$COMMAND" in
        all)
            setup_build_dir
            configure_cmake
            build_project
            run_tests
            run_benchmarks
            run_demo
            install_local
            ;;
        configure)
            setup_build_dir
            configure_cmake
            ;;
        build)
            setup_build_dir
            configure_cmake
            build_project
            ;;
        test)
            run_tests
            ;;
        benchmark)
            run_benchmarks
            ;;
        demo)
            run_demo
            ;;
        package)
            create_package
            ;;
        install)
            install_local
            ;;
        clean)
            rm -rf build
            log_success "Build directory cleaned"
            ;;
        *)
            log_error "Unknown command: $COMMAND"
            usage
            exit 1
            ;;
    esac

    log_success "Operation completed successfully!"
}

# Run main function with all arguments
main "$@"