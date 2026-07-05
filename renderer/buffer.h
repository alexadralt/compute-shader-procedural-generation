#pragma once

#include "allocator.h"
#include "device.h"

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>
#include <SDL3/SDL_stdinc.h>

namespace rdr {
    class Buffer {
        const Allocator* m_allocator;
        VmaAllocation m_vma_allocation;
        VkBuffer m_vk_buffer;

        Buffer(const Buffer& other) = delete;
        Buffer& operator=(const Buffer& other) = delete;

        void destroy();
    public:
        Buffer() : m_allocator(nullptr),
                   m_vma_allocation(VK_NULL_HANDLE),
                   m_vk_buffer(VK_NULL_HANDLE) {}
        ~Buffer() { destroy(); }

        Buffer(Buffer&& other) noexcept : m_allocator(other.m_allocator),
                                          m_vma_allocation(other.m_vma_allocation),
                                          m_vk_buffer(other.m_vk_buffer)
        {
            new (&other) Buffer();
        }

        Buffer& operator=(Buffer&& other) noexcept {
            destroy();
            new (this) Buffer(std::move(other));
            return *this;
        }

        static bool create(const Allocator& allocator, Uint64 size, VkBufferUsageFlags buffer_usage, VmaAllocationCreateFlags allocation_flags, Buffer& out_buffer);

        VkBuffer vk_buffer() const { return m_vk_buffer; }
    };
}
