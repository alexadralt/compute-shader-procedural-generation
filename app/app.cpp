#include "app.h"

#include <renderer/shader_compiler.h>

#include <print>

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

    if (!rdr::Buffer::create(m_rdr_allocator, Terrain_Size * Terrain_Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, m_terrain_heght_map_buffer)) {
        return false;
    }

    if (!rdr::Image::create(m_rdr_allocator, Terrain_Size, Terrain_Size, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0, m_terrain_height_map_image)) {
        return false;
    }

    if (!rdr::Image_View::create(m_rdr_device, m_terrain_height_map_image, 0, VK_FORMAT_B8G8R8A8_UNORM, m_terraing_height_map_image_view)) {
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
