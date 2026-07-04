#pragma once

#include <renderer/device.h>
#include <renderer/allocator.h>
#include <renderer/surface.h>
#include <renderer/swapchain.h>
#include <renderer/image.h>
#include <renderer/shader.h>
#include <renderer/buffer.h>
#include <renderer/descriptor_set_layout.h>
#include <renderer/pipeline_layout.h>
#include <renderer/pipeline.h>
#include <renderer/descriptor_pool.h>
#include <renderer/descriptor_set.h>
#include <renderer/image_view.h>

#include <SDL3/SDL.h>

#include <array>

class App {
    SDL_Window* m_window;
    rdr::Device m_rdr_device; // note: it is important that m_rdr_xxx fields are declared in order they are initialized so that desctuctors are called in correct order (I love C++)
    rdr::Allocator m_rdr_allocator;
    rdr::Surface m_rdr_surface;
    rdr::Swapchain m_rdr_swapchain;
    
    static constexpr size_t Compute_Pipeline_Count = 1;

    rdr::Shader m_terrain_gen_shader;
    rdr::Descriptor_Set_Layout m_terraing_gen_shader_descriptor_set_layout;
    rdr::Buffer m_terrain_heght_map_buffer;
    rdr::Image m_terrain_height_map_image;
    rdr::Image_View m_terraing_height_map_image_view;
    std::array<rdr::Pipeline_Layout, Compute_Pipeline_Count> m_compute_pipeline_layouts;
    std::array<rdr::Pipeline, Compute_Pipeline_Count> m_compute_pipelines;
    rdr::Descriptor_Pool m_descriptor_pool;
    rdr::Descriptor_Set m_terrain_gen_descriptor_set;

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
    void process_events(bool& running);
};
