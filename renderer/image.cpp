#include "image.h"

#include <print>
#include <array>

#if LOG_RENDERER_OBJECT_NAMES
#include <format>
#endif

void rdr::Image::destroy()
{
    if (m_allocator != nullptr && m_vma_allocation != VK_NULL_HANDLE && m_vk_image != VK_NULL_HANDLE) {
#if LOG_RENDERER_OBJECT_NAMES
        std::println("destroying {}...", m_image_name);
#else
        std::println("destroying image...");
#endif
        vmaDestroyImage(m_allocator->vma_allocator(), m_vk_image, m_vma_allocation);
    }
}

std::optional<rdr::Image> rdr::Image::create_depth_attachmnent(const Device& device, const Allocator& allocator, Uint32 width, Uint32 height)
{
    std::println("creating depth attachment...");
    
    std::array<VkFormat, 2> depth_formats { // note: ordered by priority
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    VkFormat selected_depth_format = VK_FORMAT_UNDEFINED;
    for (VkFormat format : depth_formats) {
        VkFormatProperties2 format_props{
            .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
        };
        vkGetPhysicalDeviceFormatProperties2(device.vk_phys_device(), format, &format_props);
        if (format_props.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            selected_depth_format = format;
            break;
        }
    }

    Image depth_image;
    depth_image.m_allocator = &allocator;
#if LOG_RENDERER_OBJECT_NAMES
    static Uint64 depth_attachment_count = 0;
    depth_image.m_image_name = std::format("depth attachment #{}", depth_attachment_count++);
#endif

    VkImageCreateInfo depth_image_CI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = selected_depth_format,
        .extent{
            .width = width,
            .height = height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo alloc_CI{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    if (vmaCreateImage(allocator.vma_allocator(), &depth_image_CI, &alloc_CI, &depth_image.m_vk_image, &depth_image.m_vma_allocation, nullptr) != VK_SUCCESS) {
        std::println("failed to create depth attachment");
        return std::nullopt;
    }

    return depth_image;
}
