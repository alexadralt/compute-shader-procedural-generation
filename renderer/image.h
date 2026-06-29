#pragma once

#include "device.h"
#include "allocator.h"

#include <Volk/volk.h>
#include <SDL3/SDL_stdinc.h>

#include <utility>
#include <optional>
#include <string>

namespace rdr {
    class Image {
        const Allocator* m_allocator;
        VmaAllocation m_vma_allocation;
        VkImage m_vk_image;

#if LOG_RENDERER_OBJECT_NAMES
        std::string m_image_name;
#endif

        Image(const Image& other) = delete;
        Image& operator=(const Image& other) = delete;

        void destroy();
    public:
        static std::optional<Image> create_depth_attachmnent(const Device& device, const Allocator& allocator, Uint32 width, Uint32 height);
        static std::optional<Image> create(const Allocator& allocator, Uint32 width, Uint32 height, VkFormat image_format, VkImageUsageFlags image_usage, VmaAllocationCreateFlags allocation_flags = 0);

        Image() : m_allocator(nullptr),
                  m_vma_allocation(VK_NULL_HANDLE),
                  m_vk_image(VK_NULL_HANDLE) {}
        
        // create non-owning image
        Image(VkImage vk_image) : m_allocator(nullptr),
                                  m_vma_allocation(VK_NULL_HANDLE),
                                  m_vk_image(vk_image) {}

        ~Image() { destroy(); }

        Image(Image&& other) noexcept : m_allocator(other.m_allocator),
                                        m_vma_allocation(other.m_vma_allocation),
                                        m_vk_image(other.m_vk_image)
#if LOG_RENDERER_OBJECT_NAMES
                                      , m_image_name(std::move(other.m_image_name))
#endif
        {
            new (&other) Image();
        }

        Image& operator=(Image&& other) noexcept {
            destroy();
            new (this) Image(std::move(other));
            return *this;
        }
    };
}
