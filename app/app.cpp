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

    std::println("creating window...");
    m_window = SDL_CreateWindow("I have a dream!", 1920, 1080, SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        std::println("Failed to create window: {}", SDL_GetError());
        return false;
    }

    auto [device, ok] = rdr::Device::create();
    if (!ok) {
        return false;
    }
    m_rdr_device = std::move(device);

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
