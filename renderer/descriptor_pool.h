#pragma once

#include "device.h"
#include "descriptor_set_layout.h"
#include "descriptor_set.h"

#include <Volk/volk.h>
#include <spirv_reflect.h>

#include <utility>
#include <span>

namespace rdr {
    class Descriptor_Pool {
        const Device* m_device;
        VkDescriptorPool m_vk_descriptor_pool;

        Descriptor_Pool(const Descriptor_Pool& other) = delete;
        Descriptor_Pool& operator=(const Descriptor_Pool& other) = delete;
    public:
        Descriptor_Pool() : m_device(nullptr),
                            m_vk_descriptor_pool(VK_NULL_HANDLE) {}
        ~Descriptor_Pool();

        static bool create(const Device& device, std::span<Descriptor_Set_Layout> descriptor_set_layouts, uint32_t max_sets, Descriptor_Pool& out_descriptor_pool);
        static bool create(const Device& device, const VkDescriptorPoolCreateInfo& create_info, Descriptor_Pool& out_descriptor_pool);

        Descriptor_Pool(Descriptor_Pool&& other) noexcept : m_device(other.m_device),
                                                            m_vk_descriptor_pool(other.m_vk_descriptor_pool)
        {
            new (&other) Descriptor_Pool();
        }

        Descriptor_Pool& operator=(Descriptor_Pool&& other) noexcept {
            this->~Descriptor_Pool();
            new (this) Descriptor_Pool(std::move(other));
            return *this;
        }

        VkDescriptorPool vk_descriptor_pool() const { return m_vk_descriptor_pool; }

        bool allocate_descriptor_sets(uint32_t set_count, const Descriptor_Set_Layout& layout, std::span<Descriptor_Set> out_descriptor_sets) const;
    };
}
