#include "shader.h"

#include <spirv_reflect.h>

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

    if (m_spv_shader_module != nullptr) {
        spvReflectDestroyShaderModule(m_spv_shader_module);
        delete m_spv_shader_module;
    }
}

std::vector<SpvReflectBlockVariable*> rdr::Shader::get_push_constants() const
{
    Uint32 push_constants_count = 0;
    SpvReflectResult spv_result = spvReflectEnumeratePushConstantBlocks(m_spv_shader_module, &push_constants_count, nullptr);
    if (spv_result != SPV_REFLECT_RESULT_SUCCESS) {
        std::println("Could not get shader push constants count: {}", static_cast<Sint32>(spv_result));
        return std::vector<SpvReflectBlockVariable*>();
    }

    std::vector<SpvReflectBlockVariable*> push_constants(push_constants_count);
    spv_result = spvReflectEnumeratePushConstantBlocks(m_spv_shader_module, &push_constants_count, push_constants.data());
    if (spv_result != SPV_REFLECT_RESULT_SUCCESS) {
        std::println("Could not get shader push constants: {}", static_cast<Sint32>(spv_result));
        return std::vector<SpvReflectBlockVariable*>();
    }

    return push_constants;
}

std::vector<SpvReflectDescriptorBinding*> rdr::Shader::get_descriptor_bindings() const
{
    Uint32 descriptor_bindings_count = 0;
    SpvReflectResult spv_result = spvReflectEnumerateDescriptorBindings(m_spv_shader_module, &descriptor_bindings_count, nullptr);
    if (spv_result != SPV_REFLECT_RESULT_SUCCESS) {
        std::println("Could not get shader descriptor bindings count: {}", static_cast<Sint32>(spv_result));
        return std::vector<SpvReflectDescriptorBinding*>();
    }

    std::vector<SpvReflectDescriptorBinding*> descritpor_bindings(descriptor_bindings_count);
    spv_result = spvReflectEnumerateDescriptorBindings(m_spv_shader_module, &descriptor_bindings_count, descritpor_bindings.data());
    if (spv_result != SPV_REFLECT_RESULT_SUCCESS) {
        std::println("Could not get shader descriptor bindings: {}", static_cast<Sint32>(spv_result));
        return std::vector<SpvReflectDescriptorBinding*>();
    }

    return descritpor_bindings;
}

std::vector<SpvReflectDescriptorSet*> rdr::Shader::get_descriptor_sets() const
{
    Uint32 descriptor_sets_count = 0;
    SpvReflectResult spv_result = spvReflectEnumerateDescriptorSets(m_spv_shader_module, &descriptor_sets_count, nullptr);
    if (spv_result != SPV_REFLECT_RESULT_SUCCESS) {
        std::println("Could not get shader descriptor bindings count: {}", static_cast<Sint32>(spv_result));
        return std::vector<SpvReflectDescriptorSet*>();
    }

    std::vector<SpvReflectDescriptorSet*> descritpor_sets(descriptor_sets_count);
    spv_result = spvReflectEnumerateDescriptorSets(m_spv_shader_module, &descriptor_sets_count, descritpor_sets.data());
    if (spv_result != SPV_REFLECT_RESULT_SUCCESS) {
        std::println("Could not get shader descriptor bindings: {}", static_cast<Sint32>(spv_result));
        return std::vector<SpvReflectDescriptorSet*>();
    }

    return descritpor_sets;
}
