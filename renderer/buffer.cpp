#include "buffer.h"

#include <print>

#if LOG_RENDERER_OBJECT_NAMES
#include <format>
#endif

void rdr::Buffer::destroy()
{
    if (m_allocator != nullptr && m_vma_allocation != VK_NULL_HANDLE && m_vk_buffer != VK_NULL_HANDLE) {
#if LOG_RENDERER_OBJECT_NAMES
        std::println("destroying {}...", m_name);
#else
        std::println("destroying buffer...");
#endif
        vmaDestroyBuffer(m_allocator->vma_allocator(), m_vk_buffer, m_vma_allocation);
    }
}

bool rdr::Buffer::create(const Allocator& allocator, Uint64 size, VkBufferUsageFlags buffer_usage, VmaAllocationCreateFlags allocation_flags, Buffer& out_buffer)
{
    std::println("creating vk buffer...");

    Buffer buffer;
    buffer.m_allocator = &allocator;
#if LOG_RENDERER_OBJECT_NAMES
    static uint64_t name_index = 0;
    buffer.m_name = std::format("buffer #{}", name_index++);
#endif

    VkBufferCreateInfo buffer_CI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = buffer_usage,
    };

    VmaAllocationCreateInfo allocation_CI{
        .flags = allocation_flags,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VmaAllocationInfo allocation_info{};
    VkResult result = vmaCreateBuffer(allocator.vma_allocator(), &buffer_CI, &allocation_CI, &buffer.m_vk_buffer, &buffer.m_vma_allocation, &allocation_info);
    if (result != VK_SUCCESS) {
        std::println("Could not create vk buffer: {}", static_cast<Sint32>(result));
        return false;
    }

    if (buffer_usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkBufferDeviceAddressInfo vk_buffer_device_address_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer.vk_buffer(),
        };
        buffer.m_vk_device_address = vkGetBufferDeviceAddress(allocator.device()->vk_device(), &vk_buffer_device_address_info);
    }

    buffer.m_mapped_data = allocation_info.pMappedData;

    out_buffer = std::move(buffer);
    return true;
}
