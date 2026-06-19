#include "device.h"

#include <SDL3/SDL_vulkan.h>

#include <print>
#include <vector>

void rdr::Device::destroy()
{
    if (m_vk_device != VK_NULL_HANDLE) {
        std::println("destroying logical device...");
        vkDestroyDevice(m_vk_device, nullptr);
    }
    
    if (m_vk_instance != VK_NULL_HANDLE)
    {
        std::println("destroying vulkan instance...");
        vkDestroyInstance(m_vk_instance, nullptr);
        SDL_Vulkan_UnloadLibrary();
    }
}

std::pair<rdr::Device, bool> rdr::Device::create()
{
    std::println("calling SLD_Vulkan_LoadLibrary()...");
    if (!SDL_Vulkan_LoadLibrary(nullptr)) {
        std::println("failed to call SLD_Vulkan_LoadLibrary()");
        return { Device(), false };
    }

    std::println("initializing volk...");
    if (volkInitialize() != VK_SUCCESS) {
        std::println("failed to initialize volk");
        return { Device(), false };
    }

    Device device;

    VkApplicationInfo app_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Making mother of all omelettes here Jack",
        .apiVersion = VK_API_VERSION_1_4,
    };

    Uint32 instance_extensions_count = 0;
    char const* const* instance_extensions = SDL_Vulkan_GetInstanceExtensions(&instance_extensions_count);

    VkInstanceCreateInfo instance_CI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = instance_extensions_count,
        .ppEnabledExtensionNames = instance_extensions,
    };

    std::println("creating vulkan instance...");
    if (vkCreateInstance(&instance_CI, nullptr, &device.m_vk_instance) != VK_SUCCESS) {
        std::println("failed to create vulkan instance");
        return { Device(), false};
    }

    volkLoadInstance(device.m_vk_instance);

    std::println("enumerating physical devices...");
    Uint32 phys_device_count = 0;
    if (vkEnumeratePhysicalDevices(device.m_vk_instance, &phys_device_count, nullptr) != VK_SUCCESS) {
        std::println("failed to get count of physical devices");
        return { Device(), false };
    }

    if (phys_device_count == 0) {
        std::println("found 0 physical devices");
        return { Device(), false };
    }
    
    std::vector<VkPhysicalDevice> phys_devices(phys_device_count);
    if (vkEnumeratePhysicalDevices(device.m_vk_instance, &phys_device_count, phys_devices.data()) != VK_SUCCESS) {
        std::println("failed to get list of physical devices");
        return { Device(), false };
    }

    std::println("found following physical devices:");
    VkPhysicalDeviceProperties2 phys_device_properties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    Sint32 phys_device_index = 0;
    Sint32 selected_phys_device_index = 0;
    for (auto& phys_device : phys_devices) {
        vkGetPhysicalDeviceProperties2(phys_device, &phys_device_properties);
        std::println("#{}: {}", phys_device_index, phys_device_properties.properties.deviceName);

        if (phys_device_count > 1 && phys_device_properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            selected_phys_device_index = phys_device_index;
        }

        phys_device_index++;
    }
    std::println("selected physical device #{}", selected_phys_device_index);
    device.m_vk_phys_device = phys_devices[selected_phys_device_index];
    
    std::println("enumerating queue families...");
    Uint32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device.m_vk_phys_device, &queue_family_count, nullptr);
    
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device.m_vk_phys_device, &queue_family_count, queue_families.data());
    
    Sint32 selected_queue_family_index = -1;
    for (Sint32 i = 0; i < queue_families.size(); i++) {
        if (queue_families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
            selected_queue_family_index = i;
            break;
        }
    }

    if (selected_queue_family_index < 0) {
        std::println("failed to find matching queue family");
        return { Device(), false };
    }

    if (!SDL_Vulkan_GetPresentationSupport(device.m_vk_instance, device.m_vk_phys_device, selected_phys_device_index)) {
        std::println("queue family does not support presentation");
        return { Device(), false };
    }

    std::println("selected queue familly with index {}", selected_queue_family_index);
    device.m_vk_queue_family_index = static_cast<Uint32>(selected_queue_family_index);

    const float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_CI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = device.m_vk_queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    const std::vector<const char*> device_extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkPhysicalDeviceVulkan12Features enabled_vk_12_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = true,
        .shaderSampledImageArrayNonUniformIndexing = true,
        .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
        .bufferDeviceAddress = true
    };
    VkPhysicalDeviceVulkan13Features enabled_vk_13_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &enabled_vk_12_features,
        .synchronization2 = true,
        .dynamicRendering = true,
    };
    VkPhysicalDeviceFeatures enabled_vk_10_features{
        .samplerAnisotropy = VK_TRUE
    };

    std::println("creating logical device...");
    VkDeviceCreateInfo device_CI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabled_vk_13_features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_CI,
        .enabledExtensionCount = static_cast<Uint32>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = &enabled_vk_10_features
    };
    if (vkCreateDevice(device.m_vk_phys_device, &device_CI, nullptr, &device.m_vk_device) != VK_SUCCESS) {
        std::println("failed to create logical device");
        return { Device(), false };
    }

    volkLoadDevice(device.m_vk_device);

    return { std::move(device), true };
}
