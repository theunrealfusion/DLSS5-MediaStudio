//----------------------------------------------------------------------------------
// File:        MemorySafety.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Safe memory primitives, COM wrappers, and integer overflow guards
//----------------------------------------------------------------------------------

#pragma once

#if defined(_WIN32)
#include <wrl/client.h>
template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
#endif

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <string>

namespace EnterpriseSecurity {

    // Checked arithmetic for image width/height/pitch/stride calculations
    inline uint32_t SafeMultiply(uint32_t a, uint32_t b) {
        uint64_t result = static_cast<uint64_t>(a) * static_cast<uint64_t>(b);
        if (result > UINT32_MAX) {
            throw std::overflow_error("Integer overflow detected in dimension calculation");
        }
        return static_cast<uint32_t>(result);
    }

    inline uint32_t SafeAdd(uint32_t a, uint32_t b) {
        uint64_t result = static_cast<uint64_t>(a) + static_cast<uint64_t>(b);
        if (result > UINT32_MAX) {
            throw std::overflow_error("Integer overflow detected in buffer offset calculation");
        }
        return static_cast<uint32_t>(result);
    }

    // Align size up to pitch alignment safely
    inline uint32_t AlignUp(uint32_t value, uint32_t alignment) {
        if (alignment == 0) return value;
        uint32_t mask = alignment - 1;
        return (value + mask) & ~mask;
    }

    // Secure zeroing of sensitive buffers
    inline void SecureZero(void* ptr, size_t size) {
        if (!ptr || size == 0) return;
        volatile uint8_t* p = static_cast<volatile uint8_t*>(ptr);
        while (size--) {
            *p++ = 0;
        }
    }

} // namespace EnterpriseSecurity
