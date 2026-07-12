#include "app.h"

#include <renderer/shader_compiler.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#include <print>
#include <cassert>
#include <utility>
#include <ranges>
#include <charconv>
#include <type_traits>

#define Vk_Check(call) \
    if ((call) != VK_SUCCESS) \
        assert(false)
#define Check(call) \
    if (!(call)) \
        assert(false)

void App::quit()
{
    SDL_Quit();
}

void App::process_events(bool& running)
{
    memset(m_keys_pressed_this_frame.data(), 0, m_keys_pressed_this_frame.size()); // reset m_keys_pressed_this_frame array

    for (SDL_Event event; SDL_PollEvent(&event);) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
            {
                std::println("quitting application...");
                Vk_Check(vkDeviceWaitIdle(m_rdr_device.vk_device()));
                running = false;
            } break;
            case SDL_EVENT_WINDOW_RESIZED:
            {
                m_should_recreate_swapchain = true;
            } break;
            case SDL_EVENT_KEY_DOWN:
            {
                if (!m_keyboard_state[event.key.scancode]) {
                    m_keys_pressed_this_frame[event.key.scancode] = true;
                }
                m_keyboard_state[event.key.scancode] = true; // We don't use SDL_GetKeyboardState() here because this value would be true before entering this case block (which is not what we want)
            } break;
            case SDL_EVENT_KEY_UP:
            {
                m_keyboard_state[event.key.scancode] = false;
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    SDL_GetMouseState(&m_restore_mouse_pos.x, &m_restore_mouse_pos.y);
                    
                    glm::ivec2 window_dims;
                    Check(SDL_GetWindowSizeInPixels(m_window, &window_dims.x, &window_dims.y));

                    Check(SDL_HideCursor());
                    glm::vec2 window_center = glm::vec2(window_dims) / 2.f;
                    SDL_WarpMouseInWindow(m_window, window_center.x, window_center.y);
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    SDL_WarpMouseInWindow(m_window, m_restore_mouse_pos.x, m_restore_mouse_pos.y);
                    Check(SDL_ShowCursor());
                }
            } break;
        }
    }
}

void App::update(float dt)
{
    if (m_keys_pressed_this_frame[SDL_SCANCODE_R]) {
        load_terrain_shader_data_from_file();
    }

    if (m_keys_pressed_this_frame[SDL_SCANCODE_F10]) {
        m_is_fullsreen = !m_is_fullsreen;
        Check(SDL_SetWindowFullscreen(m_window, m_is_fullsreen));
    }

    glm::mat3x3 w_from_c = world_from_camera();
    constexpr float move_speed = 2.3f;

    if (m_keyboard_state[SDL_SCANCODE_W]) {
        m_camera_info.position += w_from_c * glm::vec3(0, 0, 1) * move_speed;
    }

    if (m_keyboard_state[SDL_SCANCODE_A]) {
        m_camera_info.position += w_from_c * glm::vec3(-1, 0, 0) * move_speed;
    }

    if (m_keyboard_state[SDL_SCANCODE_S]) {
        m_camera_info.position += w_from_c * glm::vec3(0, 0, -1) * move_speed;
    }

    if (m_keyboard_state[SDL_SCANCODE_D]) {
        m_camera_info.position += w_from_c * glm::vec3(1, 0, 0) * move_speed;
    }

    if (m_keyboard_state[SDL_SCANCODE_SPACE]) {
        m_camera_info.position += glm::vec3(0, -1, 0) * move_speed;
    }

    if (m_keyboard_state[SDL_SCANCODE_C]) {
        m_camera_info.position += glm::vec3(0, 1, 0) * move_speed;
    }

    glm::vec2 mouse_pos;
    SDL_MouseButtonFlags mouse_flags = SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);
    if (mouse_flags & SDL_BUTTON_LMASK) {
        glm::ivec2 window_dims;
        Check(SDL_GetWindowSizeInPixels(m_window, &window_dims.x, &window_dims.y));
        glm::vec2 mouse_delta = mouse_pos - glm::vec2(window_dims) / 2.f;

        glm::vec2 window_center = glm::vec2(window_dims) / 2.f;
        SDL_WarpMouseInWindow(m_window, window_center.x, window_center.y);

        constexpr float Sensitivity = 0.005f;
        mouse_delta *= Sensitivity;

        m_camera_info.camera_angles_xy.x = m_camera_info.camera_angles_xy.x - mouse_delta.x;
        m_camera_info.camera_angles_xy.y = glm::clamp(m_camera_info.camera_angles_xy.y + mouse_delta.y, -glm::half_pi<float>(), glm::half_pi<float>());
    }
}

