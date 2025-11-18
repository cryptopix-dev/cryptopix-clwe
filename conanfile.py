from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.tools.files import copy
import os


class CLWEConan(ConanFile):
    name = "clwe"
    version = "1.0.0"
    description = "Cryptopix-CLWE: Color-Integrated Learning With Errors (CLWE) Cryptosystem"
    license = "Apache-2.0"
    author = "Your Organization <contact@your-org.com>"
    url = "https://github.com/your-org/cryptopix-clwe"
    homepage = "https://github.com/your-org/cryptopix-clwe"
    topics = ("cryptography", "post-quantum", "lattice-based", "kem", "pqc")

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_python": [True, False],
        "with_tests": [True, False],
        "with_benchmarks": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_python": False,
        "with_tests": False,
        "with_benchmarks": False
    }

    def export_sources(self):
        copy(self, "CMakeLists.txt", self.source_folder, self.export_sources_folder)
        copy(self, "cmake/*", self.source_folder, self.export_sources_folder)
        copy(self, "src/*", self.source_folder, self.export_sources_folder)
        copy(self, "demo_kem.cpp", self.source_folder, self.export_sources_folder)
        copy(self, "benchmark_color_kem_timing.cpp", self.source_folder, self.export_sources_folder)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("openssl/3.1.0")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_PYTHON_BINDINGS"] = self.options.with_python
        tc.variables["ENABLE_TESTS"] = self.options.with_tests
        tc.variables["ENABLE_BENCHMARKS"] = self.options.with_benchmarks
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

        # Copy license
        copy(self, "LICENSE", self.source_folder, os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.libs = ["clwe_avx"]
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.set_property("cmake_file_name", "CLWE")
        self.cpp_info.set_property("cmake_target_name", "CLWE::clwe")
        self.cpp_info.requires = ["openssl::openssl"]