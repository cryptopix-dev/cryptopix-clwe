#include "utils.hpp"
#include <cstring>
#include <chrono>
#include <iostream>

#ifdef HAVE_AVX2
#include <cpuid.h>
#endif

namespace clwe {


// AVX-Aligned Memory Allocator Implementation
void* AVXAllocator::allocate(size_t size) {
#ifdef HAVE_AVX2
    return _mm_malloc(size, 32);  // 32-byte alignment for AVX
#else
    return malloc(size);  // Standard allocation for non-AVX
#endif
}

void AVXAllocator::deallocate(void* ptr) {
#ifdef HAVE_AVX2
    _mm_free(ptr);
#else
    free(ptr);  // Standard deallocation for non-AVX
#endif
}

void* AVXAllocator::reallocate(void* ptr, size_t new_size) {
    if (ptr == nullptr) {
        return allocate(new_size);
    }
    if (new_size == 0) {
        deallocate(ptr);
        return nullptr;
    }
    void* new_ptr = allocate(new_size);
    if (new_ptr) {
        // We don't know the old size, so we can't copy safely
        // This is a limitation - reallocate should be used carefully
        deallocate(ptr);
    }
    return new_ptr;
}

// AVXVector Implementation
template<typename T>
AVXVector<T>::AVXVector() : data_(nullptr), size_(0), capacity_(0) {}

template<typename T>
AVXVector<T>::AVXVector(size_t initial_capacity)
    : data_(nullptr), size_(0), capacity_(0) {
    reserve(initial_capacity);
}

template<typename T>
AVXVector<T>::~AVXVector() {
    if (data_) {
        AVXAllocator::deallocate(data_);
    }
}

template<typename T>
AVXVector<T>::AVXVector(AVXVector&& other) noexcept
    : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}

template<typename T>
AVXVector<T>& AVXVector<T>::operator=(AVXVector&& other) noexcept {
    if (this != &other) {
        if (data_) {
            AVXAllocator::deallocate(data_);
        }
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    return *this;
}

template<typename T>
void AVXVector<T>::resize(size_t new_size) {
    if (new_size > capacity_) {
        reserve(new_size);
    }
    size_ = new_size;
}

template<typename T>
void AVXVector<T>::reserve(size_t new_capacity) {
    if (new_capacity > capacity_) {
        T* new_data = static_cast<T*>(AVXAllocator::allocate(new_capacity * sizeof(T)));
        if (data_) {
            memcpy(new_data, data_, size_ * sizeof(T));
            AVXAllocator::deallocate(data_);
        }
        data_ = new_data;
        capacity_ = new_capacity;
    }
}

template<typename T>
void AVXVector<T>::clear() {
    size_ = 0;
}

template<typename T>
void AVXVector<T>::push_back(const T& value) {
    if (size_ >= capacity_) {
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    }
    data_[size_++] = value;
}

template<typename T>
void AVXVector<T>::push_back(T&& value) {
    if (size_ >= capacity_) {
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    }
    data_[size_++] = std::move(value);
}

template<typename T>
void AVXVector<T>::pop_back() {
    if (size_ > 0) {
        --size_;
    }
}

// Explicit template instantiations
template class AVXVector<uint32_t>;
template class AVXVector<__m256i>;

// Utility functions
uint64_t get_timestamp_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

double timestamp_to_ms(uint64_t ts) {
    return static_cast<double>(ts) / 1e6;
}

// Montgomery reduction
uint32_t montgomery_reduce(uint64_t a, uint32_t q) {
    uint64_t t = a * (static_cast<uint64_t>(1) << 32) % q;
    return (a + t * q) >> 32;
}

uint32_t montgomery_reduce_avx(__m256i a, uint32_t q) {
    // This will be implemented in AVX-specific code
    // For now, return 0 as placeholder
    return 0;
}

// Barrett reduction
uint32_t barrett_reduce(uint64_t a, uint32_t q, uint64_t mu) {
    uint64_t t = (a * mu) >> 32;
    uint64_t r = a - t * q;
    if (r >= q) r -= q;
    return static_cast<uint32_t>(r);
}

// Bit operations
int bit_length(uint32_t x) {
    return 32 - __builtin_clz(x);
}

bool is_power_of_two(uint32_t x) {
    return (x & (x - 1)) == 0 && x != 0;
}

uint32_t next_power_of_two(uint32_t x) {
    if (x == 0) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x;
}

// Modular inverse using extended Euclidean algorithm
uint32_t mod_inverse(uint32_t a, uint32_t m) {
    int64_t m0 = m, t, q;
    int64_t x0 = 0, x1 = 1;
    if (m == 1) return 0;
    while (a > 1) {
        q = a / m;
        t = m;
        m = a % m, a = t;
        t = x0;
        x0 = x1 - q * x0;
        x1 = t;
    }
    if (x1 < 0) x1 += m0;
    return static_cast<uint32_t>(x1);
}

// Modular exponentiation using binary exponentiation
uint32_t mod_pow(uint32_t base, uint32_t exp, uint32_t mod) {
    uint32_t result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1) {
            result = (static_cast<uint64_t>(result) * base) % mod;
        }
        base = (static_cast<uint64_t>(base) * base) % mod;
        exp >>= 1;
    }
    return result;
}

} // namespace clwe