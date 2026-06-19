#pragma once

#include <renderer/device.h>
#include <renderer/allocator.h>
#include <renderer/surface.h>

#include <SDL3/SDL.h>

class App {
    SDL_Window* m_window;
    rdr::Device m_rdr_device;
    rdr::Allocator m_rdr_allocator; // note: it is important that m_rdr_allocator is declared after m_rdr_device so that desctuctors are called in correct order (I love C++)
    rdr::Surface m_rdr_surface;

    void quit();

public:
    App() : m_window(nullptr),
            m_rdr_device(),
            m_rdr_allocator(),
            m_rdr_surface() {};
    ~App() { quit(); };

    App(const App& other) = delete;
    App(App&& other) = delete;

    App& operator=(const App& other) = delete;
    App& operator=(App&& other) = delete;

    bool init();
    void main_loop();
};
