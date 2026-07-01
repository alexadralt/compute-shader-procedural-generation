#include "shader.h"

#include <print>

rdr::Shader::~Shader()
{
    if (m_vk_shader_module != VK_NULL_HANDLE) {
#if LOG_RENDERER_OBJECT_NAMES
        std::println("destroying shader module {}...", m_name);
#else
        std::println("destroying shader module...");
#endif
        vkDestroyShaderModule(m_device->vk_device(), m_vk_shader_module, nullptr);
    }
}
