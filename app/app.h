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
#include <renderer/fence.h>
#include <renderer/semaphore.h>
#include <renderer/command_pool.h>
#include <renderer/command_buffer.h>
#include <renderer/queue.h>

#include <SDL3/SDL.h>

#include <array>
#include <vector>

class App {
    struct Terrain_Gen_Shader_Data {
        uint32_t terrain_size;
        float frequency;
        float amplitude;
    };

    enum class Compute_Pipelines {
        Terrain_Gen = 0,
        
        Compute_Pipeline_Count
    };

    static constexpr size_t Frames_In_Flight = 2;
    static constexpr Uint32 Terrain_Size = 1000;
    
    SDL_Window* m_window;
    rdr::Device m_rdr_device; // note: it is important that m_rdr_xxx fields are declared in order they are initialized so that desctuctors are called in correct order (I love C++)
    rdr::Queue m_rdr_queue;
    rdr::Allocator m_rdr_allocator;
    rdr::Surface m_rdr_surface;
    rdr::Swapchain m_rdr_swapchain;
    std::vector<rdr::Image> m_rdr_swapchain_images;

    rdr::Shader m_terrain_gen_shader;
    rdr::Descriptor_Set_Layout m_terraing_gen_shader_descriptor_set_layout;
    rdr::Buffer m_terrain_heght_map_buffer;
    rdr::Image m_terrain_height_map_image;
    rdr::Image_View m_terraing_height_map_image_view;
    std::array<rdr::Pipeline_Layout, static_cast<size_t>(Compute_Pipelines::Compute_Pipeline_Count)> m_compute_pipeline_layouts;
    std::array<rdr::Pipeline, static_cast<size_t>(Compute_Pipelines::Compute_Pipeline_Count)> m_compute_pipelines;
    rdr::Descriptor_Pool m_descriptor_pool;
    rdr::Descriptor_Set m_terrain_gen_descriptor_set;
    std::array<rdr::Buffer, Frames_In_Flight> m_terrain_gen_shader_data_buffers;
    std::array<rdr::Fence, Frames_In_Flight> m_next_frame_fences;
    std::array<rdr::Semaphore, Frames_In_Flight> m_wait_image_acquired_semaphores;
    std::vector<rdr::Semaphore> m_wait_renderer_complete_semaphores;
    rdr::Command_Pool m_command_pool;
    std::array<rdr::Command_Buffer, Frames_In_Flight> m_command_buffers;

    void quit();

    void process_events(bool& running);
    void update(float dt);
    void render(float dt);
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
