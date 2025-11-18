cmake_minimum_required(VERSION 3.16)
project(clwe_avx VERSION 1.0.0 LANGUAGES C CXX)

# Compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Multi-architecture SIMD detection and configuration
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

# Architecture-specific SIMD detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    # x86/x64 architecture - check for AVX support
    if(MSVC)
        # MSVC uses /arch flags
        check_cxx_compiler_flag("/arch:AVX2" AVX2_SUPPORTED)
        if(AVX2_SUPPORTED)
            add_compile_definitions(HAVE_AVX2)
            add_compile_options(/arch:AVX2)
            message(STATUS "x86_64 (MSVC): AVX2 support detected and enabled")

            # AVX-512 detection for MSVC
            check_cxx_compiler_flag("/arch:AVX512" AVX512_SUPPORTED)
            if(AVX512_SUPPORTED)
                add_compile_definitions(HAVE_AVX512)
                add_compile_options(/arch:AVX512)
                message(STATUS "x86_64 (MSVC): AVX-512 support detected and enabled")
            else()
                message(STATUS "x86_64 (MSVC): AVX-512 not supported, using AVX2")
            endif()
        else()
            message(WARNING "x86_64 (MSVC): AVX2 not supported, using scalar fallback")
            add_compile_definitions(NO_AVX_SUPPORT)
        endif()
    else()
        # GCC/Clang
        check_cxx_compiler_flag("-mavx2" AVX2_SUPPORTED)
        check_cxx_compiler_flag("-mfma" FMA_SUPPORTED)

        if(AVX2_SUPPORTED)
            add_compile_definitions(HAVE_AVX2)
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mavx2")
            if(FMA_SUPPORTED)
                set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mfma")
            endif()
            message(STATUS "x86_64: AVX2 support detected and enabled")

            # AVX-512 detection with comprehensive instruction set
            check_cxx_compiler_flag("-mavx512f" AVX512F_SUPPORTED)
            check_cxx_compiler_flag("-mavx512dq" AVX512DQ_SUPPORTED)
            check_cxx_compiler_flag("-mavx512bw" AVX512BW_SUPPORTED)
            check_cxx_compiler_flag("-mavx512vl" AVX512VL_SUPPORTED)

            if(AVX512F_SUPPORTED)
                add_compile_definitions(HAVE_AVX512)
                set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mavx512f")
                message(STATUS "x86_64: AVX-512F support detected and enabled")

                if(AVX512DQ_SUPPORTED)
                    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mavx512dq")
                    add_compile_definitions(HAVE_AVX512DQ)
                    message(STATUS "x86_64: AVX-512DQ support detected and enabled")
                endif()

                if(AVX512BW_SUPPORTED)
                    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mavx512bw")
                    add_compile_definitions(HAVE_AVX512BW)
                    message(STATUS "x86_64: AVX-512BW support detected and enabled")
                endif()

                if(AVX512VL_SUPPORTED)
                    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mavx512vl")
                    add_compile_definitions(HAVE_AVX512VL)
                    message(STATUS "x86_64: AVX-512VL support detected and enabled")
                endif()
            else()
                message(STATUS "x86_64: AVX-512 not supported, using AVX2")
            endif()
        else()
            message(WARNING "x86_64: AVX2 not supported, using scalar fallback")
            add_compile_definitions(NO_AVX_SUPPORT)
        endif()
    endif()

elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(aarch64)|(arm64)|(ARM64)")
    # ARM64 architecture - check for NEON support
    check_cxx_compiler_flag("-march=armv8-a+simd" NEON_SUPPORTED)
    if(NEON_SUPPORTED)
        add_compile_definitions(HAVE_NEON)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=armv8-a+simd")
        message(STATUS "ARM64: NEON SIMD support detected and enabled")
    else()
        message(WARNING "ARM64: NEON not supported, using scalar fallback")
        add_compile_definitions(NO_SIMD_SUPPORT)
    endif()

elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(riscv64)|(riscv)")
    # RISC-V architecture - check for vector extensions
    check_cxx_compiler_flag("-march=rv64gcv" RVV_SUPPORTED)
    if(RVV_SUPPORTED)
        add_compile_definitions(HAVE_RVV)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=rv64gcv")
        message(STATUS "RISC-V: Vector extensions detected and enabled")
    else()
        message(WARNING "RISC-V: Vector extensions not supported, using scalar fallback")
        add_compile_definitions(NO_SIMD_SUPPORT)
    endif()

elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(ppc64)|(powerpc64)")
    # PowerPC architecture - check for VSX/AltiVec
    check_cxx_compiler_flag("-mvsx" VSX_SUPPORTED)
    check_cxx_compiler_flag("-maltivec" ALTIVEC_SUPPORTED)
    if(VSX_SUPPORTED OR ALTIVEC_SUPPORTED)
        add_compile_definitions(HAVE_VSX)
        if(VSX_SUPPORTED)
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mvsx")
            message(STATUS "PowerPC: VSX support detected and enabled")
        endif()
        if(ALTIVEC_SUPPORTED)
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -maltivec")
            add_compile_definitions(HAVE_ALTIVEC)
            message(STATUS "PowerPC: AltiVec support detected and enabled")
        endif()
    else()
        message(WARNING "PowerPC: VSX/AltiVec not supported, using scalar fallback")
        add_compile_definitions(NO_SIMD_SUPPORT)
    endif()

else()
    # Unknown architecture
    message(STATUS "Unknown architecture ${CMAKE_SYSTEM_PROCESSOR}, using scalar fallback")
    add_compile_definitions(NO_SIMD_SUPPORT)
endif()

# Cross-compilation support
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-compilation detected - SIMD features will be determined at runtime")
    add_compile_definitions(CROSS_COMPILING)
endif()

# Dependencies
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/core)

# Library target
add_library(clwe_avx SHARED
    src/core/parameters.cpp
    src/core/polynomial.cpp
    src/core/ntt_avx.cpp
    src/core/ntt_engine.cpp
    src/core/ntt_scalar.cpp
    src/core/ntt_neon.cpp
    src/core/ntt_rvv.cpp
    src/core/color_value.cpp
    src/core/color_ntt_engine.cpp
    src/core/color_kem.cpp
    src/core/cpu_features.cpp
    src/core/shake_sampler.cpp
    src/core/ring_operations.cpp
    src/core/sampling.cpp
    src/core/utils.cpp
)

target_link_libraries(clwe_avx PRIVATE OpenSSL::Crypto)

# KEM demonstration
add_executable(demo_kem demo_kem.cpp)
target_link_libraries(demo_kem PRIVATE clwe_avx)

# Color KEM timing benchmark
add_executable(benchmark_color_kem_timing benchmark_color_kem_timing.cpp)
target_link_libraries(benchmark_color_kem_timing PRIVATE clwe_avx)

# Main executable
# add_executable(clwe_main src/main.cpp)
# target_link_libraries(clwe_main PRIVATE clwe_avx)

# Tests
# enable_testing()
# add_subdirectory(tests)


# Python bindings (optional)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" OFF)
if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()

# Installation
install(TARGETS clwe_avx
    EXPORT CLWETargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY src/include/clwe/
    DESTINATION include/clwe
    FILES_MATCHING PATTERN "*.hpp"
)

# Install package configuration files
include(CMakePackageConfigHelpers)

# Set package version
set(CLWE_VERSION "${PROJECT_VERSION}")

# Set variables for config file (needed before configure_package_config_file)
set(CLWE_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
set(CLWE_LIBRARY_DIR "${CMAKE_INSTALL_PREFIX}/lib")

# Configure and install package config files
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CLWEConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CLWEConfig.cmake"
    INSTALL_DESTINATION lib/cmake/CLWE
    PATH_VARS
        CLWE_INCLUDE_DIR
        CLWE_LIBRARY_DIR
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CLWEConfigVersion.cmake"
    VERSION ${CLWE_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CLWEConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/CLWEConfigVersion.cmake"
    DESTINATION lib/cmake/CLWE
)

# Install target files
install(EXPORT CLWETargets
    FILE CLWETargets.cmake
    NAMESPACE CLWE::
    DESTINATION lib/cmake/CLWE
)

# Configure and install pkg-config file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/clwe.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/clwe.pc"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/clwe.pc"
    DESTINATION lib/pkgconfig
)