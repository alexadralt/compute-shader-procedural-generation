#pragma once

#include "device.h"

#include <vma/vk_mem_alloc.h>
#include <Volk/volk.h>

namespace rdr {
    class Allocator {
        const Device* m_device;
        VmaAllocator m_vma_allocator;

        Allocator(const Allocator& other) = delete;
        Allocator& operator=(const Allocator& other) = delete;

        void destroy();
    public:
        static bool create(const Device& device, Allocator& out_allocator);
        
        Allocator() : m_device(nullptr),
                      m_vma_allocator(VK_NULL_HANDLE) {}
        ~Allocator() { destroy(); }

        Allocator(Allocator&& other) noexcept : m_device(other.m_device),
                                                m_vma_allocator(other.m_vma_allocator)
        {
            new (&other) Allocator();
        }

        Allocator& operator=(Allocator&& other) noexcept {
            destroy();
            new (this) Allocator(std::move(other));
            return *this;
        }

        VmaAllocator vma_allocator() const { return m_vma_allocator; }
        const Device* device() const { return m_device; }
    };
}
