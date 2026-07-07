#include "app.h"

#include <renderer/shader_compiler.h>

#include <print>
#include <cassert>

#define Vk_Check(call) \
    if ((call) != VK_SUCCESS) \
        assert(false)
#define Check(call) \
    if (!(call)) \
        assert(false)

void App::quit()
{
    if (m_window) {
        SDL_DestroyWindow(m_window);
    }

    SDL_Quit();
}

void App::process_events(bool& running)
{
    for (SDL_Event event; SDL_PollEvent(&event);) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
            {
                std::println("quitting application...");
                running = false;
            } break;
        }
    }
}

void App::update(float dt)
{
    
}

void App::render(float dt)
{
    Check(m_next_frame_fences[m_frame_index].wait());
    Check(m_next_frame_fences[m_frame_index].reset());

    uint32_t image_index;
    Vk_Check(m_rdr_swapchain.acquire_next_image(m_wait_image_acquired_semaphores[m_frame_index], image_index));

    memcpy(m_terrain_gen_shader_data_buffers[m_frame_index].mapped_data(), &m_terrain_gen_shader_data, sizeof(Terrain_Gen_Shader_Data));

    auto& cmd = m_command_buffers[m_frame_index];
    Vk_Check(vkResetCommandBuffer(cmd.vk_command_buffer(), 0));

    VkCommandBufferBeginInfo cmd_begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    Vk_Check(vkBeginCommandBuffer(cmd.vk_command_buffer(), &cmd_begin_info));

    std::array<VkImageMemoryBarrier2, 1> image_barriers{
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_terrain_height_map_image.vk_image(),
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            },
        },
    };
    VkBufferMemoryBarrier2 buffer_barrier{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = m_terrain_heght_map_buffer.vk_buffer(),
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };
    VkDependencyInfo dependency_info{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &buffer_barrier,
        .imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers.size()),
        .pImageMemoryBarriers = image_barriers.data(),
    };
    vkCmdPipelineBarrier2(cmd.vk_command_buffer(), &dependency_info);

    vkCmdBindPipeline(cmd.vk_command_buffer(), VK_PIPELINE_BIND_POINT_COMPUTE, m_compute_pipelines[Compute_Pipelines_Terrain_Gen].vk_pipeline());
    
    vkCmdBindDescriptorSets(cmd.vk_command_buffer(), VK_PIPELINE_BIND_POINT_COMPUTE,
                            m_compute_pipeline_layouts[Compute_Pipelines_Terrain_Gen].vk_pipeline_layout(),
                            0, 1, &m_terrain_gen_descriptor_set.vk_descriptor_set(), 0, nullptr);

    vkCmdPushConstants(cmd.vk_command_buffer(), m_compute_pipeline_layouts[Compute_Pipelines_Terrain_Gen].vk_pipeline_layout(),
                       VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(VkDeviceAddress), &m_terrain_gen_shader_data_buffers[m_frame_index].vk_device_address());

    vkCmdDispatch(cmd.vk_command_buffer(), 128, 128, 1);

    std::array<VkImageMemoryBarrier2, 2> blit_image_barriers{
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_rdr_swapchain_images[image_index].vk_image(),
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            },
        },
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_terrain_height_map_image.vk_image(),
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            },
        }
    };
    VkDependencyInfo blit_barrier_dependency_info{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = static_cast<uint32_t>(blit_image_barriers.size()),
        .pImageMemoryBarriers = blit_image_barriers.data(),
    };
    vkCmdPipelineBarrier2(cmd.vk_command_buffer(), &blit_barrier_dependency_info);

    VkClearColorValue clear_color = { 0, 0, 0, 0 };
    VkImageSubresourceRange clear_subresource_range{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1
    };
    vkCmdClearColorImage(cmd.vk_command_buffer(), m_rdr_swapchain_images[image_index].vk_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &clear_subresource_range);

    VkSurfaceCapabilitiesKHR surface_caps{};
    Check(m_rdr_surface.get_surface_caps_khr(surface_caps));
    auto& extent = surface_caps.currentExtent;

    int32_t blit_size_x = static_cast<int32_t>(std::min(extent.width, Terrain_Size));
    int32_t blit_size_y = static_cast<int32_t>(std::min(extent.height, Terrain_Size));

    VkImageBlit blit{
        .srcSubresource = VkImageSubresourceLayers{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .srcOffsets = {
            VkOffset3D{ 0, 0, 0 },
            VkOffset3D{ blit_size_x, blit_size_y, 1 }
        },
        .dstSubresource = VkImageSubresourceLayers{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .dstOffsets = {
            VkOffset3D{ 0, 0, 0 },
            VkOffset3D{ blit_size_x, blit_size_y, 1 }
        },
    };
    vkCmdBlitImage(cmd.vk_command_buffer(),
                   m_terrain_height_map_image.vk_image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   m_rdr_swapchain_images[image_index].vk_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit, VK_FILTER_NEAREST);

    VkImageMemoryBarrier2 blit_to_present_image_barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
        .dstAccessMask = VK_ACCESS_2_NONE,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_rdr_swapchain_images[image_index].vk_image(),
        .subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        },
    };
    VkDependencyInfo blit_to_present_barrier_dependency_info{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &blit_to_present_image_barrier,
    };
    vkCmdPipelineBarrier2(cmd.vk_command_buffer(), &blit_to_present_barrier_dependency_info);

    Vk_Check(vkEndCommandBuffer(cmd.vk_command_buffer()));

    VkPipelineStageFlags stage_flags = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    m_rdr_queue.submit(std::span(&m_wait_image_acquired_semaphores[m_frame_index], 1),
                       std::span(&stage_flags, 1),
                       std::span(&cmd, 1),
                       std::span(&m_wait_renderer_complete_semaphores[image_index], 1),
                       m_next_frame_fences[m_frame_index]);
    
    m_frame_index = (m_frame_index + 1) % Frames_In_Flight;

    VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_wait_renderer_complete_semaphores[image_index].vk_semaphore(),
        .swapchainCount = 1,
        .pSwapchains = &m_rdr_swapchain.vk_swapchain(),
        .pImageIndices = &image_index,
    };
    Vk_Check(vkQueuePresentKHR(m_rdr_queue.vk_queue(), &present_info));
}

