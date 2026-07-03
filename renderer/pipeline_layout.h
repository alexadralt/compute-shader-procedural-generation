#pragma once

#include "device.h"
#include "descriptor_set_layout.h"

#include <Volk/volk.h>
#include <spirv_reflect.h>

#include <utility>
#include <optional>
#include <span>

#if LOG_RENDERER_OBJECT_NAMES
#include <string>
#endif

namespace rdr {
    class Pipeline_Layout {
        const Device* m_device;
        VkPipelineLayout m_vk_pipeline_layout;
#if LOG_RENDERER_OBJECT_NAMES
        std::string m_name;
#endif

        Pipeline_Layout(const Pipeline_Layout& other) = delete;
        Pipeline_Layout& operator=(const Pipeline_Layout& other) = delete;
    public:
        Pipeline_Layout() : m_device(nullptr),
                            m_vk_pipeline_layout(VK_NULL_HANDLE) {}
        ~Pipeline_Layout();

        static std::optional<Pipeline_Layout> create(const Device& device, std::span<const Descriptor_Set_Layout> descriptor_sets, std::span<SpvReflectBlockVariable*> push_constants, VkShaderStageFlags shader_stage_flags);
        static std::optional<Pipeline_Layout> create(const Device& device, const VkPipelineLayoutCreateInfo& create_info);

        Pipeline_Layout(Pipeline_Layout&& other) noexcept : m_device(other.m_device),
                                                            m_vk_pipeline_layout(other.m_vk_pipeline_layout)
#if LOG_RENDERER_OBJECT_NAMES
                                                          , m_name(std::move(other.m_name))
#endif
        {
            new (&other) Pipeline_Layout();
        }

        Pipeline_Layout& operator=(Pipeline_Layout&& other) noexcept {
            this->~Pipeline_Layout();
            new (this) Pipeline_Layout(std::move(other));
            return *this;
        }

        VkPipelineLayout vk_pipeline_layout() const { return m_vk_pipeline_layout; }
    };
}
