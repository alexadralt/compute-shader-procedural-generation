#include "image_view.h"

#include <print>

rdr::Image_View::~Image_View()
{
    if (m_device && m_vk_image_view != VK_NULL_HANDLE) {
        std::println("destroying image view...");
        vkDestroyImageView(m_device->vk_device(), m_vk_image_view, nullptr);
    }
}

bool rdr::Image_View::create(const Device& device, const Image& image, VkImageViewCreateFlags flags, VkFormat format, Image_View& out_image_view)
{
    VkImageViewCreateInfo image_view_CI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = flags,
        .image = image.vk_image(),
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };

    return create(device, image_view_CI, out_image_view);
}

bool rdr::Image_View::create(const Device& device, const VkImageViewCreateInfo& create_info, Image_View& out_image_view)
{
    std::println("creating image view...");

    Image_View image_view;
    image_view.m_device = &device;

    VkResult result = vkCreateImageView(device.vk_device(), &create_info, nullptr, &image_view.m_vk_image_view);
    if (result != VK_SUCCESS) {
        std::println("Could not create image view: {}", static_cast<int32_t>(result));
        return false;
    }

    out_image_view = std::move(image_view);
    return true;
}
