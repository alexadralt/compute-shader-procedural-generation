#pragma once

#include "command_buffer.h"

#include <Volk/volk.h>

#include <span>

namespace rdr {
    class Semaphore;
    class Fence;

    class Queue {
        VkQueue m_vk_queue;

    public:
        Queue() : m_vk_queue(VK_NULL_HANDLE) {}
        Queue(VkQueue queue) : m_vk_queue(queue) {}

        bool submit(std::span<Semaphore> wait_semaphores, std::span<VkPipelineStageFlags> wait_flags, std::span<Command_Buffer> command_buffers, std::span<Semaphore> signal_semaphores, const Fence& fence) const;

        VkQueue vk_queue() const { return m_vk_queue; };
    };
}
