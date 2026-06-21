#include "image.h"

#include <print>

void rdr::Image::destroy()
{
    if (m_device != nullptr && m_vk_image != VK_NULL_HANDLE) {
        std::println("destroying image...");
        vkDestroyImage(m_device->vk_device(), m_vk_image, nullptr);
    }
}
