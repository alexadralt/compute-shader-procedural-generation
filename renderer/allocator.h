#pragma once

#include "device.h"

#include <vma/vk_mem_alloc.h>
#include <Volk/volk.h>

#include <optional>

namespace rdr {
    class Allocator {
        VmaAllocator m_vma_allocator;

        Allocator(const Allocator& other) = delete;
        Allocator& operator=(const Allocator& other) = delete;

        void destroy();
    public:
        static std::optional<Allocator> create(const Device& device);
        
        Allocator() : m_vma_allocator(VK_NULL_HANDLE) {}
        ~Allocator() { destroy(); }

        Allocator(Allocator&& other) noexcept : m_vma_allocator(other.m_vma_allocator) {
            new (&other) Allocator();
        }

        Allocator& operator=(Allocator&& other) noexcept {
            destroy();
            new (this) Allocator(std::move(other));
            return *this;
        }
    };
}