bool App::init()
{
    std::println("initializing sdl...");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println("Failed to initialize sdl: {}", SDL_GetError());
        return false;
    }

    if (!rdr::Device::create(m_rdr_device)) {
        return false;
    }
    m_rdr_queue = m_rdr_device.get_device_queue(0);

    if (!rdr::Allocator::create(m_rdr_device, m_rdr_allocator)) {
        return false;
    }

    if (!rdr::Surface::create_window_and_surface(m_rdr_device, "Memes... the DNA of the soul", 1920, 1080, m_rdr_surface)) {
        return false;
    }
    m_window = m_rdr_surface.window();

    if (!rdr::Swapchain::create(m_rdr_device, m_rdr_surface, Frames_In_Flight, m_rdr_swapchain)) {
        return false;
    }

    m_rdr_swapchain_images = m_rdr_swapchain.get_swapchain_images_khr();
    if (m_rdr_swapchain_images.size() < Frames_In_Flight) {
        std::println("Got unexpected number of swapchain images: {} (Frames_In_Flight == {})", m_rdr_swapchain_images.size(), Frames_In_Flight);
        return false;
    }
    m_wait_renderer_complete_semaphores.resize(m_rdr_swapchain_images.size());
    for (auto& semaphore : m_wait_renderer_complete_semaphores) {
        if (!rdr::Semaphore::create(m_rdr_device, semaphore)) {
            return false;
        }
    }

    rdr::Shader_Compiler shader_compiler;
    if (!rdr::Shader_Compiler::create(shader_compiler)) {
        return false;
    }
    if (!shader_compiler.compile_from_source_file(m_rdr_device, "assets/shaders/compute/terrain_gen.hlsl", rdr::Shader_Type::Compute, m_terrain_gen_shader)) {
        return false;
    }

    if (!rdr::Descriptor_Set_Layout::create_from_shader(m_rdr_device, m_terrain_gen_shader, 0, 0, m_terraing_gen_shader_descriptor_set_layout)) {
        return false;
    }

    if (!rdr::Buffer::create(m_rdr_allocator, Terrain_Size * Terrain_Size * sizeof(float), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, m_terrain_heght_map_buffer)) {
        return false;
    }

    if (!rdr::Image::create(m_rdr_allocator, Terrain_Size, Terrain_Size, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0, m_terrain_height_map_image, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT)) {
        return false;
    }

    if (!rdr::Image_View::create(m_rdr_device, m_terrain_height_map_image, 0, VK_FORMAT_R8G8B8A8_UNORM, m_terraing_height_map_image_view)) {
        return false;
    }

    auto terrain_gen_shader_push_constants = m_terrain_gen_shader.get_push_constants();
    if (!rdr::Pipeline_Layout::create(m_rdr_device, std::span(&m_terraing_gen_shader_descriptor_set_layout, 1), terrain_gen_shader_push_constants, VK_SHADER_STAGE_COMPUTE_BIT, m_compute_pipeline_layouts[0])) {
        return false;
    }

    if (!rdr::Pipeline::create_compute(m_rdr_device, std::span(&m_terrain_gen_shader, 1), m_compute_pipeline_layouts, m_compute_pipelines)) {
        return false;
    }

    if (!rdr::Descriptor_Pool::create(m_rdr_device, std::span(&m_terraing_gen_shader_descriptor_set_layout, 1), 1, m_descriptor_pool)) {
        return false;
    }

    if (!m_descriptor_pool.allocate_descriptor_sets(1, m_terraing_gen_shader_descriptor_set_layout, std::span(&m_terrain_gen_descriptor_set, 1))) {
        return false;
    }
    
    VkDescriptorBufferInfo terrain_buffer_info{
        .buffer = m_terrain_heght_map_buffer.vk_buffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    VkDescriptorImageInfo terrain_image_info{
        .imageView = m_terraing_height_map_image_view.vk_image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    std::array<VkWriteDescriptorSet, 2> write_descriptor_set = {
        m_terrain_gen_descriptor_set.write_storage_buffer(0, std::span(&terrain_buffer_info, 1)),
        m_terrain_gen_descriptor_set.write_storage_image(1, std::span(&terrain_image_info, 1)),
    };
    rdr::Descriptor_Set::update_descriptor_sets(m_rdr_device, write_descriptor_set, std::span<VkCopyDescriptorSet>());

    for (auto& shader_buffer : m_terrain_gen_shader_data_buffers) {
        if (!rdr::Buffer::create(m_rdr_allocator, sizeof(Terrain_Gen_Shader_Data),
                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                 shader_buffer)) {
            return false;
        }
    }

    for (auto& fence : m_next_frame_fences) {
        if (!rdr::Fence::create(m_rdr_device, true, fence)) {
            return false;
        }
    }

    for (auto& semaphore : m_wait_image_acquired_semaphores) {
        if (!rdr::Semaphore::create(m_rdr_device, semaphore)) {
            return false;
        }
    }

    if (!rdr::Command_Pool::create(m_rdr_device, m_rdr_device.vk_queue_family_index(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_command_pool)) {
        return false;
    }
    if (!m_command_pool.allocate_command_buffers(m_command_buffers)) {
        return false;
    }

    return true;
}

void App::main_loop()
{
    uint64_t ticks_last_frame = SDL_GetTicks();

    bool running = true;
    while (running) {
        process_events(running);
        
        uint64_t ticks_this_frame = SDL_GetTicks();
        float dt = static_cast<float>(ticks_this_frame - ticks_last_frame) / 1000.f;
        ticks_last_frame = ticks_this_frame;
        
        update(dt);
        render(dt);
    }
}
