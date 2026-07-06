#pragma once

#include "device.h"
#include "command_buffer.h"

#include <Volk/volk.h>

#include <utility>
#include <span>

namespace rdr {
    class Command_Pool {
        const Device* m_device;
        VkCommandPool m_vk_command_pool;

        Command_Pool(const Command_Pool& other) = delete;
        Command_Pool& operator=(const Command_Pool& other) = delete;
    public:
        Command_Pool() : m_device(nullptr),
                         m_vk_command_pool(VK_NULL_HANDLE) {}
        ~Command_Pool();

        static bool create(const Device& device, uint32_t queue_family_index, VkCommandPoolCreateFlags flags, Command_Pool& out_command_pool);

        Command_Pool(Command_Pool&& other) noexcept : m_device(other.m_device),
                                                      m_vk_command_pool(other.m_vk_command_pool)
        {
            new (&other) Command_Pool();
        }

        Command_Pool& operator=(Command_Pool&& other) noexcept {
            this->~Command_Pool();
            new (this) Command_Pool(std::move(other));
            return *this;
        }

        bool allocate_command_buffers(std::span<Command_Buffer> out_command_buffers) const;

        VkCommandPool vk_command_pool() const { return m_vk_command_pool; }
        const Device* device() const { return m_device; }
    };
}