#pragma once

#include <Volk/volk.h>
#include <SDL3/SDL_stdinc.h>

#include <utility>

namespace rdr {
    class Device {
        VkInstance m_vk_instance;
        VkDevice m_vk_device;
        Uint32 m_vk_queue_family_index;

        Device(const Device& other) = default;
        Device& operator=(const Device& other) = default;

        void destroy();
    public:
        Device() : m_vk_instance(VK_NULL_HANDLE),
            m_vk_device(VK_NULL_HANDLE),
            m_vk_queue_family_index(0) {}
        ~Device() { destroy(); }

        Device(Device&& other) noexcept : m_vk_instance(other.m_vk_instance),
                                          m_vk_device(other.m_vk_device),
                                          m_vk_queue_family_index(other.m_vk_queue_family_index) {
            new (&other) Device();
        }
        
        Device& operator=(Device&& other) noexcept {
            new (this) Device(std::move(other));
            return *this;
        }

        static std::pair<Device, bool> create();

        VkDevice vk_device() const { return m_vk_device; }
        Uint32 vk_queue_family_index() const { return m_vk_queue_family_index; }
    };
}
