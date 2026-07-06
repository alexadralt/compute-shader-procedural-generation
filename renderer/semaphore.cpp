#include "semaphore.h"

#include <print>

rdr::Semaphore::~Semaphore()
{
    if (m_device && m_vk_semaphore != VK_NULL_HANDLE) {
        std::println("destroying semaphore...");
        vkDestroySemaphore(m_device->vk_device(), m_vk_semaphore, nullptr);
    }
}

bool rdr::Semaphore::create(const Device& device, Semaphore& out_semaphore)
{
    std::println("creating semaphore...");

    Semaphore semaphore;
    semaphore.m_device = &device;

    VkSemaphoreCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkResult result = vkCreateSemaphore(device.vk_device(), &create_info, nullptr, &semaphore.m_vk_semaphore);
    if (result != VK_SUCCESS) {
        std::println("Could not create semaphore: {}", static_cast<int32_t>(result));
        return false;
    }

    out_semaphore = std::move(semaphore);
    return true;
}
