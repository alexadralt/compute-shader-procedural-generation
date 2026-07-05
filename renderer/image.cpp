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
        std::println("destroying vk image...");
#endif
        vmaDestroyImage(m_allocator->vma_allocator(), m_vk_image, m_vma_allocation);
    }
}

bool rdr::Image::create_depth_attachmnent(const Device& device, const Allocator& allocator, Uint32 width, Uint32 height, Image& out_image)
{
    std::println("creating depth attachment...");

    std::array<VkFormat, 2> depth_formats{ // note: ordered by priority
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

    if (!create(allocator, width, height, selected_depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, out_image)) {
        return false;
    }
#if LOG_RENDERER_OBJECT_NAMES
    static Uint64 depth_attachment_count = 0;
    out_image.m_image_name = std::format("depth attachment #{}", depth_attachment_count++);
#endif

    return true;
}

bool rdr::Image::create(const Allocator& allocator, Uint32 width, Uint32 height, VkFormat image_format, VkImageUsageFlags image_usage, VmaAllocationCreateFlags allocation_flags, Image& out_image)
{
    std::println("creating vk image...");

    Image image;
    image.m_allocator = &allocator;
#if LOG_RENDERER_OBJECT_NAMES
    static Uint64 vk_image_count = 0;
    image.m_image_name = std::format("vk image #{}", vk_image_count++);
#endif

    VkImageCreateInfo image_CI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = image_format,
        .extent{
            .width = width,
            .height = height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = image_usage,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo alloc_CI{
        .flags = allocation_flags,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    
    VkResult result = vmaCreateImage(allocator.vma_allocator(), &image_CI, &alloc_CI, &image.m_vk_image, &image.m_vma_allocation, nullptr);
    if (result != VK_SUCCESS) {
        std::println("failed to create vk image: {}", static_cast<Sint32>(result));
        return false;
    }

    out_image = std::move(image);
    return true;
}
