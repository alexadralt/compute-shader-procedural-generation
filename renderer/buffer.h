#pragma once

#include "allocator.h"
#include "device.h"

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>
#include <SDL3/SDL_stdinc.h>

#include <utility>

#if LOG_RENDERER_OBJECT_NAMES
#include <string>
#endif

namespace rdr {
    class Buffer {
        const Allocator* m_allocator;
        VmaAllocation m_vma_allocation;
        VkBuffer m_vk_buffer;
        VkDeviceAddress m_vk_device_address;
        void* m_mapped_data;
        uint64_t m_buffer_size;
#if LOG_RENDERER_OBJECT_NAMES
        std::string m_name;
#endif

        Buffer(const Buffer& other) = delete;
        Buffer& operator=(const Buffer& other) = delete;

        void destroy();
    public:
        Buffer() : m_allocator(nullptr),
                   m_vma_allocation(VK_NULL_HANDLE),
                   m_vk_buffer(VK_NULL_HANDLE),
                   m_vk_device_address(0),
                   m_mapped_data(nullptr),
                   m_buffer_size(0) {}
        ~Buffer() { destroy(); }

        Buffer(Buffer&& other) noexcept : m_allocator(other.m_allocator),
                                          m_vma_allocation(other.m_vma_allocation),
                                          m_vk_buffer(other.m_vk_buffer),
                                          m_vk_device_address(other.m_vk_device_address),
                                          m_mapped_data(other.m_mapped_data),
                                          m_buffer_size(other.m_buffer_size)
#if LOG_RENDERER_OBJECT_NAMES
                                        , m_name(std::move(other.m_name))
#endif
        {
            new (&other) Buffer();
        }

        Buffer& operator=(Buffer&& other) noexcept {
            destroy();
            new (this) Buffer(std::move(other));
            return *this;
        }

        static bool create(const Allocator& allocator, Uint64 size, VkBufferUsageFlags buffer_usage, VmaAllocationCreateFlags allocation_flags, Buffer& out_buffer);

        const VkBuffer& vk_buffer() const { return m_vk_buffer; }
        const VkDeviceAddress& vk_device_address() const { return m_vk_device_address; }
        void* const& mapped_data() const { return m_mapped_data; }
        const uint64_t& buffer_size() const { return m_buffer_size; }
    };
}
