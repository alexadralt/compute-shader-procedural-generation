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

    if (!rdr::Allocator::create(m_rdr_device, m_rdr_allocator)) {
        return false;
    }

    if (!rdr::Surface::create_window_and_surface(m_rdr_device, "Memes... the DNA of the soul", 1920, 1080, m_rdr_surface)) {
        return false;
    }
    m_window = m_rdr_surface.window();

    if (!rdr::Swapchain::create(m_rdr_device, m_rdr_surface, m_rdr_swapchain)) {
        return false;
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

    return true;
}

void App::main_loop()
{
    bool running = true;
    while (running) {
        process_events(running);
    }
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
