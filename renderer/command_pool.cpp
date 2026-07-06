#include "command_pool.h"

#include <print>
#include <vector>
#include <ranges>

rdr::Command_Pool::~Command_Pool()
{
    if (m_device && m_vk_command_pool != VK_NULL_HANDLE) {
        std::println("destroying command pool...");
        vkDestroyCommandPool(m_device->vk_device(), m_vk_command_pool, nullptr);
    }
}

bool rdr::Command_Pool::create(const Device& device, uint32_t queue_family_index, VkCommandPoolCreateFlags flags, Command_Pool& out_command_pool)
{
    std::println("creating command pool...");

    Command_Pool command_pool;
    command_pool.m_device = &device;

    VkCommandPoolCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = flags,
        .queueFamilyIndex = queue_family_index,
    };

    VkResult result = vkCreateCommandPool(device.vk_device(), &create_info, nullptr, &command_pool.m_vk_command_pool);
    if (result != VK_SUCCESS) {
        std::println("Could not create command pool: {}", static_cast<int32_t>(result));
        return false;
    }

    out_command_pool = std::move(command_pool);
    return true;
}

bool rdr::Command_Pool::allocate_command_buffers(std::span<Command_Buffer> out_command_buffers) const
{
    VkCommandBufferAllocateInfo allocate_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_vk_command_pool,
        .commandBufferCount = static_cast<uint32_t>(out_command_buffers.size()),
    };

    std::vector<VkCommandBuffer> vk_command_buffers(out_command_buffers.size());
    VkResult result = vkAllocateCommandBuffers(m_device->vk_device(), &allocate_info, vk_command_buffers.data());
    if (result != VK_SUCCESS) {
        std::println("Could not allocate command buffers: {}", static_cast<int32_t>(result));
        return false;
    }

    for (auto [i, out_command_buffer] : std::views::enumerate(out_command_buffers)) {
        out_command_buffer = Command_Buffer(vk_command_buffers[i]);
    }
    return true;
}
