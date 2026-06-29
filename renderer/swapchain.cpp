#include "swapchain.h"

#include <print>

void rdr::Swapchain::destroy()
{
    if (m_vk_swapchain != VK_NULL_HANDLE) {
        std::println("destroying swapchain...");
        vkDestroySwapchainKHR(m_device->vk_device(), m_vk_swapchain, nullptr);
    }
}

std::optional<rdr::Swapchain> rdr::Swapchain::create(const Device& device, const Surface& surface)
{
    std::println("creating swapchain...");
    
    Swapchain swapchain;
    swapchain.m_device = &device;
    swapchain.m_image_format = VK_FORMAT_B8G8R8A8_SRGB;

    auto surface_caps_result = surface.get_surface_caps_khr();
    if (!surface_caps_result.has_value()) {
        return std::nullopt;
    }

    auto& surface_caps = surface_caps_result.value();
    VkExtent2D swapchain_extent = surface_caps.currentExtent;
    
    VkSwapchainCreateInfoKHR swapchain_CI{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface.vk_surface(),
        .minImageCount = surface_caps.minImageCount,
        .imageFormat = swapchain.m_image_format,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .imageExtent{.width = swapchain_extent.width, .height = swapchain_extent.height },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR // <--- vsync
    };
    if (vkCreateSwapchainKHR(device.vk_device(), &swapchain_CI, nullptr, &swapchain.m_vk_swapchain) != VK_SUCCESS) {
        std::println("failed to create swapchain");
        return std::nullopt;
    }

    return swapchain;
}

std::vector<rdr::Image> rdr::Swapchain::get_swapchain_images_khr() const
{
    Uint32 image_count = 0;
    if (vkGetSwapchainImagesKHR(m_device->vk_device(), m_vk_swapchain, &image_count, nullptr) != VK_SUCCESS) {
        std::println("could not get swapchain images count");
        return std::vector<rdr::Image>();
    }

    std::vector<VkImage> swapchain_vk_images(image_count);
    if (vkGetSwapchainImagesKHR(m_device->vk_device(), m_vk_swapchain, &image_count, swapchain_vk_images.data()) != VK_SUCCESS) {
        std::println("could not get swapchain images");
        return std::vector<rdr::Image>();
    }

    std::vector<Image> swapchain_images;
    swapchain_images.reserve(image_count);
    for (auto vk_image : swapchain_vk_images) {
        swapchain_images.emplace_back(vk_image);
    }

    return swapchain_images;
}
