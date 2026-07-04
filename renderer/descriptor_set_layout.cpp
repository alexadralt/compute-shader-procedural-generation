#include "descriptor_set_layout.h"

#include <spirv_reflect.h>

#include <print>
#include <vector>

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

bool rdr::Descriptor_Set_Layout::create_from_shader(const Device& device, const Shader& shader, Uint32 set_number, VkDescriptorSetLayoutCreateFlags flags, Descriptor_Set_Layout& out_descriptor_set_layout)
{
    std::println("creating descriptor set layout from shader...");

    SpvReflectResult spv_result;
    const SpvReflectDescriptorSet* shader_descriptor_set = spvReflectGetDescriptorSet(shader.spv_shader_module(), set_number, &spv_result);
    if (spv_result != SPV_REFLECT_RESULT_SUCCESS) {
        std::println("Could not get descriptor set from shader: {}", static_cast<Sint32>(spv_result));
        return false;
    }

    std::vector<VkDescriptorSetLayoutBinding> vk_descriptor_layout_bindings(shader_descriptor_set->binding_count);
    for (Uint32 i = 0; i < shader_descriptor_set->binding_count; ++i) {
        SpvReflectDescriptorBinding* shader_descriptor_binding = shader_descriptor_set->bindings[i];

        vk_descriptor_layout_bindings[i].binding = shader_descriptor_binding->binding;
        vk_descriptor_layout_bindings[i].descriptorType = static_cast<VkDescriptorType>(shader_descriptor_binding->descriptor_type);
        vk_descriptor_layout_bindings[i].descriptorCount = shader_descriptor_binding->count;
        vk_descriptor_layout_bindings[i].stageFlags = static_cast<VkShaderStageFlags>(shader.spv_shader_module()->shader_stage);
    }

    return create(device, std::move(vk_descriptor_layout_bindings), flags, out_descriptor_set_layout);
}

bool rdr::Descriptor_Set_Layout::create(const Device& device, std::vector<VkDescriptorSetLayoutBinding>&& layout_bindings, VkDescriptorSetLayoutCreateFlags flags, Descriptor_Set_Layout& out_descriptor_set_layout)
{
    std::println("creating descriptor set layout...");

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_CI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags = flags,
        .bindingCount = static_cast<Uint32>(layout_bindings.size()),
        .pBindings = layout_bindings.data(),
    };
    
    Descriptor_Set_Layout descriptor_set_layout;
    descriptor_set_layout.m_device = &device;
    descriptor_set_layout.m_vk_layout_bindings = std::move(layout_bindings);
#if LOG_RENDERER_OBJECT_NAMES
    static Uint64 name_index = 0;
    descriptor_set_layout.m_name = std::format("#{}", name_index++);
#endif

    VkResult result = vkCreateDescriptorSetLayout(device.vk_device(), &descriptor_set_layout_CI, nullptr, &descriptor_set_layout.m_vk_descriptor_set_layout);
    if (result != VK_SUCCESS) {
        std::println("Could not create descriptor set layout: {}", static_cast<Sint32>(result));
        return false;
    }

    out_descriptor_set_layout = std::move(descriptor_set_layout);
    return true;
}
