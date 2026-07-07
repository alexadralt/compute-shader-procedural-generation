#pragma once

#include "device.h"

#include <Volk/volk.h>

#include <utility>
#include <span>

namespace rdr {
    class Fence {
        const Device* m_device;
        VkFence m_vk_fence;

        Fence(const Fence& other) = delete;
        Fence& operator=(const Fence& other) = delete;
    public:
        Fence() : m_device(nullptr),
                  m_vk_fence(VK_NULL_HANDLE) {}
        ~Fence();

        static bool create(const Device& device, bool signaled, Fence& out_fence);

        Fence(Fence&& other) noexcept : m_device(other.m_device),
                                        m_vk_fence(other.m_vk_fence)
        {
            new (&other) Fence();
        }

        Fence& operator=(Fence&& other) noexcept {
            this->~Fence();
            new (this) Fence(std::move(other));
            return *this;
        }

        VkFence vk_fence() const { return m_vk_fence; }
        bool wait(uint64_t timeout = 0xFFFFFFFFFFFFFFFF) const;
        bool reset() const;
    };
}
