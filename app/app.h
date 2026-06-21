#pragma once

#include <renderer/device.h>
#include <renderer/allocator.h>
#include <renderer/surface.h>
#include <renderer/swapchain.h>

#include <SDL3/SDL.h>

class App {
    SDL_Window* m_window;
    rdr::Device m_rdr_device; // note: it is important that m_rdr_xxx fields are declared in reverse order than they are initialized so that desctuctors are called in correct order (I love C++)
    rdr::Allocator m_rdr_allocator; 
    rdr::Surface m_rdr_surface;
    rdr::Swapchain m_rdr_swapchain;

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
