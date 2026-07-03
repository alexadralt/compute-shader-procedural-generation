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

    auto device_create_result = rdr::Device::create();
    if (!device_create_result.has_value()) {
        return false;
    }
    m_rdr_device = std::move(device_create_result.value());

    auto allocator_create_result = rdr::Allocator::create(m_rdr_device);
    if (!allocator_create_result.has_value()) {
        return false;
    }
    m_rdr_allocator = std::move(allocator_create_result.value());

    auto surface_create_result = rdr::Surface::create_window_and_surface(m_rdr_device, "Memes... the DNA of the soul", 1920, 1080);
    if (!surface_create_result.has_value()) {
        return false;
    }
    m_rdr_surface = std::move(surface_create_result.value());
    m_window = m_rdr_surface.window();

    auto swapchain_create_result = rdr::Swapchain::create(m_rdr_device, m_rdr_surface);
    if (!swapchain_create_result.has_value()) {
        return false;
    }
    m_rdr_swapchain = std::move(swapchain_create_result.value());

    auto shader_compiler_create_result = rdr::Shader_Compiler::create();
    if (!shader_compiler_create_result.has_value()) {
        return false;
    }
    rdr::Shader_Compiler& shader_compiler = shader_compiler_create_result.value();
    auto shader_create_result = shader_compiler.compile_from_source_file(m_rdr_device, "assets/shaders/compute/terrain_gen.hlsl", rdr::Shader_Type::Compute);
    if (!shader_create_result.has_value()) {
        return false;
    }
    m_terrain_gen_shader = std::move(shader_create_result.value());

    auto descriptor_set_create_result = rdr::Descriptor_Set_Layout::create_from_shader(m_rdr_device, m_terrain_gen_shader, 0);
    if (!descriptor_set_create_result.has_value()) {
        return false;
    }
    m_terraing_gen_shader_descriptor_set = std::move(descriptor_set_create_result.value());

    auto buffer_create_result = rdr::Buffer::create(m_rdr_allocator, Terrain_Size * Terrain_Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    if (!buffer_create_result.has_value()) {
        return false;
    }
    m_terrain_heght_map_buffer = std::move(buffer_create_result.value());

    auto image_create_result = rdr::Image::create(m_rdr_allocator, Terrain_Size, Terrain_Size, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    if (!image_create_result.has_value()) {
        return false;
    }
    m_terrain_height_map_image = std::move(image_create_result.value());

    auto terrain_gen_shader_push_constants = m_terrain_gen_shader.get_push_constants();
    auto pipeline_layout_create_result = rdr::Pipeline_Layout::create(m_rdr_device, std::span(&m_terraing_gen_shader_descriptor_set, 1), std::span(terrain_gen_shader_push_constants), VK_SHADER_STAGE_COMPUTE_BIT);
    if (!pipeline_layout_create_result.has_value()) {
        return false;
    }
    m_compute_pipeline_layouts[0] = std::move(pipeline_layout_create_result.value());

    std::vector<rdr::Pipeline> compute_pipelines = rdr::Pipeline::create_compute(m_rdr_device, std::span(&m_terrain_gen_shader, 1), std::span(&m_compute_pipeline_layouts[0], Compute_Pipeline_Count));
    if (compute_pipelines.size() < 1) {
        return false;
    }
    for (size_t i = 0; i < Compute_Pipeline_Count; ++i) {
        m_compute_pipelines[i] = std::move(compute_pipelines[i]);
    }

    return true;
}

void App::main_loop()
{
    bool running = true;
    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                {
                    std::println("quitting application...");
                    running = false;
                } break;
            }
        }
    }
}
