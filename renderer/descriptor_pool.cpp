#include "descriptor_pool.h"

#include <print>
#include <vector>
#include <cassert>

rdr::Descriptor_Pool::~Descriptor_Pool()
{
    if (m_device && m_vk_descriptor_pool != VK_NULL_HANDLE) {
        std::println("destroying descriptor pool...");
        vkDestroyDescriptorPool(m_device->vk_device(), m_vk_descriptor_pool, nullptr);
    }
}

bool rdr::Descriptor_Pool::create(const Device& device, std::span<Descriptor_Set_Layout> descriptor_set_layouts, uint32_t max_sets, Descriptor_Pool& out_descriptor_pool)
{
    uint32_t pool_sizes_count = 0;
    for (const Descriptor_Set_Layout& set : descriptor_set_layouts) {
        pool_sizes_count += set.vk_layout_bindings().size();
    }

    std::vector<VkDescriptorPoolSize> pool_sizes;
    pool_sizes.reserve(pool_sizes_count);
    for (const Descriptor_Set_Layout& set : descriptor_set_layouts) {
        for (auto& binding : set.vk_layout_bindings()) {
            VkDescriptorPoolSize pool_size{
                .type = binding.descriptorType,
                .descriptorCount = binding.descriptorCount,
            };
            pool_sizes.push_back(pool_size);
        }
    }
    
    VkDescriptorPoolCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = max_sets,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };

    return create(device, create_info, out_descriptor_pool);
}

bool rdr::Descriptor_Pool::create(const Device& device, const VkDescriptorPoolCreateInfo& create_info, Descriptor_Pool& out_descriptor_pool)
{
    std::println("creating descriptor pool...");

    Descriptor_Pool descriptor_pool;
    descriptor_pool.m_device = &device;
    VkResult result = vkCreateDescriptorPool(device.vk_device(), &create_info, nullptr, &descriptor_pool.m_vk_descriptor_pool);
    if (result != VK_SUCCESS) {
        std::println("Could not create descriptor pool: {}", static_cast<int32_t>(result));
        return false;
    }

    out_descriptor_pool = std::move(descriptor_pool);
    return true;
}

bool rdr::Descriptor_Pool::allocate_descriptor_sets(uint32_t set_count, const Descriptor_Set_Layout& layout, std::span<Descriptor_Set> out_descriptor_sets) const
{
    assert(set_count == static_cast<uint32_t>(out_descriptor_sets.size()));

    // yes, we really have to do that
    std::vector<VkDescriptorSetLayout> vk_layouts(set_count);
    for (VkDescriptorSetLayout& vk_layout : vk_layouts) {
        vk_layout = layout.vk_descriptor_set_layout();
    }
    
    VkDescriptorSetAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_vk_descriptor_pool,
        .descriptorSetCount = set_count,
        .pSetLayouts = vk_layouts.data(),
    };

    std::vector<VkDescriptorSet> vk_descriptor_sets(set_count);
    VkResult result = vkAllocateDescriptorSets(m_device->vk_device(), &alloc_info, vk_descriptor_sets.data());
    if (result != VK_SUCCESS) {
        std::println("Could not allocate descriptor sets: {}", static_cast<int32_t>(result));
        return false;
    }

    for (size_t i = 0; i < vk_descriptor_sets.size(); ++i) {
        Descriptor_Set descriptor_set(vk_descriptor_sets[i]);
        out_descriptor_sets[i] = std::move(descriptor_set);
    }

    return true;
}
