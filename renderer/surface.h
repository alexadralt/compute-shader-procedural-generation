#pragma once

#include "device.h"

#include <Volk/volk.h>
#include <SDL3/SDL_video.h>

#include <utility>

namespace rdr {
    class Surface {
        const Device* m_device;
        VkSurfaceKHR m_vk_surface;
        SDL_Window* m_window;

        Surface(const Surface& other) = delete;
        Surface& operator=(const Surface& other) = delete;

        void destroy();
    public:
        static std::pair<Surface, bool> create_window_and_surface(const Device& device, const char* window_title, Sint32 window_width, Sint32 window_height);

        Surface() : m_device(nullptr),
                    m_vk_surface(VK_NULL_HANDLE),
                    m_window(nullptr) {}
        ~Surface() { destroy(); }

        Surface(Surface&& other) noexcept : m_device(other.m_device),
                                            m_vk_surface(other.m_vk_surface),
                                            m_window(other.m_window) {
            new (&other) Surface();
        }

        Surface& operator=(Surface&& other) noexcept {
            destroy();
            new (this) Surface(std::move(other));
            return *this;
        }

        SDL_Window* window() const { return m_window; }
        std::pair<VkSurfaceCapabilitiesKHR, bool> get_surface_caps_khr() const;
    };
}
