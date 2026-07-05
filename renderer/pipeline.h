#pragma once

#include "device.h"
#include "pipeline_layout.h"

#include <Volk/volk.h>

#include <utility>
#include <span>
#include <vector>

#if LOG_RENDERER_OBJECT_NAMES
#include <string>
#endif

namespace rdr {
    class Pipeline {
        const Device* m_device;
        VkPipeline m_vk_pipeline;
#if LOG_RENDERER_OBJECT_NAMES
        std::string m_name;
#endif

        Pipeline(const Pipeline& other) = delete;
        Pipeline& operator=(const Pipeline& other) = delete;
    public:
        Pipeline() : m_device(nullptr),
                     m_vk_pipeline(VK_NULL_HANDLE) {}
        ~Pipeline();

        static bool create_compute(const Device& device, std::span<Shader> shaders, std::span<Pipeline_Layout> pipeline_layouts, std::span<Pipeline> out_pipelines);
        static bool create_compute(const Device& device, std::span<VkComputePipelineCreateInfo> create_infos, std::span<Pipeline> out_pipelines);

        Pipeline(Pipeline&& other) noexcept : m_device(other.m_device),
                                              m_vk_pipeline(other.m_vk_pipeline)
#if LOG_RENDERER_OBJECT_NAMES
                                            , m_name(std::move(other.m_name))
#endif
        {
            new (&other) Pipeline();
        }

        Pipeline& operator=(Pipeline&& other) noexcept {
            this->~Pipeline();
            new (this) Pipeline(std::move(other));
            return *this;
        }

        VkPipeline vk_pipeline() const { return m_vk_pipeline; }
    };
}