void App::render(float dt)
{
    maybe_update_swapchain();

    Check(m_next_frame_fences[m_frame_index].wait());
    Check(m_next_frame_fences[m_frame_index].reset());

    uint32_t image_index;
    check_if_should_update_swapchain(m_rdr_swapchain.acquire_next_image(m_wait_image_acquired_semaphores[m_frame_index], image_index));

    memcpy(m_terrain_gen_shader_data_buffers[m_frame_index].mapped_data(), &m_terrain_gen_shader_data, sizeof(Terrain_Gen_Shader_Data));

    if (m_terrain_gen_shader_octave_weights[m_frame_index].buffer_size() != m_terrain_gen_octave_weights.size() * sizeof(float)) {
        Check(rdr::Buffer::create(m_rdr_allocator, sizeof(float) * m_terrain_gen_octave_weights.size(),
                                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                  m_terrain_gen_shader_octave_weights[m_frame_index]));
    }
    memcpy(m_terrain_gen_shader_octave_weights[m_frame_index].mapped_data(), m_terrain_gen_octave_weights.data(), m_terrain_gen_octave_weights.size() * sizeof(float));

    Terrain_Raymarch_Shader_Data raymarch_info{
        .camera_info = {
            .position = m_camera_info.position,
            .world_from_camera = glm::transpose(world_from_camera()),
        },
        .out_image_size = { m_output_image.image_size()[0], m_output_image.image_size()[1] },
        .terrain_size = Terrain_Size,
    };
    memcpy(m_terrain_raymarch_info_buffers[m_frame_index].mapped_data(), &raymarch_info, sizeof(Terrain_Raymarch_Shader_Data));

    auto& cmd = m_command_buffers[m_frame_index];
    Vk_Check(vkResetCommandBuffer(cmd.vk_command_buffer(), 0));

    VkCommandBufferBeginInfo cmd_begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    Vk_Check(vkBeginCommandBuffer(cmd.vk_command_buffer(), &cmd_begin_info));

    // terrain generation pass
    
    std::array<VkBufferMemoryBarrier2, 1> terrain_gen_buffer_barriers{
        VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = m_terrain_heght_map_buffer.vk_buffer(),
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
    };
    cmd.pipeline_barrier(std::span<VkImageMemoryBarrier2>(), terrain_gen_buffer_barriers);

    vkCmdBindPipeline(cmd.vk_command_buffer(), VK_PIPELINE_BIND_POINT_COMPUTE, m_compute_pipelines[Shaders_Terrain_Gen].vk_pipeline());
    
    vkCmdBindDescriptorSets(cmd.vk_command_buffer(), VK_PIPELINE_BIND_POINT_COMPUTE,
                            m_compute_pipeline_layouts[Shaders_Terrain_Gen].vk_pipeline_layout(),
                            0, 1, &m_terrain_gen_descriptor_set.vk_descriptor_set(), 0, nullptr);

    std::array<uint64_t, 2> push_constants = {
        static_cast<uint64_t>(m_terrain_gen_shader_data_buffers[m_frame_index].vk_device_address()),
        static_cast<uint64_t>(m_terrain_gen_shader_octave_weights[m_frame_index].vk_device_address()),
    };
    vkCmdPushConstants(cmd.vk_command_buffer(), m_compute_pipeline_layouts[Shaders_Terrain_Gen].vk_pipeline_layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                      static_cast<uint32_t>(sizeof(uint64_t) * push_constants.size()), push_constants.data());

    vkCmdDispatch(cmd.vk_command_buffer(), Terrain_Size / 8, Terrain_Size / 8, 1);

    // terrain normals pass

    std::array<VkBufferMemoryBarrier2, 2> terrain_normals_buffer_barriers{
        VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = m_terrain_normals_buffer.vk_buffer(),
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
        VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = m_terrain_heght_map_buffer.vk_buffer(),
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
    };
    cmd.pipeline_barrier(std::span<VkImageMemoryBarrier2>(), terrain_normals_buffer_barriers);

    vkCmdBindPipeline(cmd.vk_command_buffer(), VK_PIPELINE_BIND_POINT_COMPUTE, m_compute_pipelines[Shaders_Terrain_Normals].vk_pipeline());

    vkCmdBindDescriptorSets(cmd.vk_command_buffer(), VK_PIPELINE_BIND_POINT_COMPUTE,
        m_compute_pipeline_layouts[Shaders_Terrain_Normals].vk_pipeline_layout(),
        0, 1, &m_terrain_normals_descriptor_set.vk_descriptor_set(), 0, nullptr);

    vkCmdPushConstants(cmd.vk_command_buffer(), m_compute_pipeline_layouts[Shaders_Terrain_Normals].vk_pipeline_layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &Terrain_Size);

    vkCmdDispatch(cmd.vk_command_buffer(), Terrain_Size / 8, Terrain_Size / 8, 1);

    // render with raymarching

    std::array<VkImageMemoryBarrier2, 1> raymarch_image_barriers{
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_output_image.vk_image(),
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            },
        },
    };
    std::array<VkBufferMemoryBarrier2, 2> raymarch_buffer_barriers{
        VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = m_terrain_normals_buffer.vk_buffer(),
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
        VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = m_terrain_heght_map_buffer.vk_buffer(),
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
    };
    cmd.pipeline_barrier(raymarch_image_barriers, raymarch_buffer_barriers);

    vkCmdBindPipeline(cmd.vk_command_buffer(), VK_PIPELINE_BIND_POINT_COMPUTE, m_compute_pipelines[Shaders_Terrain_Raymarch].vk_pipeline());

    vkCmdBindDescriptorSets(cmd.vk_command_buffer(), VK_PIPELINE_BIND_POINT_COMPUTE,
        m_compute_pipeline_layouts[Shaders_Terrain_Raymarch].vk_pipeline_layout(),
        0, 1, &m_terrain_raymarch_descriptor_set.vk_descriptor_set(), 0, nullptr);

    vkCmdPushConstants(cmd.vk_command_buffer(), m_compute_pipeline_layouts[Shaders_Terrain_Raymarch].vk_pipeline_layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(VkDeviceAddress), &m_terrain_raymarch_info_buffers[m_frame_index].vk_device_address());

    vkCmdDispatch(cmd.vk_command_buffer(), m_output_image.image_size()[0] / 8, m_output_image.image_size()[1] / 8, 1);

    // blit

    std::array<VkImageMemoryBarrier2, 2> blit_image_barriers{
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_rdr_swapchain_images[image_index].vk_image(),
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            },
        },
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_output_image.vk_image(),
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            },
        },
    };
    cmd.pipeline_barrier(blit_image_barriers, std::span<VkBufferMemoryBarrier2>());

    VkImageBlit2 blit_region{
        .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
        .srcSubresource = VkImageSubresourceLayers{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .srcOffsets = {
            VkOffset3D{ 0, 0, 0 },
            VkOffset3D{ static_cast<int32_t>(m_output_image.image_size()[0]), static_cast<int32_t>(m_output_image.image_size()[1]), 1 }
        },
        .dstSubresource = VkImageSubresourceLayers{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .dstOffsets = {
            VkOffset3D{ 0, 0, 0 },
            VkOffset3D{ m_current_window_size.x, m_current_window_size.y, 1 }
        },
    };
    VkBlitImageInfo2 blit_info{
        .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .srcImage = m_output_image.vk_image(),
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage = m_rdr_swapchain_images[image_index].vk_image(),
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = 1,
        .pRegions = &blit_region,
        .filter = VK_FILTER_LINEAR,
    };
    vkCmdBlitImage2(cmd.vk_command_buffer(), &blit_info);

    // present

    std::array<VkImageMemoryBarrier2, 1> blit_to_present_image_barriers{
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
            .dstAccessMask = VK_ACCESS_2_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_rdr_swapchain_images[image_index].vk_image(),
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            },
        }
    };
    cmd.pipeline_barrier(blit_to_present_image_barriers, std::span<VkBufferMemoryBarrier2>());

    Vk_Check(vkEndCommandBuffer(cmd.vk_command_buffer()));

    VkPipelineStageFlags stage_flags = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    m_rdr_queue.submit(std::span(&m_wait_image_acquired_semaphores[m_frame_index], 1),
                       std::span(&stage_flags, 1),
                       std::span(&cmd, 1),
                       std::span(&m_wait_renderer_complete_semaphores[image_index], 1),
                       m_next_frame_fences[m_frame_index]);
    
    m_frame_index = (m_frame_index + 1) % Frames_In_Flight;

    VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_wait_renderer_complete_semaphores[image_index].vk_semaphore(),
        .swapchainCount = 1,
        .pSwapchains = &m_rdr_swapchain.vk_swapchain(),
        .pImageIndices = &image_index,
    };
    check_if_should_update_swapchain(vkQueuePresentKHR(m_rdr_queue.vk_queue(), &present_info));
}

