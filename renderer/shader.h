#pragma once

#include "device.h"

#include <Volk/volk.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

struct SpvReflectShaderModule;
struct SpvReflectBlockVariable;
struct SpvReflectDescriptorBinding;
struct SpvReflectDescriptorSet;

namespace rdr {
    enum class Shader_Type {
        Vertex = 1,
        Fragment,
        Compute,
    };

    class Shader {
        const Device* m_device;
        VkShaderModule m_vk_shader_module;
        SpvReflectShaderModule* m_spv_shader_module;
#if LOG_RENDERER_OBJECT_NAMES
        std::string m_name;
#endif

        Shader(const Shader& other) = delete;
        Shader& operator=(const Shader& other) = delete;
    public:
        Shader() : m_device(nullptr),
                   m_vk_shader_module(VK_NULL_HANDLE),
                   m_spv_shader_module(nullptr) { }
        ~Shader();

        Shader(const Device& device, VkShaderModule shader_module, SpvReflectShaderModule* spv_shader_module) : m_device(&device),
                                                                                                                m_vk_shader_module(shader_module),
                                                                                                                m_spv_shader_module(spv_shader_module) {}

#if LOG_RENDERER_OBJECT_NAMES
        Shader(const Device& device, VkShaderModule shader_module, SpvReflectShaderModule* spv_shader_module, std::string&& name) : m_device(&device),
                                                                                                                                    m_vk_shader_module(shader_module),
                                                                                                                                    m_spv_shader_module(spv_shader_module),
                                                                                                                                    m_name(std::move(name)) {}
#endif

        Shader(Shader&& other) noexcept : m_device(other.m_device),
                                          m_vk_shader_module(other.m_vk_shader_module),
                                          m_spv_shader_module(other.m_spv_shader_module)
#if LOG_RENDERER_OBJECT_NAMES
                                        , m_name(std::move(other.m_name))
#endif
        {
            new (&other) Shader();
        }

        Shader& operator=(Shader&& other) noexcept {
            this->~Shader();
            new (this) Shader(std::move(other));
            return *this;
        }

        VkShaderModule vk_shader_module() const { return m_vk_shader_module; }

        std::vector<SpvReflectBlockVariable*> get_push_constants() const;
        std::vector<SpvReflectDescriptorBinding*> get_descriptor_bindings() const;
        std::vector< SpvReflectDescriptorSet*> get_descriptor_sets() const;
    };
}
