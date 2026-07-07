#include "fence.h"

#include <print>

rdr::Fence::~Fence()
{
    if (m_device && m_vk_fence != VK_NULL_HANDLE) {
        std::println("destroying fence...");
        vkDestroyFence(m_device->vk_device(), m_vk_fence, nullptr);
    }
}

bool rdr::Fence::create(const Device& device, bool signaled, Fence& out_fence)
{
    std::println("craeting fence...");

    Fence fence;
    fence.m_device = &device;
    
    VkFenceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : static_cast<uint32_t>(0),
    };

    VkResult result = vkCreateFence(device.vk_device(), &create_info, nullptr, &fence.m_vk_fence);
    if (result != VK_SUCCESS) {
        std::println("Could not create fence: {}", static_cast<int32_t>(result));
        return false;
    }

    out_fence = std::move(fence);
    return true;
}

bool rdr::Fence::wait(uint64_t timeout) const
{
    VkResult result = vkWaitForFences(m_device->vk_device(), 1, &m_vk_fence, true, timeout);
    if (result != VK_SUCCESS) {
        std::println("Could not wait for fence: {}", static_cast<int32_t>(result));
    }

    return result == VK_SUCCESS;
}

bool rdr::Fence::reset() const
{
    VkResult result = vkResetFences(m_device->vk_device(), 1, &m_vk_fence);
    if (result != VK_SUCCESS) {
        std::println("Could not reset fence: {}", static_cast<int32_t>(result));
    }

    return result == VK_SUCCESS;
}