void App::maybe_update_swapchain()
{
    if (m_should_recreate_swapchain) {
        m_should_recreate_swapchain = false;

        vkDeviceWaitIdle(m_rdr_device.vk_device());

        Check(rdr::Swapchain::create(m_rdr_device, m_rdr_surface, Frames_In_Flight, m_rdr_swapchain));

        VkSurfaceCapabilitiesKHR surface_caps{};
        m_rdr_surface.get_surface_caps_khr(surface_caps);
        VkExtent2D extent = surface_caps.currentExtent;
        m_current_window_size = glm::ivec2(static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height));
        
        m_rdr_swapchain_images = m_rdr_swapchain.get_swapchain_images_khr();
        assert(m_rdr_swapchain_images.size() >= Frames_In_Flight);
        
        m_wait_renderer_complete_semaphores.resize(m_rdr_swapchain_images.size());
        for (auto& semaphore : m_wait_renderer_complete_semaphores) {
            Check(rdr::Semaphore::create(m_rdr_device, semaphore));
        }
    }
}

void App::check_if_should_update_swapchain(VkResult result)
{
    if (result != VK_SUCCESS) {
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            m_should_recreate_swapchain = true;
        }
        else {
            assert(false);
        }
    }
}

std::vector<std::string_view> App::lex_config_file(std::string_view text)
{
    std::vector<std::string_view> tokens;
    tokens.reserve(256);

    size_t line_start = 0;
    for (auto [index, value] : std::views::enumerate(text)) {
        size_t token_start = 0;
        if (value == '\r' || value == '\n' || value == '\0') {
            auto line = text.substr(line_start, index - line_start + 1); // if we are in CRLF we will visit \n after \r and then line will be an empty string
            line_start = value == '\r' ? index + 2 : index + 1;

            for (auto [index, value] : std::views::enumerate(line)) {
                if (value == ' ' || value == '\t' || value == '\r' || value == '\n' || value == '\0') {
                    if (token_start < static_cast<size_t>(index)) {
                        tokens.push_back(line.substr(token_start, index - token_start));
                    }
                    if (value == '\r' || value == '\n') {
                        tokens.push_back("\n"); // issue a new line token
                    }

                    token_start = index + 1;
                }
            }
        }
    }

    return tokens;
}

