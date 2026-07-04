#pragma once

#include "device.h"

#include <Volk/volk.h>

#include <span>

namespace rdr {
    class Descriptor_Set {
        VkDescriptorSet m_vk_descriptor_set;
    public:
        Descriptor_Set() : m_vk_descriptor_set(VK_NULL_HANDLE) {}
        Descriptor_Set(VkDescriptorSet vk_descriptor_set) : m_vk_descriptor_set(vk_descriptor_set) {}

        static void update_descriptor_sets(const Device& device, std::span<VkWriteDescriptorSet> write_descriptor_sets, std::span<VkCopyDescriptorSet> copy_descriptor_sets) {
            vkUpdateDescriptorSets(device.vk_device(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), static_cast<uint32_t>(copy_descriptor_sets.size()), copy_descriptor_sets.data());
        }
        
        VkDescriptorSet vk_descriptor_set() const { return m_vk_descriptor_set; }
        
        VkWriteDescriptorSet write_storage_image(uint32_t binding, std::span<VkDescriptorImageInfo> image_infos) const {
            return VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_vk_descriptor_set,
                .dstBinding = binding,
                .descriptorCount = static_cast<uint32_t>(image_infos.size()),
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = image_infos.data(),
            };
        }

        VkWriteDescriptorSet write_storage_buffer(uint32_t binding, std::span<VkDescriptorBufferInfo> buffer_infos) const {
            return VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_vk_descriptor_set,
                .dstBinding = binding,
                .descriptorCount = static_cast<uint32_t>(buffer_infos.size()),
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = buffer_infos.data(),
            };
        }
    };
}
