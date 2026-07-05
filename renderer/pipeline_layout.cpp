#include "pipeline_layout.h"

#include <SDL3/SDL_stdinc.h>

#include <print>
#include <vector>

#if LOG_RENDERER_OBJECT_NAMES
#include <format>
#endif

rdr::Pipeline_Layout::~Pipeline_Layout()
{
    if (m_device != nullptr && m_vk_pipeline_layout != VK_NULL_HANDLE) {
#if LOG_RENDERER_OBJECT_NAMES
        std::println("destroying pipeline layout {}...", m_name);
#else
        std::println("destroying pipeline layout...");
#endif
        vkDestroyPipelineLayout(m_device->vk_device(), m_vk_pipeline_layout, nullptr);
    }
}

bool rdr::Pipeline_Layout::create(const Device& device, std::span<const Descriptor_Set_Layout> descriptor_sets, std::span<SpvReflectBlockVariable*> push_constants, VkShaderStageFlags shader_stage_flags, Pipeline_Layout& out_pipeline_layout)
{
    std::vector<VkPushConstantRange> vk_push_constant_ranges(push_constants.size());
    for (size_t i = 0; i < push_constants.size(); ++i) {
        const SpvReflectBlockVariable* push_constant = push_constants[i];
        vk_push_constant_ranges[i].size = push_constant->size;
        vk_push_constant_ranges[i].offset = push_constant->offset;
        vk_push_constant_ranges[i].stageFlags = shader_stage_flags;
    }

    std::vector<VkDescriptorSetLayout> vk_descriptor_set_layouts(descriptor_sets.size());
    for (size_t i = 0; i < descriptor_sets.size(); ++i) {
        vk_descriptor_set_layouts[i] = descriptor_sets[i].vk_descriptor_set_layout();
    }

    VkPipelineLayoutCreateInfo vk_pipeline_layout_CI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<Uint32>(descriptor_sets.size()),
        .pSetLayouts = vk_descriptor_set_layouts.data(),
        .pushConstantRangeCount = static_cast<Uint32>(push_constants.size()),
        .pPushConstantRanges = vk_push_constant_ranges.data(),
    };

    return create(device, vk_pipeline_layout_CI, out_pipeline_layout);
}

bool rdr::Pipeline_Layout::create(const Device& device, const VkPipelineLayoutCreateInfo& create_info, Pipeline_Layout& out_pipeline_layout)
{
    std::println("creating pipeline layout...");
    
    Pipeline_Layout pipeline_layout;
    pipeline_layout.m_device = &device;
#if LOG_RENDERER_OBJECT_NAMES
    static Uint64 name_index = 0;
    pipeline_layout.m_name = std::format("#{}", name_index++);
#endif

    VkResult result = vkCreatePipelineLayout(device.vk_device(), &create_info, nullptr, &pipeline_layout.m_vk_pipeline_layout);
    if (result != VK_SUCCESS) {
        std::println("Could not create pipeline layout: {}", static_cast<Sint32>(result));
        return false;
    }

    out_pipeline_layout = std::move(pipeline_layout);
    return true;
}
