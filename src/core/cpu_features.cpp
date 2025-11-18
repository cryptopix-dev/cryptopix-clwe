#include "cpu_features.hpp"
#include <iostream>
#include <cstring>
#include <sstream>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace clwe {

std::string CPUFeatures::to_string() const {
    std::stringstream ss;
    ss << "Architecture: ";

    switch (architecture) {
        case CPUArchitecture::X86_64: ss << "x86_64"; break;
        case CPUArchitecture::ARM64: ss << "ARM64"; break;
        case CPUArchitecture::RISCV64: ss << "RISC-V 64"; break;
        case CPUArchitecture::PPC64: ss << "PowerPC 64"; break;
        default: ss << "Unknown"; break;
    }

    ss << ", SIMD: ";
    switch (max_simd_support) {
        case SIMDSupport::AVX512: ss << "AVX-512"; break;
        case SIMDSupport::AVX2: ss << "AVX2"; break;
        case SIMDSupport::NEON: ss << "NEON"; break;
        case SIMDSupport::RVV: ss << "RVV"; break;
        case SIMDSupport::VSX: ss << "VSX"; break;
        default: ss << "None"; break;
    }

    return ss.str();
}

CPUFeatures CPUFeatureDetector::detect() {
    CPUArchitecture arch = detect_architecture();

    switch (arch) {
        case CPUArchitecture::X86_64:
            return detect_x86();
        case CPUArchitecture::ARM64:
            return detect_arm();
        case CPUArchitecture::RISCV64:
            return detect_riscv();
        case CPUArchitecture::PPC64:
            return detect_ppc();
        default:
            CPUFeatures features;
            features.architecture = CPUArchitecture::UNKNOWN;
            features.max_simd_support = SIMDSupport::NONE;
            return features;
    }
}

CPUArchitecture CPUFeatureDetector::detect_architecture() {
#if defined(__x86_64__) || defined(_M_X64)
    return CPUArchitecture::X86_64;
#elif defined(__aarch64__) || defined(_M_ARM64)
    return CPUArchitecture::ARM64;
#elif defined(__riscv) && (__riscv_xlen == 64)
    return CPUArchitecture::RISCV64;
#elif defined(__powerpc64__) || defined(__ppc64__)
    return CPUArchitecture::PPC64;
#else
    return CPUArchitecture::UNKNOWN;
#endif
}

CPUFeatures CPUFeatureDetector::detect_x86() {
    CPUFeatures features;
    features.architecture = CPUArchitecture::X86_64;

    if (!has_cpuid()) {
        features.max_simd_support = SIMDSupport::NONE;
        return features;
    }

    uint32_t regs[4];

    // Check AVX2 support
    cpuid(0x7, 0, regs);
    bool has_avx2 = (regs[1] & (1 << 5)) != 0;  // AVX2 bit
    bool has_bmi2 = (regs[1] & (1 << 8)) != 0;  // BMI2 bit

    // Check AVX-512 support
    bool has_avx512f = (regs[1] & (1 << 16)) != 0;   // AVX-512F
    bool has_avx512dq = (regs[1] & (1 << 17)) != 0;  // AVX-512DQ
    bool has_avx512bw = (regs[1] & (1 << 30)) != 0;  // AVX-512BW
    bool has_avx512vl = (regs[2] & (1 << 31)) != 0;  // AVX-512VL

    // Check OS support for AVX
    uint64_t xcr0 = xgetbv(0);
    bool os_avx_support = (xcr0 & 0x6) == 0x6;  // XMM and YMM state
    bool os_avx512_support = (xcr0 & 0xE6) == 0xE6;  // XMM, YMM, and ZMM state

    features.has_avx2 = has_avx2 && os_avx_support;
    features.has_avx512f = has_avx512f && os_avx512_support;
    features.has_avx512dq = has_avx512dq && os_avx512_support;
    features.has_avx512bw = has_avx512bw && os_avx512_support;
    features.has_avx512vl = has_avx512vl && os_avx512_support;

    // Determine maximum SIMD support
    if (features.has_avx512f) {
        features.max_simd_support = SIMDSupport::AVX512;
    } else if (features.has_avx2) {
        features.max_simd_support = SIMDSupport::AVX2;
    } else {
        features.max_simd_support = SIMDSupport::NONE;
    }

    return features;
}

CPUFeatures CPUFeatureDetector::detect_arm() {
    CPUFeatures features;
    features.architecture = CPUArchitecture::ARM64;

    // ARM64 always has NEON support in modern processors
    features.has_neon = true;
    features.max_simd_support = SIMDSupport::NEON;

    // Check for SVE (Scalable Vector Extension)
    // This is a simplified check - in practice, you'd need to query the system
#if defined(__ARM_FEATURE_SVE)
    features.has_sve = true;
#endif

    return features;
}

CPUFeatures CPUFeatureDetector::detect_riscv() {
    CPUFeatures features;
    features.architecture = CPUArchitecture::RISCV64;

    // Check for RVV support
    // This is a simplified check - in practice, you'd need to query the system
#if defined(__riscv_v)
    features.has_rvv = true;
    features.rvv_vlen = __riscv_v_elen;  // Vector element length
    features.max_simd_support = SIMDSupport::RVV;
#else
    features.max_simd_support = SIMDSupport::NONE;
#endif

    return features;
}

CPUFeatures CPUFeatureDetector::detect_ppc() {
    CPUFeatures features;
    features.architecture = CPUArchitecture::PPC64;

    // PowerPC VSX/AltiVec support
    // This is a simplified check - in practice, you'd need to query the system
#if defined(__VSX__) || defined(__ALTIVEC__)
    features.has_vsx = true;
    features.has_altivec = true;
    features.max_simd_support = SIMDSupport::VSX;
#else
    features.max_simd_support = SIMDSupport::NONE;
#endif

    return features;
}

bool CPUFeatureDetector::has_cpuid() {
#if defined(__x86_64__) || defined(_M_X64)
    return true;
#else
    return false;
#endif
}

void CPUFeatureDetector::cpuid(uint32_t leaf, uint32_t subleaf, uint32_t* regs) {
#if defined(_MSC_VER)
    __cpuidex((int*)regs, leaf, subleaf);
#elif defined(__x86_64__) || defined(_M_X64)
    __asm__ volatile(
        "cpuid"
        : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
        : "a"(leaf), "c"(subleaf)
    );
#else
    regs[0] = regs[1] = regs[2] = regs[3] = 0;
#endif
}

uint64_t CPUFeatureDetector::xgetbv(uint32_t xcr) {
#if defined(_MSC_VER)
    return _xgetbv(xcr);
#elif defined(__x86_64__) || defined(_M_X64)
    uint32_t eax, edx;
    __asm__ volatile(
        "xgetbv"
        : "=a"(eax), "=d"(edx)
        : "c"(xcr)
    );
    return ((uint64_t)edx << 32) | eax;
#else
    return 0;
#endif
}

} // namespace clwe