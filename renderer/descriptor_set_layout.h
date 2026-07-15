#pragma once

#include "device.h"
#include "shader.h"

#include <Volk/volk.h>
#include <SDL3/SDL_stdinc.h>

#include <utility>
#include <vector>
#include <span>

#if LOG_RENDERER_OBJECT_NAMES
#include <string>
#endif

namespace rdr {
    class Descriptor_Set_Layout {
        const Device* m_device;
        VkDescriptorSetLayout m_vk_descriptor_set_layout;
        std::vector<VkDescriptorSetLayoutBinding> m_vk_layout_bindings;
#if LOG_RENDERER_OBJECT_NAMES
        std::string m_name;
#endif

        Descriptor_Set_Layout(const Descriptor_Set_Layout& other) = delete;
        Descriptor_Set_Layout& operator=(const Descriptor_Set_Layout& other) = delete;
    public:
        Descriptor_Set_Layout() : m_device(nullptr),
                                  m_vk_descriptor_set_layout(VK_NULL_HANDLE) {}
        ~Descriptor_Set_Layout();

        static bool create_from_shader(const Device& device, const Shader& shader, Uint32 set_number, VkDescriptorSetLayoutCreateFlags flags, uint32_t variable_descriptor_array_size, Descriptor_Set_Layout& out_descriptor_set_layout);
        static bool create(const Device& device, std::vector<VkDescriptorSetLayoutBinding>&& layout_bindings, VkDescriptorSetLayoutCreateFlags flags, std::span<VkDescriptorBindingFlags> binding_flags, Descriptor_Set_Layout& out_descriptor_set_layout);

        Descriptor_Set_Layout(Descriptor_Set_Layout&& other) noexcept : m_device(other.m_device),
                                                                        m_vk_descriptor_set_layout(other.m_vk_descriptor_set_layout),
                                                                        m_vk_layout_bindings(std::move(other.m_vk_layout_bindings))
#if LOG_RENDERER_OBJECT_NAMES
                                                                      , m_name(std::move(other.m_name))
#endif
        {
            new (&other) Descriptor_Set_Layout();
        }

        Descriptor_Set_Layout& operator=(Descriptor_Set_Layout&& other) noexcept {
            this->~Descriptor_Set_Layout();
            new (this) Descriptor_Set_Layout(std::move(other));
            return *this;
        }

        VkDescriptorSetLayout vk_descriptor_set_layout() const { return m_vk_descriptor_set_layout; }
        const std::vector<VkDescriptorSetLayoutBinding>& vk_layout_bindings() const { return m_vk_layout_bindings; }
    };
}
