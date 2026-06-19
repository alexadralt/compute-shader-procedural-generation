#include "surface.h"

#include <SDL3/SDL_vulkan.h>

#include <print>

void rdr::Surface::destroy()
{
    if (m_vk_surface != VK_NULL_HANDLE) {
        std::println("destroying vk surface...");
        vkDestroySurfaceKHR(m_device->vk_instance(), m_vk_surface, nullptr);
    }

    if (m_window != nullptr) {
        std::println("destroying window...");
        SDL_DestroyWindow(m_window);
    }
}

std::pair<rdr::Surface, bool> rdr::Surface::create_window_and_surface(const Device& device, const char* window_title, Sint32 window_width, Sint32 window_height)
{
    std::println("creating window and vk surface...");

    Surface surface;
    surface.m_device = &device;

    surface.m_window = SDL_CreateWindow(window_title, window_width, window_height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    if (!surface.m_window) {
        std::println("failed to create window: {}", SDL_GetError());
        return { Surface(), false };
    }

    if (!SDL_Vulkan_CreateSurface(surface.m_window, device.vk_instance(), nullptr, &surface.m_vk_surface)) {
        std::println("failed to create vk surface: {}", SDL_GetError());
        return { Surface(), false };
    }

    return { std::move(surface), true };
}

std::pair<VkSurfaceCapabilitiesKHR, bool> rdr::Surface::get_surface_caps_khr() const
{
    VkSurfaceCapabilitiesKHR caps{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device->vk_phys_device(), m_vk_surface, &caps) != VK_SUCCESS) {
        std::println("failed to get vk surface capabilities");
        return { caps, false };
    }

    return { caps, true };
}
