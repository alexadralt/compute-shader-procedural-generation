#include "buffer.h"

#include <print>

void rdr::Buffer::destroy()
{
    if (m_allocator != nullptr && m_vma_allocation != VK_NULL_HANDLE && m_vk_buffer != VK_NULL_HANDLE) {
        std::println("destroying vk buffer...");
        vmaDestroyBuffer(m_allocator->vma_allocator(), m_vk_buffer, m_vma_allocation);
    }
}

bool rdr::Buffer::create(const Allocator& allocator, Uint64 size, VkBufferUsageFlags buffer_usage, VmaAllocationCreateFlags allocation_flags, Buffer& out_buffer)
{
    std::println("creating vk buffer...");

    Buffer buffer;
    buffer.m_allocator = &allocator;

    VkBufferCreateInfo buffer_CI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = buffer_usage,
    };

    VmaAllocationCreateInfo allocation_CI{
        .flags = allocation_flags,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VkResult result = vmaCreateBuffer(allocator.vma_allocator(), &buffer_CI, &allocation_CI, &buffer.m_vk_buffer, &buffer.m_vma_allocation, nullptr);
    if (result != VK_SUCCESS) {
        std::println("Could not create vk buffer: {}", static_cast<Sint32>(result));
        return false;
    }

    out_buffer = std::move(buffer);
    return true;
}
