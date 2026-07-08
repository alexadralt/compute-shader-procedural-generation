#include "device.h"

#include <SDL3/SDL_vulkan.h>

#include <print>
#include <vector>
#include <array>
#include <algorithm>
#include <cstring>

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

std::vector<const char*> rdr::Device::get_validation_layers()
{
    Uint32 available_validation_layer_count = 0;
    if (vkEnumerateInstanceLayerProperties(&available_validation_layer_count, nullptr) != VK_SUCCESS) {
        std::println("failed to enumerate validation layers");
        return std::vector<const char *>();
    }

    std::vector<VkLayerProperties> available_validation_layers(available_validation_layer_count);
    if (vkEnumerateInstanceLayerProperties(&available_validation_layer_count, available_validation_layers.data()) != VK_SUCCESS) {
        std::println("failed to get validation layers");
        return std::vector<const char*>();
    }

    std::array<const char*, 1> requested_validation_layers{
        "VK_LAYER_KHRONOS_validation",
    };

    std::vector<const char *> validation_layers_to_enable;
    validation_layers_to_enable.reserve(requested_validation_layers.size());

    std::vector<const char*> unavailable_validation_layers;
    unavailable_validation_layers.reserve(requested_validation_layers.size());

    Sint32 validation_layer_index = 0;
    for (const char *layer_name : requested_validation_layers) {
        if (std::ranges::any_of(available_validation_layers, [layer_name](const VkLayerProperties& props) { return strcmp(layer_name, props.layerName) == 0; })) {
            validation_layers_to_enable.push_back(layer_name);
        }
        else {
            unavailable_validation_layers.push_back(layer_name);
        }
    }

    if (unavailable_validation_layers.size() > 0) {
        std::println("following validation layers are unavailable:");
        Sint32 layer_index = 0;
        for (const char* layer_name : unavailable_validation_layers) {
            std::println("#{}: {}", layer_index++, layer_name);
        }
    }

    return validation_layers_to_enable;
}

bool rdr::Device::create(Device& out_device)
{
    std::println("calling SLD_Vulkan_LoadLibrary()...");
    if (!SDL_Vulkan_LoadLibrary(nullptr)) {
        std::println("failed to call SLD_Vulkan_LoadLibrary()");
        return false;
    }

    std::println("initializing volk...");
    if (volkInitialize() != VK_SUCCESS) {
        std::println("failed to initialize volk");
        return false;
    }

    Device device;

    VkApplicationInfo app_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Making mother of all omelettes here Jack",
        .apiVersion = VK_API_VERSION_1_4,
    };

    Uint32 instance_extensions_count = 0;
    char const* const* instance_extensions = SDL_Vulkan_GetInstanceExtensions(&instance_extensions_count);

    auto validation_layers_to_enable = get_validation_layers();
    if (validation_layers_to_enable.size() > 0) {
        std::println("enabling following validation layers:");

        Sint32 layer_index = 0;
        for (const char* layer_name : validation_layers_to_enable) {
            std::println("#{}: {}", layer_index++, layer_name);
        }
    }

    VkInstanceCreateInfo instance_CI {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<Uint32>(validation_layers_to_enable.size()),
        .ppEnabledLayerNames = validation_layers_to_enable.data(),
        .enabledExtensionCount = instance_extensions_count,
        .ppEnabledExtensionNames = instance_extensions,
    };

    std::println("creating vulkan instance...");
    if (vkCreateInstance(&instance_CI, nullptr, &device.m_vk_instance) != VK_SUCCESS) {
        std::println("failed to create vulkan instance");
        return false;
    }

    volkLoadInstance(device.m_vk_instance);

    std::println("enumerating physical devices...");
    Uint32 phys_device_count = 0;
    if (vkEnumeratePhysicalDevices(device.m_vk_instance, &phys_device_count, nullptr) != VK_SUCCESS) {
        std::println("failed to get count of physical devices");
        return false;
    }

    if (phys_device_count == 0) {
        std::println("found 0 physical devices");
        return false;
    }
    
    std::vector<VkPhysicalDevice> phys_devices(phys_device_count);
    if (vkEnumeratePhysicalDevices(device.m_vk_instance, &phys_device_count, phys_devices.data()) != VK_SUCCESS) {
        std::println("failed to get list of physical devices");
        return false;
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
        return false;
    }

    if (!SDL_Vulkan_GetPresentationSupport(device.m_vk_instance, device.m_vk_phys_device, selected_phys_device_index)) {
        std::println("queue family does not support presentation");
        return false;
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

    const std::array<const char*, 1> device_extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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
        .samplerAnisotropy = true,
        .shaderInt64 = true,
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
        return false;
    }

    volkLoadDevice(device.m_vk_device);

    out_device = std::move(device);
    return true;
}

rdr::Queue rdr::Device::get_device_queue(uint32_t queue_index) const
{
    VkQueue queue;
    vkGetDeviceQueue(m_vk_device, m_vk_queue_family_index, queue_index, &queue);
    return Queue(queue);
}
