#pragma once

#include "device.h"
#include "image.h"

#include <Volk/volk.h>

namespace rdr {
    class Image_View {
        const Device* m_device;
        VkImageView m_vk_image_view;

        Image_View(const Image_View& other) = delete;
        Image_View& operator=(const Image_View& other) = delete;
    public:
        Image_View() : m_device(nullptr),
                       m_vk_image_view(VK_NULL_HANDLE) {}
        ~Image_View();

        static bool create(const Device& device, const Image& image, VkImageViewCreateFlags flags, VkFormat format, Image_View& out_image_view);
        static bool create(const Device& device, const VkImageViewCreateInfo& create_info, Image_View& out_image_view);

        Image_View(Image_View&& other) noexcept : m_device(other.m_device),
                                                  m_vk_image_view(other.m_vk_image_view)
        {
            new (&other) Image_View();
        }

        Image_View& operator=(Image_View&& other) noexcept {
            this->~Image_View();
            new (this) Image_View(std::move(other));
            return *this;
        }

        VkImageView vk_image_view() const { return m_vk_image_view; }
    };
}
