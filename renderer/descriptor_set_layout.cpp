#include "descriptor_set_layout.h"

#include <spirv_reflect.h>

#include <print>

#if LOG_RENDERER_OBJECT_NAMES
#include <format>
#endif

rdr::Descriptor_Set_Layout::~Descriptor_Set_Layout()
{
    if (m_device != nullptr && m_vk_descriptor_set_layout != VK_NULL_HANDLE) {
#if LOG_RENDERER_OBJECT_NAMES
        std::println("destroying descriptor set layout {}...", m_name);
#else
        std::println("destroying descriptor set layout...");
#endif
        vkDestroyDescriptorSetLayout(m_device->vk_device(), m_vk_descriptor_set_layout, nullptr);
    }
}

std::optional<rdr::Descriptor_Set_Layout> rdr::Descriptor_Set_Layout::create_from_shader(const Device& device, const Shader& shader, Uint32 set_number)
{
    std::println("creating descriptor set layout from shader...");

    SpvReflectResult spv_result;
    const SpvReflectDescriptorSet* shader_descriptor_set = spvReflectGetDescriptorSet(shader.spv_shader_module(), set_number, &spv_result);
    if (spv_result != SPV_REFLECT_RESULT_SUCCESS) {
        std::println("Could not get descriptor set from shader: {}", static_cast<Sint32>(spv_result));
        return std::nullopt;
    }

    std::vector<VkDescriptorSetLayoutBinding> vk_descriptor_layout_bindings;
    vk_descriptor_layout_bindings.reserve(shader_descriptor_set->binding_count);

    for (Uint32 i = 0; i < shader_descriptor_set->binding_count; ++i) {
        SpvReflectDescriptorBinding* shader_descriptor_binding = shader_descriptor_set->bindings[i];

        VkDescriptorSetLayoutBinding vk_descriptor_layout_binding{
            .binding = shader_descriptor_binding->binding,
            .descriptorType = static_cast<VkDescriptorType>(shader_descriptor_binding->descriptor_type),
            .descriptorCount = shader_descriptor_binding->count,
            .stageFlags = static_cast<VkShaderStageFlags>(shader.spv_shader_module()->shader_stage),
        };
        vk_descriptor_layout_bindings.push_back(vk_descriptor_layout_binding);
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_CI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<Uint32>(vk_descriptor_layout_bindings.size()),
        .pBindings = vk_descriptor_layout_bindings.data(),
    };

    return create(device, descriptor_set_layout_CI);
}

std::optional<rdr::Descriptor_Set_Layout> rdr::Descriptor_Set_Layout::create(const Device& device, const VkDescriptorSetLayoutCreateInfo& create_info)
{
    std::println("creating descriptor set layout...");
    
    Descriptor_Set_Layout descriptor_set_layout;
    descriptor_set_layout.m_device = &device;

#if LOG_RENDERER_OBJECT_NAMES
    static Uint64 name_index = 0;
    descriptor_set_layout.m_name = std::format("#{}", name_index++);
#endif

    VkResult result = vkCreateDescriptorSetLayout(device.vk_device(), &create_info, nullptr, &descriptor_set_layout.m_vk_descriptor_set_layout);
    if (result != VK_SUCCESS) {
        std::println("Could not create descriptor set layout: {}", static_cast<Sint32>(result));
        return std::nullopt;
    }

    return descriptor_set_layout;
}
