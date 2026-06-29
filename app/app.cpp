#include "app.h"

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

    auto surface_caps = m_rdr_surface.get_surface_caps_khr();
    if (!surface_caps.has_value()) {
        return false;
    }
    VkExtent2D surface_extent = surface_caps.value().currentExtent;
    auto depth_attachment = rdr::Image::create_depth_attachmnent(m_rdr_device, m_rdr_allocator, surface_extent.width, surface_extent.height);
    if (!depth_attachment.has_value()) {
        return false;
    }
    m_depth_attachment = std::move(depth_attachment.value());

    auto shader_create_result = rdr::Shader::create_from_source_file(m_rdr_device, "assets/shaders/compute/terrain_gen.hlsl", rdr::Shader_Type::Compute);
    if (!shader_create_result.has_value()) {
        return false;
    }
    m_terrain_gen_shader = std::move(shader_create_result.value());

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
                    running = false;
                } break;
            }
        }
    }
}
