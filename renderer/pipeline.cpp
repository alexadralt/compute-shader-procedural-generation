#include "pipeline.h"

#include <SDL3/SDL_stdinc.h>

#include <print>
#include <cassert>

#if LOG_RENDERER_OBJECT_NAMES
#include <format>
#endif

rdr::Pipeline::~Pipeline()
{
    if (m_device && m_vk_pipeline != nullptr) {
#if LOG_RENDERER_OBJECT_NAMES
        std::println("destroying {}...", m_name);
#else
        std::println("destroying pipeline...");
#endif
        vkDestroyPipeline(m_device->vk_device(), m_vk_pipeline, nullptr);
    }
}

std::vector<rdr::Pipeline> rdr::Pipeline::create_compute(const Device& device, std::span<Shader> shaders, std::span<Pipeline_Layout> pipeline_layouts)
{
    assert(shaders.size() == pipeline_layouts.size());

    std::vector<VkComputePipelineCreateInfo> create_infos(shaders.size());
    for (size_t i = 0; i < create_infos.size(); ++i) {
        VkComputePipelineCreateInfo& create_info = create_infos[i];
        create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        create_info.stage = VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shaders[i].vk_shader_module(),
            .pName = "main",
        };
        create_info.layout = pipeline_layouts[i].vk_pipeline_layout();
    }

    return create_compute(device, std::span(create_infos));
}

std::vector<rdr::Pipeline> rdr::Pipeline::create_compute(const Device& device, std::span<VkComputePipelineCreateInfo> create_infos)
{
    std::println("creating compute pipelines...");

    std::vector<Pipeline> compute_pipelines(create_infos.size());
    for (Pipeline& pipeline : compute_pipelines) {
        pipeline.m_device = &device;
#if LOG_RENDERER_OBJECT_NAMES
        static Uint64 name_index = 0;
        pipeline.m_name = std::format("compute pipeline #{}", name_index++);
#endif
    }

    std::vector<VkPipeline> vk_pipelines(create_infos.size());
    VkResult result = vkCreateComputePipelines(device.vk_device(), VK_NULL_HANDLE, static_cast<Uint32>(create_infos.size()), create_infos.data(), nullptr, vk_pipelines.data());
    if (result != VK_SUCCESS) {
        std::println("Could not create compute pipelines: {}", static_cast<Sint32>(result));
        return std::vector<Pipeline>();
    }

    for (size_t i = 0; i < compute_pipelines.size(); ++i) {
        compute_pipelines[i].m_vk_pipeline = vk_pipelines[i];
    }

    return compute_pipelines;
}
