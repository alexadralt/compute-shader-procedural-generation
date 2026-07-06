#include "allocator.h"

#include <print>

void rdr::Allocator::destroy()
{
    if (m_vma_allocator != VK_NULL_HANDLE) {
        std::println("destroying vma allocator");
        vmaDestroyAllocator(m_vma_allocator);
    }
}

bool rdr::Allocator::create(const Device& device, Allocator& out_allocator)
{
    std::println("creating vma allocator...");

    Allocator allocator;
    allocator.m_device = &device;

    VmaVulkanFunctions vk_functions {};

    vk_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vk_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    vk_functions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vk_functions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vk_functions.vkAllocateMemory = vkAllocateMemory;
    vk_functions.vkFreeMemory = vkFreeMemory;
    vk_functions.vkMapMemory = vkMapMemory;
    vk_functions.vkUnmapMemory = vkUnmapMemory;
    vk_functions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vk_functions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    vk_functions.vkBindBufferMemory = vkBindBufferMemory;
    vk_functions.vkBindImageMemory = vkBindImageMemory;
    vk_functions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vk_functions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vk_functions.vkCreateBuffer = vkCreateBuffer;
    vk_functions.vkDestroyBuffer = vkDestroyBuffer;
    vk_functions.vkCreateImage = vkCreateImage;
    vk_functions.vkDestroyImage = vkDestroyImage;
    vk_functions.vkCmdCopyBuffer = vkCmdCopyBuffer;

    vk_functions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
    vk_functions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
    vk_functions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
    vk_functions.vkBindImageMemory2KHR = vkBindImageMemory2;
    vk_functions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;

    VmaAllocatorCreateInfo allocator_CI {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = device.vk_phys_device(),
        .device = device.vk_device(),
        .pVulkanFunctions = &vk_functions,
        .instance = device.vk_instance(),
    };
    if (vmaCreateAllocator(&allocator_CI, &allocator.m_vma_allocator) != VK_SUCCESS) {
        std::println("failed to create vma allocator");
        return false;
    }

    out_allocator = std::move(allocator);
    return true;
}