template<typename T, typename V>
bool parse_value(std::string_view value_name, size_t token_index, std::span<std::string_view> tokens, T&& value_parser, V& out_value) {
    auto token = tokens[token_index];
    if (strncmp(token.data(), value_name.data(), value_name.size()) == 0) {
        if (token_index + 1 >= tokens.size()) {
            std::println("Found no value for {}", value_name);
            return false;
        }
        if (!value_parser(token_index + 1, tokens, out_value)) {
            std::println("Could not parse value for {}", value_name);
            return false;
        }
    }

    return true; // return true even if current token does not match value_name for convenience
}

template<typename T>
auto value_parser() {
    return [](size_t token_index, std::span<std::string_view> tokens, T& out_value) {
        auto token = tokens[token_index];
        if constexpr (std::is_integral_v<T>) { // we have to branch here because the interface of std::from_chars() is different for floats and integers
            if (token.starts_with("0x") || token.starts_with("0X")) { // check if token is a hex value starting with 0x... (necessary because of how std::from_chars() works)
                token = token.substr(2, token.size() - 2);
            }

            auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), out_value, 10); // try parse as base 10 first
            bool success = ec == std::errc() && ptr == token.data() + token.size();
            if (!success) {
                auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), out_value, 16); // then try parse as base 16 (hex) if failed
                return ec == std::errc() && ptr == token.data() + token.size();
            }
            return true;
        }
        else {
            auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), out_value);
            return ec == std::errc() && ptr == token.data() + token.size();
        }
    };
}

