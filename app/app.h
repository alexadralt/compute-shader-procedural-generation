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
#include <glm/glm.hpp>

#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <span>

class App {
    static constexpr size_t   Frames_In_Flight = 3;
    static constexpr uint32_t Terrain_Size = 1024;
    static constexpr size_t   Chunks_Count_X = 24;
    
    struct Terrain_Gen_Shader_Data {
        uint32_t terrain_size = Terrain_Size;
        float amplitude = 1;
        uint32_t octave_count = 1;
        uint32_t chunk_count_x = Chunks_Count_X;
        uint64_t seed = 0xFAFAFAFAFAFAFAFA;
    };

    struct Terrain_Normals_Shader_Data {
        uint32_t terrain_size;
        uint32_t render_distance;
    };

    struct Raymarch_Camera_Info {
        alignas(16) glm::vec3 position;
        alignas(16) glm::mat3x4 world_from_camera; // 3x4 beacuse in std140 layout each float3 in a 3x3 matrix is padded to have 16-byte allignemt
    };

    enum class Raymarch_Render_Type {
        Regular,
        Normals,
    };

    struct Terrain_Raymarch_Shader_Data {
        Raymarch_Camera_Info camera_info;
        alignas(8) glm::uvec2 out_image_size;
        uint32_t terrain_size;
        uint32_t chunk_count_x;
        Raymarch_Render_Type render_type;
    };

    struct Camera_Info {
        glm::vec3 position = glm::vec3(512, -250, 512);
        glm::vec2 camera_angles_xy;
    };

    enum Shaders : size_t {
        Shaders_Terrain_Gen = 0,
        Shaders_Terrain_Normals,
        Shaders_Terrain_Raymarch,
        
        Shaders_Count
    };

    class Text_File {
        std::string_view m_contents;

    public:
        Text_File() {};
        ~Text_File();
        
        static bool open(const std::string& path, Text_File& out_text_file);

        Text_File(const Text_File& other);
        Text_File& operator=(const Text_File& other);

        Text_File(Text_File&& other) noexcept;
        Text_File& operator=(Text_File&& other) noexcept;

        std::string_view contents() const { return m_contents; }
    };
    
    SDL_Window* m_window;
    rdr::Device m_rdr_device; // note: it is important that m_rdr_xxx fields are declared in order they are initialized so that desctuctors are called in correct order (I love C++)
    rdr::Queue m_rdr_queue;
    rdr::Allocator m_rdr_allocator;
    rdr::Surface m_rdr_surface;
    rdr::Swapchain m_rdr_swapchain;
    std::vector<rdr::Image> m_rdr_swapchain_images;

    rdr::Image m_output_image;
    rdr::Image_View m_output_image_view;

    std::array<rdr::Buffer, Chunks_Count_X * Chunks_Count_X> m_terrain_heght_map_buffers;
    std::array<rdr::Buffer, Chunks_Count_X * Chunks_Count_X> m_terrain_normals_buffers;
    
    std::array<rdr::Shader, Shaders_Count> m_shaders;
    
    std::array<rdr::Descriptor_Set_Layout, Shaders_Count> m_descriptor_set_layouts; // set 0
    rdr::Descriptor_Pool m_descriptor_pool;
    rdr::Descriptor_Set m_terrain_gen_descriptor_set;
    rdr::Descriptor_Set m_terrain_normals_descriptor_set;
    rdr::Descriptor_Set m_terrain_raymarch_descriptor_set;
    
    std::array<rdr::Pipeline_Layout, Shaders_Count> m_compute_pipeline_layouts;
    std::array<rdr::Pipeline, Shaders_Count> m_compute_pipelines;
    
    rdr::Command_Pool m_command_pool;
    std::array<rdr::Command_Buffer, Frames_In_Flight> m_command_buffers;
    std::array<rdr::Fence, Frames_In_Flight> m_next_frame_fences;
    std::array<rdr::Semaphore, Frames_In_Flight> m_wait_image_acquired_semaphores;
    std::vector<rdr::Semaphore> m_wait_renderer_complete_semaphores;

    uint32_t m_frame_index = 0;
    bool m_should_recreate_swapchain = false;
    glm::ivec2 m_current_window_size = { 0, 0 };
    
    Terrain_Gen_Shader_Data m_terrain_gen_shader_data;
    Terrain_Raymarch_Shader_Data m_terrain_raymarch_info;
    std::vector<float> m_terrain_gen_octave_weights;
    std::vector<float> m_terrain_gen_octave_frequencies;
    std::array<rdr::Buffer, Frames_In_Flight> m_terrain_gen_shader_data_buffers;
    std::array<rdr::Buffer, Frames_In_Flight> m_terrain_gen_shader_octave_weights;
    std::array<rdr::Buffer, Frames_In_Flight> m_terrain_gen_shader_octave_frequencies;
    std::array<rdr::Buffer, Frames_In_Flight> m_terrain_raymarch_info_buffers;

    Camera_Info m_camera_info;
    Raymarch_Render_Type m_raymarch_render_type = Raymarch_Render_Type::Regular;

    std::array<bool, SDL_SCANCODE_COUNT> m_keyboard_state;          // an array of keys that are being pressed this frame
    std::array<bool, SDL_SCANCODE_COUNT> m_keys_pressed_this_frame; // an array of keys that have received key down event this frame

    glm::vec2 m_restore_mouse_pos = { 0, 0 };
    bool m_is_fullsreen = false;

    bool m_should_regenerate_terrain = true;

    void quit();

    void process_events(bool& running);
    void update(float dt);
    void render(float dt);
    
    void maybe_update_swapchain();
    void check_if_should_update_swapchain(VkResult result);
    bool load_shaders(bool create_descriptor_set_layouts);

    static std::vector<std::string_view> lex_config_file(std::string_view text);
    bool load_terrain_shader_data_from_file();

    glm::mat4x4 camera_from_world() const;
    glm::mat4x4 world_from_camera() const;
public:
    App() : m_window(nullptr),
            m_terrain_gen_shader_data(),
            m_terrain_raymarch_info(),
            m_keyboard_state(),
            m_keys_pressed_this_frame(),
            m_camera_info() {};
    ~App() { quit(); };

    App(const App& other) = delete;
    App(App&& other) = delete;

    App& operator=(const App& other) = delete;
    App& operator=(App&& other) = delete;

    bool init();
    void main_loop();
};
