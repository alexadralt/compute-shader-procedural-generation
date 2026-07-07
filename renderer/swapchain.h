#pragma once

#include "surface.h"
#include "device.h"
#include "image.h"
#include "semaphore.h"

#include <Volk/volk.h>

#include <vector>

namespace rdr {
    class Swapchain {
        const Device* m_device;
        VkSwapchainKHR m_vk_swapchain;
        VkFormat m_image_format;

        Swapchain(const Swapchain& other) = delete;
        Swapchain& operator=(const Swapchain& other) = delete;

        void destroy();
    public:
        Swapchain() : m_device(nullptr),
                      m_vk_swapchain(VK_NULL_HANDLE),
                      m_image_format(VK_FORMAT_UNDEFINED) {}

        ~Swapchain() { destroy(); }

        Swapchain(Swapchain&& other) noexcept : m_device(other.m_device),
                                                m_vk_swapchain(other.m_vk_swapchain),
                                                m_image_format(other.m_image_format)
        {
            new (&other) Swapchain();
        }

        Swapchain& operator=(Swapchain&& other) noexcept {
            destroy();
            new (this) Swapchain(std::move(other));
            return *this;
        }

        static bool create(const Device& device, const Surface& surface, uint32_t frames_in_filght, Swapchain& out_swapchain);

        const VkFormat& image_format() const { return m_image_format; }
        const VkSwapchainKHR& vk_swapchain() const { return m_vk_swapchain; }
        std::vector<Image> get_swapchain_images_khr() const;
        
        VkResult acquire_next_image(const Semaphore& semaphore, uint32_t& out_index, uint64_t timeout = 0xFFFFFFFFFFFFFFFF) const {
            return vkAcquireNextImageKHR(m_device->vk_device(), m_vk_swapchain, timeout, semaphore.vk_semaphore(), VK_NULL_HANDLE, &out_index);
        }
    };
}
