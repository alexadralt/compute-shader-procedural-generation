#pragma once

#include "device.h"

#include <Volk/volk.h>

#include <utility>

namespace rdr {
    class Semaphore {
        const Device* m_device;
        VkSemaphore m_vk_semaphore;

        Semaphore(const Semaphore& other) = delete;
        Semaphore& operator=(const Semaphore& other) = delete;
    public:
        Semaphore() : m_device(nullptr),
                      m_vk_semaphore(VK_NULL_HANDLE) {}
        ~Semaphore();

        static bool create(const Device& device, Semaphore& out_semaphore);

        Semaphore(Semaphore&& other) noexcept : m_device(other.m_device),
                                                m_vk_semaphore(other.m_vk_semaphore)
        {
            new (&other) Semaphore();
        }

        Semaphore& operator=(Semaphore&& other) noexcept {
            this->~Semaphore();
            new (this) Semaphore(std::move(other));
            return *this;
        }
        
        VkSemaphore vk_semaphore() const { return m_vk_semaphore; }
    };
}
