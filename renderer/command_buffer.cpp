#include "command_buffer.h"

void rdr::Command_Buffer::pipeline_barrier(std::span<VkImageMemoryBarrier2> image_barriers, std::span<VkBufferMemoryBarrier2> buffer_barriers) const
{
    VkDependencyInfo dependency_info{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barriers.size()),
        .pBufferMemoryBarriers = buffer_barriers.data(),
        .imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers.size()),
        .pImageMemoryBarriers = image_barriers.data(),
    };
    vkCmdPipelineBarrier2(m_vk_command_buffer, &dependency_info);
}