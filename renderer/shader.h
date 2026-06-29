#pragma once

#include "device.h"
#include <Volk/volk.h>

#include <optional>
#include <string>

#if LOG_RENDERER_OBJECT_NAMES
#include <utility>
#endif

namespace rdr {
    enum class Shader_Type {
        Vertex = 1,
        Fragment,
        Compute,
    };

    class Shader {
        const Device* m_device;
        VkShaderModule m_vk_shader_module;
#if LOG_RENDERER_OBJECT_NAMES
        std::string m_name;
#endif

        Shader(const Shader& other) = delete;
        Shader& operator=(const Shader& other) = delete;

        void destroy();
    public:
        Shader() : m_device(nullptr),
                   m_vk_shader_module(VK_NULL_HANDLE) {}
        ~Shader() { destroy(); }

        Shader(Shader&& other) noexcept : m_device(other.m_device),
                                          m_vk_shader_module(other.m_vk_shader_module)
#if LOG_RENDERER_OBJECT_NAMES
                                        , m_name(std::move(other.m_name))
#endif
        {
            new (&other) Shader();
        }

        Shader& operator=(Shader&& other) noexcept {
            destroy();
            new (this) Shader(std::move(other));
            return *this;
        }

        static std::optional<Shader> create_from_source_file(const Device& device, const std::string& source_path, Shader_Type shader_type);

        VkShaderModule vk_shader_module() const { return m_vk_shader_module; }
    };
}
