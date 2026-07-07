#pragma once

#include <Volk/volk.h>

#include <span>

namespace rdr {
    class Command_Buffer {
        VkCommandBuffer m_vk_command_buffer;

    public:
        Command_Buffer() : m_vk_command_buffer(VK_NULL_HANDLE) {}
        Command_Buffer(VkCommandBuffer command_buffer) : m_vk_command_buffer(command_buffer) {}

        VkCommandBuffer vk_command_buffer() const { return m_vk_command_buffer; }

        void pipeline_barrier(std::span<VkImageMemoryBarrier2> image_barriers, std::span<VkBufferMemoryBarrier2> buffer_barriers) const;
    };
}
