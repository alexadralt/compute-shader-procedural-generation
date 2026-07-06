#include "queue.h"

#include "semaphore.h"
#include "fence.h"

#include <cassert>
#include <vector>
#include <print>

bool rdr::Queue::submit(std::span<Semaphore> wait_semaphores, std::span<VkPipelineStageFlags> wait_flags, std::span<Command_Buffer> command_buffers, std::span<Semaphore> signal_semaphores, const Fence& fence) const
{
    assert(wait_semaphores.size() == wait_flags.size());

    std::vector<VkSemaphore> vk_wait_semaphores(wait_semaphores.size());
    for (size_t i = 0; i < wait_semaphores.size(); ++i) {
        vk_wait_semaphores[i] = wait_semaphores[i].vk_semaphore();
    }

    std::vector<VkCommandBuffer> vk_command_buffers(command_buffers.size());
    for (size_t i = 0; i < command_buffers.size(); ++i) {
        vk_command_buffers[i] = command_buffers[i].vk_command_buffer();
    }

    std::vector<VkSemaphore> vk_signal_semaphores(signal_semaphores.size());
    for (size_t i = 0; i < signal_semaphores.size(); ++i) {
        vk_signal_semaphores[i] = signal_semaphores[i].vk_semaphore();
    }

    VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size()),
        .pWaitSemaphores = vk_wait_semaphores.data(),
        .pWaitDstStageMask = wait_flags.data(),
        .commandBufferCount = static_cast<uint32_t>(command_buffers.size()),
        .pCommandBuffers = vk_command_buffers.data(),
        .signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size()),
        .pSignalSemaphores = vk_signal_semaphores.data(),
    };

    VkResult result = vkQueueSubmit(m_vk_queue, 1, &submit_info, fence.vk_fence());
    if (result != VK_SUCCESS) {
        std::println("Could not submit to queue: {}", static_cast<int32_t>(result));
        return false;
    }

    return true;
}
