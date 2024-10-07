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

bool VulkanInstance::getPhysicalDeviceHasRequiredFeatures(const VkPhysicalDevice& physicalDevice) const {
    // Check for the physical device features
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    // We need shaderStorageImageWriteWithoutFormat support
    if (!deviceFeatures.shaderStorageImageWriteWithoutFormat) {
        return false;
    }

    // Check if there is a queue family that supports compute
    uint32_t queueFamilyCount = 0;
    getPhysicalDeviceQueueFamilyProperties(physicalDevice, queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    getPhysicalDeviceQueueFamilyProperties(physicalDevice, queueFamilyCount, queueFamilies.data());

    for (const auto &queueFamily: queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            // Device supports compute queue, we're good
            return true;
        }
    }

    // No compute queue found
    return false;
}

VkPhysicalDevice VulkanInstance::findSupportedVulkanDevice() const {
    uint32_t gpuCount = 0;
    if (enumeratePhysicalDevices(gpuCount, nullptr) != VK_SUCCESS || gpuCount < 1) {
        MNN_ERROR("Invalide device for support vulkan\n");
        return VK_NULL_HANDLE;
    }

    std::vector<VkPhysicalDevice> vulkanDevices(gpuCount);
    if (enumeratePhysicalDevices(gpuCount, vulkanDevices.data()) != VK_SUCCESS) {
        MNN_ERROR("Invalide device for support vulkan\n");
        return VK_NULL_HANDLE;
    }

    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
    for (int i = 0; i < vulkanDevices.size(); i++) {
        const auto &device = vulkanDevices[i];

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        if (!getPhysicalDeviceHasRequiredFeatures(device)) {
            // Skip unsupported GPU
            continue;
        } else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            // Prefer discrete GPU
            bestDevice = device;
            break;
        } else if (i == 0) {
            // Use first GPU as fallback when no discrete GPUs are found
            bestDevice = device;
        }
    }

    return bestDevice;
}
} // namespace MNN
