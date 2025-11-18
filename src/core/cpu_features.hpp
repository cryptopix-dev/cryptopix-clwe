#ifndef CPU_FEATURES_HPP
#define CPU_FEATURES_HPP

#include <cstdint>
#include <string>

namespace clwe {

enum class CPUArchitecture {
    UNKNOWN,
    X86_64,
    ARM64,
    RISCV64,
    PPC64
};

enum class SIMDSupport {
    NONE,
    AVX2,
    AVX512,
    NEON,
    RVV,
    VSX
};

struct CPUFeatures {
    CPUArchitecture architecture = CPUArchitecture::UNKNOWN;
    SIMDSupport max_simd_support = SIMDSupport::NONE;

    bool has_avx2 = false;
    bool has_avx512f = false;
    bool has_avx512dq = false;
    bool has_avx512bw = false;
    bool has_avx512vl = false;

    bool has_neon = false;
    bool has_sve = false;

    bool has_rvv = false;
    uint32_t rvv_vlen = 0;

    bool has_vsx = false;
    bool has_altivec = false;

    std::string to_string() const;
};

class CPUFeatureDetector {
public:
    static CPUFeatures detect();

private:
    static CPUFeatures detect_x86();

    static CPUFeatures detect_arm();

    static CPUFeatures detect_riscv();

    static CPUFeatures detect_ppc();

    static CPUArchitecture detect_architecture();

    static bool has_cpuid();
    static void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t* regs);
    static uint64_t xgetbv(uint32_t xcr);
};

} // namespace clwe

#endif // CPU_FEATURES_HPP