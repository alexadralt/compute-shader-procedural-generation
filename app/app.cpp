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
    if (!device_create_result.second) {
        return false;
    }
    m_rdr_device = std::move(device_create_result.first);

    auto allocator_create_result = rdr::Allocator::create(m_rdr_device);
    if (!allocator_create_result.second) {
        return false;
    }
    m_rdr_allocator = std::move(allocator_create_result.first);

    auto surface_create_result = rdr::Surface::create_window_and_surface(m_rdr_device, "Memes... the DNA of the soul", 1920, 1080);
    if (!surface_create_result.second) {
        return false;
    }
    m_rdr_surface = std::move(surface_create_result.first);
    m_window = m_rdr_surface.window();

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
