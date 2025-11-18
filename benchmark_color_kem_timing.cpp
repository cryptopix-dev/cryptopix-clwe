#include <iostream>
#include <chrono>
#include <vector>
#include <functional>
#include "clwe/clwe.hpp"
#include "src/core/color_kem.hpp"
#include "src/core/cpu_features.hpp"

using namespace clwe;

double time_operation(const std::function<void()>& operation, int iterations = 100) {
    std::vector<double> times;

    for (int i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        operation();
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        times.push_back(static_cast<double>(duration.count()));
    }

    
    double sum = 0;
    for (double t : times) sum += t;
    return sum / times.size();
}

void benchmark_security_level(int security_level) {
    std::cout << "Security Level: " << security_level << "-bit" << std::endl;
    std::cout << "=====================================" << std::endl;

    
    clwe::CLWEParameters params(security_level);
    clwe::ColorKEM kem(params);

    
    auto [public_key, private_key] = kem.keygen();

    
    auto [ciphertext, shared_secret] = kem.encapsulate(public_key);

    
    double keygen_time = time_operation([&]() {
        auto [pk, sk] = kem.keygen();
    });

    
    double encap_time = time_operation([&]() {
        auto [ct, ss] = kem.encapsulate(public_key);
    });

    
    double decap_time = time_operation([&]() {
        ColorValue recovered = kem.decapsulate(public_key, private_key, ciphertext);
    });

    double total_kem_time = keygen_time + encap_time + decap_time;
    double throughput = 1000000.0 / total_kem_time;  

    
    std::cout << "Key Generation:     " << keygen_time << " μs" << std::endl;
    std::cout << "Encapsulation:      " << encap_time << " μs" << std::endl;
    std::cout << "Decapsulation:      " << decap_time << " μs" << std::endl;
    std::cout << "Total KEM Time:     " << total_kem_time << " μs" << std::endl;
    std::cout << "Throughput:         " << throughput << " operations/second" << std::endl;
    std::cout << std::endl;

    std::cout << "Time Distribution:" << std::endl;
    std::cout << "  KeyGen: " << (keygen_time / total_kem_time * 100) << "%" << std::endl;
    std::cout << "  Encap:  " << (encap_time / total_kem_time * 100) << "%" << std::endl;
    std::cout << "  Decap:  " << (decap_time / total_kem_time * 100) << "%" << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "🎨 CLWE Color KEM Timing Benchmark" << std::endl;
    std::cout << "===================================" << std::endl;

    
    CPUFeatures features = CPUFeatureDetector::detect();
    std::cout << "CPU: " << features.to_string() << std::endl;
    std::cout << std::endl;

    
    std::vector<int> security_levels = {128, 192, 256};

    for (int level : security_levels) {
        benchmark_security_level(level);
    }

    std::cout << "Benchmark completed successfully!" << std::endl;

    return 0;
}