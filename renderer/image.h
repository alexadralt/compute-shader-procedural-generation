#pragma once

#include "device.h"
#include "allocator.h"

#include <Volk/volk.h>
#include <SDL3/SDL_stdinc.h>

#include <utility>
#include <string>
#include <array>

namespace rdr {
    class Image {
        const Allocator* m_allocator;
        VmaAllocation m_vma_allocation;
        VkImage m_vk_image;
        std::array<uint32_t, 3> m_image_size;

#if LOG_RENDERER_OBJECT_NAMES
        std::string m_image_name;
#endif

        Image(const Image& other) = delete;
        Image& operator=(const Image& other) = delete;

        void destroy();
    public:
        static bool create_depth_attachmnent(const Device& device, const Allocator& allocator, Uint32 width, Uint32 height, Image& out_image);
        static bool create(const Allocator& allocator, Uint32 width, Uint32 height, VkFormat image_format, VkImageUsageFlags image_usage, VmaAllocationCreateFlags allocation_flags, Image& out_image, VkImageCreateFlags flags = 0);

        Image() : m_allocator(nullptr),
                  m_vma_allocation(VK_NULL_HANDLE),
                  m_vk_image(VK_NULL_HANDLE),
                  m_image_size() {}
        
        // create non-owning image
        Image(VkImage vk_image) : m_allocator(nullptr),
                                  m_vma_allocation(VK_NULL_HANDLE),
                                  m_vk_image(vk_image) {}

        ~Image() { destroy(); }

        Image(Image&& other) noexcept : m_allocator(other.m_allocator),
                                        m_vma_allocation(other.m_vma_allocation),
                                        m_vk_image(other.m_vk_image),
                                        m_image_size(other.m_image_size)
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

        VkImage vk_image() const { return m_vk_image; }
        const std::array<uint32_t, 3>& image_size() const { return m_image_size; }
    };
}
