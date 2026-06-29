#pragma once

#include <renderer/device.h>
#include <renderer/allocator.h>
#include <renderer/surface.h>
#include <renderer/swapchain.h>
#include <renderer/image.h>
#include <renderer/shader.h>
#include <renderer/buffer.h>

#include <SDL3/SDL.h>

class App {
    SDL_Window* m_window;
    rdr::Device m_rdr_device; // note: it is important that m_rdr_xxx fields are declared in reverse order than they are initialized so that desctuctors are called in correct order (I love C++)
    rdr::Allocator m_rdr_allocator;
    rdr::Surface m_rdr_surface;
    rdr::Swapchain m_rdr_swapchain;
    
    rdr::Shader m_terrain_gen_shader;
    rdr::Buffer m_terrain_heght_map_buffer;
    rdr::Image m_terrain_height_map_image;

    void quit();

    static constexpr Uint32 Terrain_Size = 1000;

public:
    App() : m_window(nullptr) {};
    ~App() { quit(); };

    App(const App& other) = delete;
    App(App&& other) = delete;

    App& operator=(const App& other) = delete;
    App& operator=(App&& other) = delete;

    bool init();
    void main_loop();
};
