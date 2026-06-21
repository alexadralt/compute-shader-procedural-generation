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

    auto device = rdr::Device::create();
    if (!device.has_value()) {
        return false;
    }
    m_rdr_device = std::move(device.value());

    auto allocator = rdr::Allocator::create(m_rdr_device);
    if (!allocator.has_value()) {
        return false;
    }
    m_rdr_allocator = std::move(allocator.value());

    auto surface = rdr::Surface::create_window_and_surface(m_rdr_device, "Memes... the DNA of the soul", 1920, 1080);
    if (!surface.has_value()) {
        return false;
    }
    m_rdr_surface = std::move(surface.value());
    m_window = m_rdr_surface.window();

    auto swapchain = rdr::Swapchain::create(m_rdr_device, m_rdr_surface);
    if (!swapchain.has_value()) {
        return false;
    }
    m_rdr_swapchain = std::move(swapchain.value());

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