template<typename T>
auto array_value_parser() {
    return [](size_t token_index, std::span<std::string_view> tokens, std::vector<T>& out_value) {
        auto vp = value_parser<T>();
        bool compute_length = true;
        size_t out_array_length = 0;

        while (true) { // iterate over tokens twice to first measure array length to allocate memory and then fill the array
            for (size_t i = token_index; i < tokens.size();) {
                if (tokens[i].size() == 1 && *tokens[i].data() == '\\') {
                    if (tokens[i + 1].size() == 1 && *tokens[i + 1].data() == '\n') {
                        i += 2;
                        continue;
                    }
                    else {
                        std::println("expected \"\\n\" after \"\\\" token but found \"{}\"", tokens[i + 1]);
                        return false;
                    }
                }

                if (tokens[i].size() == 1 && *tokens[i].data() == '\n') {
                    break;
                }

                T value;
                if (!vp(i, tokens, value)) {
                    std::println("Unexpected token: \"{}\"", tokens[i]);
                    return false;
                }

                if (compute_length) {
                    out_array_length += 1;
                }
                else {
                    out_value.push_back(value);
                }

                i += 1;
            }

            if (compute_length) {
                compute_length = false; // we are done measuring length and will start filling the array now
                out_value.clear();
                out_value.reserve(out_array_length);
            }
            else {
                break; // we have filled the array
            }
        }

        return true;
    };
}

bool App::load_terrain_shader_data_from_file()
{
    std::println("loading terrain gen config...");

    Text_File file;
    if (!Text_File::open("assets/configs/terrain_gen.txt", file)) {
        return false;
    }

    auto tokens = lex_config_file(file.contents());

    for (size_t i = 0; i < tokens.size(); ++i) {
        if (!parse_value("frequency", i, tokens, value_parser<float>(), m_terrain_gen_shader_data.frequency)) {
            return false;
        }

        if (!parse_value("amplitude", i, tokens, value_parser<float>(), m_terrain_gen_shader_data.amplitude)) {
            return false;
        }

        if (!parse_value("seed", i, tokens, value_parser<uint64_t>(), m_terrain_gen_shader_data.seed)) {
            return false;
        }

        if (!parse_value("octave_weights", i, tokens, array_value_parser<float>(), m_terrain_gen_octave_weights)) {
            return false;
        }
    }

    m_terrain_gen_shader_data.octave_count = static_cast<uint32_t>(m_terrain_gen_octave_weights.size());

    return true;
}

