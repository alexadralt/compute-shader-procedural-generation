#pragma once

#include "device.h"

#include <Volk/volk.h>
#include <SDL3/SDL_stdinc.h>

#include <utility>

namespace rdr {
    class Image {
        const Device* m_device;
        VkImage m_vk_image;

        Image(const Image& other) = delete;
        Image& operator=(const Image& other) = delete;

        void destroy();
    public:
        Image() : m_device(nullptr),
                  m_vk_image(VK_NULL_HANDLE) {}
        Image(VkImage vk_image) : m_device(nullptr),
                                  m_vk_image(vk_image) {}

        ~Image() { destroy(); }

        Image(Image&& other) noexcept : m_vk_image(other.m_vk_image) {
            new (&other) Image();
        }

        Image& operator=(Image&& other) noexcept {
            destroy();
            new (this) Image(std::move(other));
            return *this;
        }
    };
}
