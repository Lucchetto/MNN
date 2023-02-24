//
//  VulkanInstance.cpp
//  MNN
//
//  Created by MNN on 2019/01/31.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#include "backend/vulkan/component/VulkanInstance.hpp"
#include <vector>

namespace MNN {
VulkanInstance::VulkanInstance() : mOwner(true), mInstance(VK_NULL_HANDLE) {
    VkApplicationInfo appInfo = {
        /* .sType              = */ VK_STRUCTURE_TYPE_APPLICATION_INFO,
        /* .pNext              = */ nullptr,
        /* .pApplicationName   = */ "MNN_Vulkan",
        /* .applicationVersion = */ VK_MAKE_VERSION(1, 0, 0),
        /* .pEngineName        = */ "Compute",
        /* .engineVersion      = */ VK_MAKE_VERSION(1, 0, 0),
        /* .apiVersion         = */ VK_MAKE_VERSION(1, 0, 0),
    };
    std::vector<const char*> instance_extensions;
#ifdef MNN_VULKAN_DEBUG
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
#endif
    // Create the Vulkan instance
    VkInstanceCreateInfo instanceCreateInfo{
        /* .sType                   = */ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        /* .pNext                   = */ nullptr,
        /* .flags                   = */ 0,
        /* .pApplicationInfo        = */ &appInfo,
#ifdef MNN_VULKAN_DEBUG
        /* .enabledLayerCount       = */ 1,
        /* .ppEnabledLayerNames     = */ validationLayers.data(),
#else
        /* .enabledLayerCount       = */ 0,
        /* .ppEnabledLayerNames     = */ nullptr,
#endif
        /* .enabledExtensionCount   = */ static_cast<uint32_t>(instance_extensions.size()),
        /* .ppEnabledExtensionNames = */ instance_extensions.data(),
    };
    CALL_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &mInstance));
}
VulkanInstance::VulkanInstance(VkInstance instance) : mOwner(false), mInstance(instance) {
}

VulkanInstance::~VulkanInstance() {
    if (mOwner && (VK_NULL_HANDLE != mInstance)) {
        vkDestroyInstance(mInstance, nullptr);
        mInstance = VK_NULL_HANDLE;
    }
}
const VkResult VulkanInstance::enumeratePhysicalDevices(uint32_t& physicalDeviceCount,
                                                        VkPhysicalDevice* physicalDevices) const {
    return vkEnumeratePhysicalDevices(get(), &physicalDeviceCount, physicalDevices);
}

void VulkanInstance::getPhysicalDeviceQueueFamilyProperties(const VkPhysicalDevice& physicalDevice,
                                                            uint32_t& queueFamilyPropertyCount,
                                                            VkQueueFamilyProperties* pQueueFamilyProperties) const {
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, pQueueFamilyProperties);
}

bool VulkanInstance::getPhysicalDeviceComputeQueueSupport(const VkPhysicalDevice& physicalDevice) const {
    // Get count of queues
    uint32_t queueCount = 0;
    getPhysicalDeviceQueueFamilyProperties(physicalDevice, queueCount, nullptr);
    if (queueCount == 0) {
        return false;
    }

    // Get actual queues
    VkQueueFamilyProperties queues[queueCount];
    getPhysicalDeviceQueueFamilyProperties(physicalDevice, queueCount, queues);

    // Check vulkan compute support
    for (uint32_t i = 0; i < queueCount; ++queueCount) {
        if ((queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            return true;
        }
    }

    return false;
}

const bool VulkanInstance::supportVulkan() const {
    uint32_t gpuCount = 1;
    VkPhysicalDevice vulkanDevices[1];
    auto res          = enumeratePhysicalDevices(gpuCount, vulkanDevices);
    if ((0 == gpuCount) || (VK_SUCCESS != res) || !getPhysicalDeviceComputeQueueSupport(vulkanDevices[0])) {
        MNN_ERROR("Invalide device for support vulkan\n");
        return false;
    }
    return true;
}
} // namespace MNN
