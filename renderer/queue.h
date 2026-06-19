#pragma once

#include "device.h"

#include <Volk/volk.h>

namespace rdr {
    class Queue {
        VkQueue m_vk_queue;

    public:
        Queue(const Device& device) {
            vkGetDeviceQueue(device.vk_device(), device.vk_queue_family_index(), 0, &m_vk_queue);
        }

        VkQueue vk_queue() const { return m_vk_queue };
    };
}
