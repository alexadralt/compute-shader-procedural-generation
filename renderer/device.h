#pragma once

#include "queue.h"

#include <Volk/volk.h>
#include <SDL3/SDL_stdinc.h>

#include <vector>

namespace rdr {
    class Device {
        VkInstance m_vk_instance;
        VkDevice m_vk_device;
        VkPhysicalDevice m_vk_phys_device;
        Uint32 m_vk_queue_family_index;

        Device(const Device& other) = delete;
        Device& operator=(const Device& other) = delete;

        void destroy();

        static std::vector<const char*> get_validation_layers();
    public:
        Device() : m_vk_instance(VK_NULL_HANDLE),
            m_vk_device(VK_NULL_HANDLE),
            m_vk_phys_device(VK_NULL_HANDLE),
            m_vk_queue_family_index(0) {}
        ~Device() { destroy(); }

        Device(Device&& other) noexcept : m_vk_instance(other.m_vk_instance),
                                          m_vk_device(other.m_vk_device),
                                          m_vk_phys_device(other.m_vk_phys_device),
                                          m_vk_queue_family_index(other.m_vk_queue_family_index)
        {
            new (&other) Device();
        }
        
        Device& operator=(Device&& other) noexcept {
            destroy();
            new (this) Device(std::move(other));
            return *this;
        }

        static bool create(Device& out_device);

        VkInstance vk_instance() const { return m_vk_instance; }
        VkDevice vk_device() const { return m_vk_device; }
        VkPhysicalDevice vk_phys_device() const { return m_vk_phys_device; }
        Uint32 vk_queue_family_index() const { return m_vk_queue_family_index; }
        Queue get_device_queue(uint32_t queue_index) const;
    };
}