glm::mat4x4 App::camera_from_world() const
{
    glm::mat4x4 rotation = glm::eulerAngleXYZ(m_camera_info.camera_angles_xy.y, m_camera_info.camera_angles_xy.x, 0.f);
    glm::mat4x4 position = glm::translate(glm::mat4x4(1.f), m_camera_info.position);
    return rotation * position;
}

glm::mat4x4 App::world_from_camera() const
{
    return glm::inverse(camera_from_world());
}

bool App::init()
{
    std::println("initializing sdl...");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println("Failed to initialize sdl: {}", SDL_GetError());
        return false;
    }

    if (!load_terrain_shader_data_from_file()) {
        return false;
    }

    if (!rdr::Device::create(m_rdr_device)) {
        return false;
    }
    m_rdr_queue = m_rdr_device.get_device_queue(0);

    if (!rdr::Allocator::create(m_rdr_device, m_rdr_allocator)) {
        return false;
    }

    m_current_window_size = glm::ivec2(1920, 1080);
    if (!rdr::Surface::create_window_and_surface(m_rdr_device, "Memes... the DNA of the soul", m_current_window_size.x, m_current_window_size.y, m_rdr_surface)) {
        return false;
    }
    m_window = m_rdr_surface.window();

    if (!rdr::Swapchain::create(m_rdr_device, m_rdr_surface, Frames_In_Flight, m_rdr_swapchain)) {
        return false;
    }

    m_rdr_swapchain_images = m_rdr_swapchain.get_swapchain_images_khr();
    if (m_rdr_swapchain_images.size() < Frames_In_Flight) {
        std::println("Got unexpected number of swapchain images: {} (Frames_In_Flight == {})", m_rdr_swapchain_images.size(), Frames_In_Flight);
        return false;
    }
    m_wait_renderer_complete_semaphores.resize(m_rdr_swapchain_images.size());
    for (auto& semaphore : m_wait_renderer_complete_semaphores) {
        if (!rdr::Semaphore::create(m_rdr_device, semaphore)) {
            return false;
        }
    }

    if (!rdr::Image::create(m_rdr_allocator, 1920, 1080, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, m_output_image, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT)) {
        return false;
    }
    if (!rdr::Image_View::create(m_rdr_device, m_output_image, 0, VK_FORMAT_R8G8B8A8_UNORM, m_output_image_view)) {
        return false;
    }

    rdr::Shader_Compiler shader_compiler;
    if (!rdr::Shader_Compiler::create(shader_compiler)) {
        return false;
    }
    if (!shader_compiler.compile_from_source_file(m_rdr_device, "assets/shaders/compute/terrain_gen.hlsl", rdr::Shader_Type::Compute, m_shaders[Shaders_Terrain_Gen])) {
        return false;
    }
    if (!shader_compiler.compile_from_source_file(m_rdr_device, "assets/shaders/compute/terrain_normals.hlsl", rdr::Shader_Type::Compute, m_shaders[Shaders_Terrain_Normals])) {
        return false;
    }
    if (!shader_compiler.compile_from_source_file(m_rdr_device, "assets/shaders/compute/terrain_raymarch.hlsl", rdr::Shader_Type::Compute, m_shaders[Shaders_Terrain_Raymarch])) {
        return false;
    }

    if (!rdr::Descriptor_Set_Layout::create_from_shader(m_rdr_device, m_shaders[Shaders_Terrain_Gen], 0, 0, m_descriptor_set_layouts[Shaders_Terrain_Gen])) {
        return false;
    }
    if (!rdr::Descriptor_Set_Layout::create_from_shader(m_rdr_device, m_shaders[Shaders_Terrain_Normals], 0, 0, m_descriptor_set_layouts[Shaders_Terrain_Normals])) {
        return false;
    }
    if (!rdr::Descriptor_Set_Layout::create_from_shader(m_rdr_device, m_shaders[Shaders_Terrain_Raymarch], 0, 0, m_descriptor_set_layouts[Shaders_Terrain_Raymarch])) {
        return false;
    }

    if (!rdr::Buffer::create(m_rdr_allocator, Terrain_Size * Terrain_Size * sizeof(float), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, m_terrain_heght_map_buffer)) {
        return false;
    }
    if (!rdr::Buffer::create(m_rdr_allocator, Terrain_Size * Terrain_Size * sizeof(float[2]), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, m_terrain_normals_buffer)) {
        return false;
    }

    auto terrain_gen_shader_push_constants = m_shaders[Shaders_Terrain_Gen].get_push_constants();
    if (!rdr::Pipeline_Layout::create(m_rdr_device, std::span(&m_descriptor_set_layouts[Shaders_Terrain_Gen], 1), terrain_gen_shader_push_constants,
                                      VK_SHADER_STAGE_COMPUTE_BIT, m_compute_pipeline_layouts[Shaders_Terrain_Gen])) {
        return false;
    }
    auto terrain_normals_shader_push_constants = m_shaders[Shaders_Terrain_Normals].get_push_constants();
    if (!rdr::Pipeline_Layout::create(m_rdr_device, std::span(&m_descriptor_set_layouts[Shaders_Terrain_Normals], 1), terrain_normals_shader_push_constants,
                                      VK_SHADER_STAGE_COMPUTE_BIT, m_compute_pipeline_layouts[Shaders_Terrain_Normals])) {
        return false;
    }
    auto terrain_raymarch_shader_push_constants = m_shaders[Shaders_Terrain_Raymarch].get_push_constants();
    if (!rdr::Pipeline_Layout::create(m_rdr_device, std::span(&m_descriptor_set_layouts[Shaders_Terrain_Raymarch], 1), terrain_raymarch_shader_push_constants,
                                      VK_SHADER_STAGE_COMPUTE_BIT, m_compute_pipeline_layouts[Shaders_Terrain_Raymarch])) {
        return false;
    }

    if (!rdr::Pipeline::create_compute(m_rdr_device, m_shaders, m_compute_pipeline_layouts, m_compute_pipelines)) {
        return false;
    }
    
    if (!rdr::Descriptor_Pool::create(m_rdr_device, m_descriptor_set_layouts, static_cast<uint32_t>(m_descriptor_set_layouts.size()), m_descriptor_pool)) { // allocating one descriptor set per shader
        return false;
    }

    if (!m_descriptor_pool.allocate_descriptor_sets(1, m_descriptor_set_layouts[Shaders_Terrain_Gen], std::span(&m_terrain_gen_descriptor_set, 1))) {
        return false;
    }
    if (!m_descriptor_pool.allocate_descriptor_sets(1, m_descriptor_set_layouts[Shaders_Terrain_Normals], std::span(&m_terrain_normals_descriptor_set, 1))) {
        return false;
    }
    if (!m_descriptor_pool.allocate_descriptor_sets(1, m_descriptor_set_layouts[Shaders_Terrain_Raymarch], std::span(&m_terrain_raymarch_descriptor_set, 1))) {
        return false;
    }

    VkDescriptorBufferInfo terrain_buffer_info{
        .buffer = m_terrain_heght_map_buffer.vk_buffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    std::array<VkWriteDescriptorSet, 1> write_descriptor_set = {
        m_terrain_gen_descriptor_set.write_storage_buffer(0, std::span(&terrain_buffer_info, 1)),
    };
    rdr::Descriptor_Set::update_descriptor_sets(m_rdr_device, write_descriptor_set, std::span<VkCopyDescriptorSet>());

    VkDescriptorBufferInfo terrain_normals_buffer_info{
        .buffer = m_terrain_normals_buffer.vk_buffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    std::array<VkWriteDescriptorSet, 2> write_terrain_normals_descriptor_set = {
        m_terrain_normals_descriptor_set.write_storage_buffer(0, std::span(&terrain_buffer_info, 1)),
        m_terrain_normals_descriptor_set.write_storage_buffer(1, std::span(&terrain_normals_buffer_info, 1)),
    };
    rdr::Descriptor_Set::update_descriptor_sets(m_rdr_device, write_terrain_normals_descriptor_set, std::span<VkCopyDescriptorSet>());

    VkDescriptorImageInfo output_image_info{
        .imageView = m_output_image_view.vk_image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    std::array<VkWriteDescriptorSet, 3> write_terrain_raymarch_descriptor_set = {
        m_terrain_raymarch_descriptor_set.write_storage_buffer(0, std::span(&terrain_buffer_info, 1)),
        m_terrain_raymarch_descriptor_set.write_storage_buffer(1, std::span(&terrain_normals_buffer_info, 1)),
        m_terrain_raymarch_descriptor_set.write_storage_image (2, std::span(&output_image_info, 1)),
    };
    rdr::Descriptor_Set::update_descriptor_sets(m_rdr_device, write_terrain_raymarch_descriptor_set, std::span<VkCopyDescriptorSet>());

    for (auto& shader_buffer : m_terrain_gen_shader_data_buffers) {
        if (!rdr::Buffer::create(m_rdr_allocator, sizeof(Terrain_Gen_Shader_Data),
                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                 shader_buffer)) {
            return false;
        }
    }

    for (auto& shader_buffer : m_terrain_raymarch_info_buffers) {
        if (!rdr::Buffer::create(m_rdr_allocator, sizeof(Terrain_Raymarch_Shader_Data),
                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                 shader_buffer)) {
            return false;
        }
    }

    for (auto& octave_weights : m_terrain_gen_shader_octave_weights) {
        if (!rdr::Buffer::create(m_rdr_allocator, sizeof(float) * m_terrain_gen_octave_weights.size(),
                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                 octave_weights)) {
            return false;
        }
    }

    for (auto& fence : m_next_frame_fences) {
        if (!rdr::Fence::create(m_rdr_device, true, fence)) {
            return false;
        }
    }

    for (auto& semaphore : m_wait_image_acquired_semaphores) {
        if (!rdr::Semaphore::create(m_rdr_device, semaphore)) {
            return false;
        }
    }

    if (!rdr::Command_Pool::create(m_rdr_device, m_rdr_device.vk_queue_family_index(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_command_pool)) {
        return false;
    }
    if (!m_command_pool.allocate_command_buffers(m_command_buffers)) {
        return false;
    }

    return true;
}

void App::main_loop()
{
    uint64_t ticks_last_frame = SDL_GetTicks();

    bool running = true;
    while (running) {
        uint64_t ticks_this_frame = SDL_GetTicks();
        float dt = static_cast<float>(ticks_this_frame - ticks_last_frame) / 1000.f;
        ticks_last_frame = ticks_this_frame;
        
        update(dt);
        render(dt);
        
        process_events(running);
    }
}

bool App::Text_File::open(const std::string& path, Text_File& out_text_file)
{
    Text_File file;
    size_t file_size;
    void* data = SDL_LoadFile(path.c_str(), &file_size);
    if (!data) {
        std::println("Could not load file {}: {}", path, SDL_GetError());
        return false;
    }

    file.m_contents = std::string_view(static_cast<char*>(data), file_size + 1); // string is null terminated so we do file_size + 1 for parsing convenience
    out_text_file = std::move(file);
    return true;
}

App::Text_File::~Text_File()
{
    if (m_contents.data()) {
        SDL_free(const_cast<char*>(m_contents.data()));
    }
}

App::Text_File::Text_File(const Text_File& other)
{
    void* data = SDL_malloc(other.m_contents.size());
    memcpy(data, other.m_contents.data(), other.m_contents.size());
    m_contents = std::string_view(static_cast<char*>(data), other.m_contents.size());
}

App::Text_File& App::Text_File::operator=(const Text_File& other)
{
    this->~Text_File();
    new (this) Text_File(other);
    return *this;
}

App::Text_File::Text_File(Text_File&& other) noexcept
    : m_contents(other.m_contents)
{
    new (&other) Text_File();
}

App::Text_File& App::Text_File::operator=(Text_File&& other) noexcept
{
    this->~Text_File();
    new (this) Text_File(std::move(other));
    return *this;
}
