#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>
#include <cstddef>
#include <memory>

#ifdef HAVE_AVX2
#include <immintrin.h>
#else
// Define dummy AVX types for non-AVX architectures
typedef struct { uint32_t m[8]; } __m256i;
#endif

namespace clwe {


// AVX-Aligned Memory Allocator
class AVXAllocator {
public:
    static void* allocate(size_t size);
    static void deallocate(void* ptr);
    static void* reallocate(void* ptr, size_t new_size);
};

// AVX-Aligned Vector Template
template<typename T>
class AVXVector {
private:
    T* data_;
    size_t size_;
    size_t capacity_;

public:
    AVXVector();
    AVXVector(size_t initial_capacity);
    ~AVXVector();

    // Disable copy and assignment for simplicity
    AVXVector(const AVXVector&) = delete;
    AVXVector& operator=(const AVXVector&) = delete;

    // Move operations
    AVXVector(AVXVector&& other) noexcept;
    AVXVector& operator=(AVXVector&& other) noexcept;

    void resize(size_t new_size);
    void reserve(size_t new_capacity);
    void clear();

    T* data() { return data_; }
    const T* data() const { return data_; }
    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }

    void push_back(const T& value);
    void push_back(T&& value);
    void pop_back();
};

// Utility functions
uint64_t get_timestamp_ns();
double timestamp_to_ms(uint64_t ts);

// Montgomery reduction utilities
uint32_t montgomery_reduce(uint64_t a, uint32_t q);
uint32_t montgomery_reduce_avx(__m256i a, uint32_t q); // Forward declare for AVX

// Barrett reduction
uint32_t barrett_reduce(uint64_t a, uint32_t q, uint64_t mu);

// Bit operations
int bit_length(uint32_t x);
bool is_power_of_two(uint32_t x);
uint32_t next_power_of_two(uint32_t x);

// Modular inverse
uint32_t mod_inverse(uint32_t a, uint32_t m);

// Modular exponentiation
uint32_t mod_pow(uint32_t base, uint32_t exp, uint32_t mod);

} // namespace clwe

#endif // UTILS_HPP