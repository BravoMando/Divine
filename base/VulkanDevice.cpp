/*
 *
 ******************************************************************************
 *    Copyright [2024] [YongSong]
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 ******************************************************************************
 *
 */

#include "VulkanDevice.h"
#include "VulkanTools.h"
#include "VulkanInitializer.hpp"

#include <algorithm>
#include <unordered_set>

VulkanDevice::VulkanDevice(bool enableValidationLayer, const VkAllocationCallbacks *pAllocator)
{
    m_EnableValidationLayer = enableValidationLayer;
    p_Allocator = pAllocator;
}

VulkanDevice::~VulkanDevice()
{
    if (m_TransferCmdPool != VK_NULL_HANDLE)
    {
        DestroyCommandPool(m_TransferCmdPool);
    }
    if (m_Device != VK_NULL_HANDLE)
    {
        DestroyLogicalDevice();
    }
}

void VulkanDevice::InitDevice(VkInstance instance,
                              VkSurfaceKHR surface,
                              QueueTypeFlags queueType,
                              QueueFamilyIndices *pIndices,
                              std::vector<uint32_t> &uniqueIndices,
                              Queues *pQueues)
{
    if (instance == VK_NULL_HANDLE || surface == VK_NULL_HANDLE)
    {
        FATAL("Instance and Surface must be valid!");
    }
    if (pIndices == nullptr || pQueues == nullptr)
    {
        FATAL("The address must not be nullptr!");
    }
    if (queueType == QUEUE_TYPE_NONE)
    {
        FATAL("Queue type must not be none!");
    }

    std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    std::vector<const char *> deviceLayers = {};
    const char *pLayer = "VK_LAYER_KHRONOS_validation";
    if (m_EnableValidationLayer)
    {
        deviceLayers.push_back(pLayer);
    }

    // GPU
    VkBool32 presentSupport = VK_FALSE;
    uint32_t gpuCount = 0;
    CHECK_VK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, VK_NULL_HANDLE));
    if (gpuCount > 0)
    {
        INFO("GPU count: %d.\n", gpuCount);
        std::vector<VkPhysicalDevice> gpus(gpuCount);
        CHECK_VK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.data()));

        // Consider discrete GPU with different queue family index if there is one
        for (VkPhysicalDevice gpu : gpus)
        {
            VkPhysicalDeviceProperties gpuProps;
            vkGetPhysicalDeviceProperties(gpu, &gpuProps);
            INFO("GPU: %s found!\n", gpuProps.deviceName);

            // If there is a discrete GPU with different queue family index if there is one
            if (gpuProps.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                uint32_t queueCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, nullptr);
                if (queueCount > 0)
                {
                    std::vector<VkQueueFamilyProperties> queueFamilyProps(queueCount);
                    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, queueFamilyProps.data());
                    bool queueComplete = true;
                    uint32_t i = 0;
                    for (const auto &queueProps : queueFamilyProps)
                    {
                        // Compute queue family index
                        if ((queueType & QUEUE_TYPE_COMPUTE) == QUEUE_TYPE_COMPUTE)
                        {
                            if (!pIndices->ComputeHasValue)
                            {
                                if (queueProps.queueFlags & VK_QUEUE_COMPUTE_BIT)
                                {
                                    pIndices->ComputeHasValue = true;
                                    pIndices->Compute = i;
                                }
                            }
                            queueComplete = queueComplete && pIndices->ComputeHasValue;
                        }

                        // Graphics queue family index, different from compute queue index is preferred
                        if ((queueType & QUEUE_TYPE_GRAPHICS) == QUEUE_TYPE_GRAPHICS)
                        {
                            if (!pIndices->GraphicsHasValue)
                            {
                                if (queueProps.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                                {
                                    if (!pIndices->ComputeHasValue)
                                    {
                                        pIndices->GraphicsHasValue = true;
                                        pIndices->Graphics = i;
                                    }
                                    else
                                    {
                                        if (pIndices->Compute != i)
                                        {
                                            pIndices->GraphicsHasValue = true;
                                            pIndices->Graphics = i;
                                        }
                                    }
                                }
                            }
                            queueComplete = queueComplete && pIndices->GraphicsHasValue;
                        }

                        // Transfer queue family index, different from graphics queue index is preferred
                        if ((queueType & QUEUE_TYPE_TRANSFER) == QUEUE_TYPE_TRANSFER)
                        {
                            if (!pIndices->TransferHasValue)
                            {
                                if (queueProps.queueFlags & VK_QUEUE_TRANSFER_BIT)
                                {
                                    if (!pIndices->GraphicsHasValue)
                                    {
                                        pIndices->TransferHasValue = true;
                                        pIndices->Transfer = i;
                                    }
                                    else
                                    {
                                        if (pIndices->Graphics != i)
                                        {
                                            pIndices->TransferHasValue = true;
                                            pIndices->Transfer = i;
                                        }
                                    }
                                }
                            }
                            queueComplete = queueComplete && pIndices->TransferHasValue;
                        }

                        // Present queue family index
                        if ((queueType & QUEUE_TYPE_PRESENT) == QUEUE_TYPE_PRESENT)
                        {
                            if (!pIndices->PresentHasValue)
                            {
                                CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &presentSupport));
                                if (presentSupport == VK_TRUE)
                                {
                                    pIndices->PresentHasValue = true;
                                    pIndices->Present = i;
                                }
                            }
                            queueComplete = queueComplete && pIndices->PresentHasValue;
                        }

                        if (queueComplete)
                        {
                            break;
                        }
                        ++i;
                    }

                    bool HaveFound = true;
                    if (queueComplete)
                    {
                        // Check extension support
                        uint32_t extensionCount = 0;
                        CHECK_VK_RESULT(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr));
                        if (extensionCount > 0)
                        {
                            std::vector<VkExtensionProperties> extensions(extensionCount);
                            CHECK_VK_RESULT(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, extensions.data()));
                            for (const auto &extensionProps : extensions)
                            {
                                m_SupportedExtensions.push_back(std::string(extensionProps.extensionName));
                            }

                            if (m_EnabledExtensions.size() > 0)
                            {
                                for (const auto &enabledExtension : m_EnabledExtensions)
                                {
                                    if (std::find(deviceExtensions.begin(), deviceExtensions.end(), enabledExtension.c_str()) == deviceExtensions.end())
                                    {
                                        deviceExtensions.push_back(enabledExtension.c_str());
                                    }
                                }
                            }

                            for (const char *deviceExtension : deviceExtensions)
                            {
                                if (std::find(m_SupportedExtensions.begin(), m_SupportedExtensions.end(), std::string(deviceExtension)) == m_SupportedExtensions.end())
                                {
                                    HaveFound = false;
                                    m_SupportedExtensions.clear();
                                    deviceExtensions.erase(deviceExtensions.begin() + 1, deviceExtensions.end());
                                    WARNING("%s required, but not available in device: %s\n", deviceExtension, gpuProps.deviceName);
                                    break;
                                }
                            }
                        }

                        if (m_EnableValidationLayer)
                        {
                            // Check layer support
                            uint32_t layerCount = 0;
                            CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(gpu, &layerCount, nullptr));
                            if (layerCount > 0)
                            {
                                std::vector<VkLayerProperties> layers(layerCount);
                                CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(gpu, &layerCount, layers.data()));
                                for (const auto &deviceLayer : layers)
                                {
                                    m_SupportedLayers.push_back(std::string(deviceLayer.layerName));
                                }

                                if (m_EnabledLayers.size() > 0)
                                {
                                    for (const auto &enabledLayer : m_EnabledLayers)
                                    {
                                        if (std::find(deviceLayers.begin(), deviceLayers.end(), enabledLayer.c_str()) == deviceLayers.end())
                                        {
                                            deviceLayers.push_back(enabledLayer.c_str());
                                        }
                                    }
                                }

                                for (const char *deviceLayer : deviceLayers)
                                {
                                    if (std::find(m_SupportedLayers.begin(), m_SupportedLayers.end(), std::string(deviceLayer)) == m_SupportedLayers.end())
                                    {
                                        HaveFound = false;
                                        m_SupportedLayers.clear();
                                        deviceLayers.erase(deviceLayers.begin() + 1, deviceLayers.end());
                                        WARNING("%s required, but not available in device: %s\n", deviceLayer, gpuProps.deviceName);
                                        break;
                                    }
                                }
                            }
                        }

                        if (HaveFound)
                        {
                            m_GPU = gpu;
                            vkGetPhysicalDeviceProperties(gpu, &m_GPUProperties);
                            vkGetPhysicalDeviceFeatures(gpu, &m_GPUFeatures);
                            for (const auto &queueProps : queueFamilyProps)
                            {
                                m_QueueFamilyProperties.push_back(queueProps);
                            }
                            INFO("GPU: %s selected!\n", gpuProps.deviceName);
                            break;
                        }

                    } // if (queueComplete)
                } // if (queueCount > 0)
            } // if (gpuProps.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        } // for (VkPhysicalDevice gpu : gpus)

        // Consider discrete GPU if there is one
        if (m_GPU == VK_NULL_HANDLE)
        {
            for (VkPhysicalDevice gpu : gpus)
            {
                VkPhysicalDeviceProperties gpuProps;
                vkGetPhysicalDeviceProperties(gpu, &gpuProps);

                // If there is a discrete GPU
                if (gpuProps.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    uint32_t queueCount = 0;
                    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, nullptr);
                    if (queueCount > 0)
                    {
                        std::vector<VkQueueFamilyProperties> queueFamilyProps(queueCount);
                        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, queueFamilyProps.data());
                        bool queueComplete = true;
                        uint32_t i = 0;
                        for (const auto &queueProps : queueFamilyProps)
                        {
                            // Compute queue family index
                            if ((queueType & QUEUE_TYPE_COMPUTE) == QUEUE_TYPE_COMPUTE)
                            {
                                if (!pIndices->ComputeHasValue)
                                {
                                    if (queueProps.queueFlags & VK_QUEUE_COMPUTE_BIT)
                                    {
                                        pIndices->ComputeHasValue = true;
                                        pIndices->Compute = i;
                                    }
                                }
                                queueComplete = queueComplete && pIndices->ComputeHasValue;
                            }

                            // Graphics queue family index
                            if ((queueType & QUEUE_TYPE_GRAPHICS) == QUEUE_TYPE_GRAPHICS)
                            {
                                if (!pIndices->GraphicsHasValue)
                                {
                                    if (queueProps.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                                    {
                                        pIndices->GraphicsHasValue = true;
                                        pIndices->Graphics = i;
                                    }
                                }
                                queueComplete = queueComplete && pIndices->GraphicsHasValue;
                            }

                            // Transfer queue family index
                            if ((queueType & QUEUE_TYPE_TRANSFER) == QUEUE_TYPE_TRANSFER)
                            {
                                if (!pIndices->TransferHasValue)
                                {
                                    if (queueProps.queueFlags & VK_QUEUE_TRANSFER_BIT)
                                    {
                                        pIndices->TransferHasValue = true;
                                        pIndices->Transfer = i;
                                    }
                                }
                                queueComplete = queueComplete && pIndices->TransferHasValue;
                            }

                            // Present queue family index
                            if ((queueType & QUEUE_TYPE_PRESENT) == QUEUE_TYPE_PRESENT)
                            {
                                if (!pIndices->PresentHasValue)
                                {
                                    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &presentSupport));
                                    if (presentSupport == VK_TRUE)
                                    {
                                        pIndices->PresentHasValue = true;
                                        pIndices->Present = i;
                                    }
                                }
                                queueComplete = queueComplete && pIndices->PresentHasValue;
                            }

                            if (queueComplete)
                            {
                                break;
                            }
                            ++i;
                        }

                        bool HaveFound = true;
                        if (queueComplete)
                        {
                            // Check extension support
                            uint32_t extensionCount = 0;
                            CHECK_VK_RESULT(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr));
                            if (extensionCount > 0)
                            {
                                std::vector<VkExtensionProperties> extensions(extensionCount);
                                CHECK_VK_RESULT(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, extensions.data()));
                                for (const auto &extensionProps : extensions)
                                {
                                    m_SupportedExtensions.push_back(std::string(extensionProps.extensionName));
                                }

                                if (m_EnabledExtensions.size() > 0)
                                {
                                    for (const auto &enabledExtension : m_EnabledExtensions)
                                    {
                                        if (std::find(deviceExtensions.begin(), deviceExtensions.end(), enabledExtension.c_str()) == deviceExtensions.end())
                                        {
                                            deviceExtensions.push_back(enabledExtension.c_str());
                                        }
                                    }
                                }

                                for (const char *deviceExtension : deviceExtensions)
                                {
                                    if (std::find(m_SupportedExtensions.begin(), m_SupportedExtensions.end(), std::string(deviceExtension)) == m_SupportedExtensions.end())
                                    {
                                        HaveFound = false;
                                        m_SupportedExtensions.clear();
                                        deviceExtensions.erase(deviceExtensions.begin() + 1, deviceExtensions.end());
                                        WARNING("%s required, but not available in device: %s\n", deviceExtension, gpuProps.deviceName);
                                        break;
                                    }
                                }
                            }

                            if (m_EnableValidationLayer)
                            {
                                // Check layer support
                                uint32_t layerCount = 0;
                                CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(gpu, &layerCount, nullptr));
                                if (layerCount > 0)
                                {
                                    std::vector<VkLayerProperties> layers(layerCount);
                                    CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(gpu, &layerCount, layers.data()));
                                    for (const auto &deviceLayer : layers)
                                    {
                                        m_SupportedLayers.push_back(std::string(deviceLayer.layerName));
                                    }

                                    if (m_EnabledLayers.size() > 0)
                                    {
                                        for (const auto &enabledLayer : m_EnabledLayers)
                                        {
                                            if (std::find(deviceLayers.begin(), deviceLayers.end(), enabledLayer.c_str()) == deviceLayers.end())
                                            {
                                                deviceLayers.push_back(enabledLayer.c_str());
                                            }
                                        }
                                    }

                                    for (const char *deviceLayer : deviceLayers)
                                    {
                                        if (std::find(m_SupportedLayers.begin(), m_SupportedLayers.end(), std::string(deviceLayer)) == m_SupportedLayers.end())
                                        {
                                            HaveFound = false;
                                            m_SupportedLayers.clear();
                                            deviceLayers.erase(deviceLayers.begin() + 1, deviceLayers.end());
                                            WARNING("%s required, but not available in device: %s\n", deviceLayer, gpuProps.deviceName);
                                            break;
                                        }
                                    }
                                }
                            }

                            if (HaveFound)
                            {
                                m_GPU = gpu;
                                vkGetPhysicalDeviceProperties(gpu, &m_GPUProperties);
                                vkGetPhysicalDeviceFeatures(gpu, &m_GPUFeatures);
                                for (const auto &queueProps : queueFamilyProps)
                                {
                                    m_QueueFamilyProperties.push_back(queueProps);
                                }
                                INFO("GPU: %s selected!\n", gpuProps.deviceName);
                                break;
                            }

                        } // if (queueComplete)
                    } // if (queueCount > 0)
                } // if (gpuProps.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            } // for (VkPhysicalDevice gpu : gpus)
        } // if (m_GPU == VK_NULL_HANDLE)

        // If no discrete GPU selected
        if (m_GPU == VK_NULL_HANDLE)
        {
            for (VkPhysicalDevice gpu : gpus)
            {
                VkPhysicalDeviceProperties gpuProps;
                vkGetPhysicalDeviceProperties(gpu, &gpuProps);

                uint32_t queueCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, nullptr);
                if (queueCount > 0)
                {
                    std::vector<VkQueueFamilyProperties> queueFamilyProps(queueCount);
                    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, queueFamilyProps.data());
                    bool queueComplete = true;
                    uint32_t i = 0;
                    for (const auto &queueProps : queueFamilyProps)
                    {
                        // Compute queue family index
                        if ((queueType & QUEUE_TYPE_COMPUTE) == QUEUE_TYPE_COMPUTE)
                        {
                            if (!pIndices->ComputeHasValue)
                            {
                                if (queueProps.queueFlags & VK_QUEUE_COMPUTE_BIT)
                                {
                                    pIndices->ComputeHasValue = true;
                                    pIndices->Compute = i;
                                }
                            }
                            queueComplete = queueComplete && pIndices->ComputeHasValue;
                        }

                        // Graphics queue family index, different from compute queue index is preferred
                        if ((queueType & QUEUE_TYPE_GRAPHICS) == QUEUE_TYPE_GRAPHICS)
                        {
                            if (!pIndices->GraphicsHasValue)
                            {
                                if (queueProps.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                                {
                                    if (!pIndices->ComputeHasValue)
                                    {
                                        pIndices->GraphicsHasValue = true;
                                        pIndices->Graphics = i;
                                    }
                                    else
                                    {
                                        if (pIndices->Compute != i)
                                        {
                                            pIndices->GraphicsHasValue = true;
                                            pIndices->Graphics = i;
                                        }
                                    }
                                }
                            }
                            queueComplete = queueComplete && pIndices->GraphicsHasValue;
                        }

                        // Transfer queue family index, different from graphics queue index is preferred
                        if ((queueType & QUEUE_TYPE_TRANSFER) == QUEUE_TYPE_TRANSFER)
                        {
                            if (!pIndices->TransferHasValue)
                            {
                                if (queueProps.queueFlags & VK_QUEUE_TRANSFER_BIT)
                                {
                                    if (!pIndices->GraphicsHasValue)
                                    {
                                        pIndices->TransferHasValue = true;
                                        pIndices->Transfer = i;
                                    }
                                    else
                                    {
                                        if (pIndices->Graphics != i)
                                        {
                                            pIndices->TransferHasValue = true;
                                            pIndices->Transfer = i;
                                        }
                                    }
                                }
                            }
                            queueComplete = queueComplete && pIndices->TransferHasValue;
                        }

                        // Present queue family index
                        if ((queueType & QUEUE_TYPE_PRESENT) == QUEUE_TYPE_PRESENT)
                        {
                            if (!pIndices->PresentHasValue)
                            {
                                CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &presentSupport));
                                if (presentSupport == VK_TRUE)
                                {
                                    pIndices->PresentHasValue = true;
                                    pIndices->Present = i;
                                }
                            }
                            queueComplete = queueComplete && pIndices->PresentHasValue;
                        }

                        if (queueComplete)
                        {
                            break;
                        }
                        ++i;
                    }

                    bool HaveFound = true;
                    if (queueComplete)
                    {
                        // Check extension support
                        uint32_t extensionCount = 0;
                        CHECK_VK_RESULT(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr));
                        if (extensionCount > 0)
                        {
                            std::vector<VkExtensionProperties> extensions(extensionCount);
                            CHECK_VK_RESULT(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, extensions.data()));
                            for (const auto &extensionProps : extensions)
                            {
                                m_SupportedExtensions.push_back(std::string(extensionProps.extensionName));
                            }

                            if (m_EnabledExtensions.size() > 0)
                            {
                                for (const auto &enabledExtension : m_EnabledExtensions)
                                {
                                    if (std::find(deviceExtensions.begin(), deviceExtensions.end(), enabledExtension.c_str()) == deviceExtensions.end())
                                    {
                                        deviceExtensions.push_back(enabledExtension.c_str());
                                    }
                                }
                            }

                            for (const char *deviceExtension : deviceExtensions)
                            {
                                if (std::find(m_SupportedExtensions.begin(), m_SupportedExtensions.end(), std::string(deviceExtension)) == m_SupportedExtensions.end())
                                {
                                    HaveFound = false;
                                    m_SupportedExtensions.clear();
                                    deviceExtensions.erase(deviceExtensions.begin() + 1, deviceExtensions.end());
                                    WARNING("%s required, but not available in device: %s\n", deviceExtension, gpuProps.deviceName);
                                    break;
                                }
                            }
                        }

                        if (m_EnableValidationLayer)
                        {
                            // Check layer support
                            uint32_t layerCount = 0;
                            CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(gpu, &layerCount, nullptr));
                            if (layerCount > 0)
                            {
                                std::vector<VkLayerProperties> layers(layerCount);
                                CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(gpu, &layerCount, layers.data()));
                                for (const auto &deviceLayer : layers)
                                {
                                    m_SupportedLayers.push_back(std::string(deviceLayer.layerName));
                                }

                                if (m_EnabledLayers.size() > 0)
                                {
                                    for (const auto &enabledLayer : m_EnabledLayers)
                                    {
                                        if (std::find(deviceLayers.begin(), deviceLayers.end(), enabledLayer.c_str()) == deviceLayers.end())
                                        {
                                            deviceLayers.push_back(enabledLayer.c_str());
                                        }
                                    }
                                }

                                for (const char *deviceLayer : deviceLayers)
                                {
                                    if (std::find(m_SupportedLayers.begin(), m_SupportedLayers.end(), std::string(deviceLayer)) == m_SupportedLayers.end())
                                    {
                                        HaveFound = false;
                                        m_SupportedLayers.clear();
                                        deviceLayers.erase(deviceLayers.begin() + 1, deviceLayers.end());
                                        WARNING("%s required, but not available in device: %s\n", deviceLayer, gpuProps.deviceName);
                                        break;
                                    }
                                }
                            }
                        }

                        if (HaveFound)
                        {
                            m_GPU = gpu;
                            vkGetPhysicalDeviceProperties(gpu, &m_GPUProperties);
                            vkGetPhysicalDeviceFeatures(gpu, &m_GPUFeatures);
                            for (const auto &queueProps : queueFamilyProps)
                            {
                                m_QueueFamilyProperties.push_back(queueProps);
                            }
                            INFO("GPU: %s selected!\n", gpuProps.deviceName);
                            break;
                        }

                    } // if (queueComplete)
                } // if (queueCount > 0)
            } // for (VkPhysicalDevice gpu : gpus)
        } // if (m_GPU == VK_NULL_HANDLE)

        // If queue family index are not seperate
        if (m_GPU == VK_NULL_HANDLE)
        {
            for (VkPhysicalDevice gpu : gpus)
            {
                VkPhysicalDeviceProperties gpuProps;
                vkGetPhysicalDeviceProperties(gpu, &gpuProps);

                uint32_t queueCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, nullptr);
                if (queueCount > 0)
                {
                    std::vector<VkQueueFamilyProperties> queueFamilyProps(queueCount);
                    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, queueFamilyProps.data());
                    bool queueComplete = true;
                    uint32_t i = 0;
                    for (const auto &queueProps : queueFamilyProps)
                    {
                        // Compute queue family index
                        if ((queueType & QUEUE_TYPE_COMPUTE) == QUEUE_TYPE_COMPUTE)
                        {
                            if (!pIndices->ComputeHasValue)
                            {
                                if (queueProps.queueFlags & VK_QUEUE_COMPUTE_BIT)
                                {
                                    pIndices->ComputeHasValue = true;
                                    pIndices->Compute = i;
                                }
                            }
                            queueComplete = queueComplete && pIndices->ComputeHasValue;
                        }

                        // Graphics queue family index
                        if ((queueType & QUEUE_TYPE_GRAPHICS) == QUEUE_TYPE_GRAPHICS)
                        {
                            if (!pIndices->GraphicsHasValue)
                            {
                                if (queueProps.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                                {
                                    pIndices->GraphicsHasValue = true;
                                    pIndices->Graphics = i;
                                }
                            }
                            queueComplete = queueComplete && pIndices->GraphicsHasValue;
                        }

                        // Transfer queue family index
                        if ((queueType & QUEUE_TYPE_TRANSFER) == QUEUE_TYPE_TRANSFER)
                        {
                            if (!pIndices->TransferHasValue)
                            {
                                if (queueProps.queueFlags & VK_QUEUE_TRANSFER_BIT)
                                {
                                    pIndices->TransferHasValue = true;
                                    pIndices->Transfer = i;
                                }
                            }
                            queueComplete = queueComplete && pIndices->TransferHasValue;
                        }

                        // Present queue family index
                        if ((queueType & QUEUE_TYPE_PRESENT) == QUEUE_TYPE_PRESENT)
                        {
                            if (!pIndices->PresentHasValue)
                            {
                                CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &presentSupport));
                                if (presentSupport == VK_TRUE)
                                {
                                    pIndices->PresentHasValue = true;
                                    pIndices->Present = i;
                                }
                            }
                            queueComplete = queueComplete && pIndices->PresentHasValue;
                        }

                        if (queueComplete)
                        {
                            break;
                        }
                        ++i;
                    }

                    bool HaveFound = true;
                    if (queueComplete)
                    {
                        // Check extension support
                        uint32_t extensionCount = 0;
                        CHECK_VK_RESULT(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr));
                        if (extensionCount > 0)
                        {
                            std::vector<VkExtensionProperties> extensions(extensionCount);
                            CHECK_VK_RESULT(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, extensions.data()));
                            for (const auto &extensionProps : extensions)
                            {
                                m_SupportedExtensions.push_back(std::string(extensionProps.extensionName));
                            }

                            if (m_EnabledExtensions.size() > 0)
                            {
                                for (const auto &enabledExtension : m_EnabledExtensions)
                                {
                                    if (std::find(deviceExtensions.begin(), deviceExtensions.end(), enabledExtension.c_str()) == deviceExtensions.end())
                                    {
                                        deviceExtensions.push_back(enabledExtension.c_str());
                                    }
                                }
                            }

                            for (const char *deviceExtension : deviceExtensions)
                            {
                                if (std::find(m_SupportedExtensions.begin(), m_SupportedExtensions.end(), std::string(deviceExtension)) == m_SupportedExtensions.end())
                                {
                                    HaveFound = false;
                                    m_SupportedExtensions.clear();
                                    deviceExtensions.erase(deviceExtensions.begin() + 1, deviceExtensions.end());
                                    WARNING("%s required, but not available in device: %s\n", deviceExtension, gpuProps.deviceName);
                                    break;
                                }
                            }
                        }

                        if (m_EnableValidationLayer)
                        {
                            // Check layer support
                            uint32_t layerCount = 0;
                            CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(gpu, &layerCount, nullptr));
                            if (layerCount > 0)
                            {
                                std::vector<VkLayerProperties> layers(layerCount);
                                CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(gpu, &layerCount, layers.data()));
                                for (const auto &deviceLayer : layers)
                                {
                                    m_SupportedLayers.push_back(std::string(deviceLayer.layerName));
                                }

                                if (m_EnabledLayers.size() > 0)
                                {
                                    for (const auto &enabledLayer : m_EnabledLayers)
                                    {
                                        if (std::find(deviceLayers.begin(), deviceLayers.end(), enabledLayer.c_str()) == deviceLayers.end())
                                        {
                                            deviceLayers.push_back(enabledLayer.c_str());
                                        }
                                    }
                                }

                                for (const char *deviceLayer : deviceLayers)
                                {
                                    if (std::find(m_SupportedLayers.begin(), m_SupportedLayers.end(), std::string(deviceLayer)) == m_SupportedLayers.end())
                                    {
                                        HaveFound = false;
                                        m_SupportedLayers.clear();
                                        deviceLayers.erase(deviceLayers.begin() + 1, deviceLayers.end());
                                        WARNING("%s required, but not available in device: %s\n", deviceLayer, gpuProps.deviceName);
                                        break;
                                    }
                                }
                            }
                        }

                        if (HaveFound)
                        {
                            m_GPU = gpu;
                            vkGetPhysicalDeviceProperties(gpu, &m_GPUProperties);
                            vkGetPhysicalDeviceFeatures(gpu, &m_GPUFeatures);
                            for (const auto &queueProps : queueFamilyProps)
                            {
                                m_QueueFamilyProperties.push_back(queueProps);
                            }
                            INFO("GPU: %s selected!\n", gpuProps.deviceName);
                            break;
                        }

                    } // if (queueComplete)
                } // if (queueCount > 0)
            } // for (VkPhysicalDevice gpu : gpus)
        } // if (m_GPU == VK_NULL_HANDLE)

        // If no GPU selected
        if (m_GPU == VK_NULL_HANDLE)
        {
            FATAL("No GPU was selected!");
        }
    }
    else
    {
        FATAL("GPU count is 0!");
    }

    if (ExtensionSupport(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME) && API_VERSION > VK_API_VERSION_1_0)
    {
        VkPhysicalDeviceProperties2 GPUProperties2{};
        GPUProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        GPUProperties2.pNext = &m_GPUDriverProperties;
        m_GPUDriverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
        vkGetPhysicalDeviceProperties2(m_GPU, &GPUProperties2);
    }

    // Logical device
    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> deviceQueueInfos;
    std::unordered_set<uint32_t> uniqueQueueIndices = {pIndices->Compute,
                                                       pIndices->Graphics,
                                                       pIndices->Transfer,
                                                       pIndices->Present};
    uniqueIndices.clear();
    for (const auto &index : uniqueQueueIndices)
    {
        VkDeviceQueueCreateInfo deviceQueueInfo = vkinfo::DeviceQueueInfo();
        deviceQueueInfo.queueCount = 1;
        deviceQueueInfo.queueFamilyIndex = index;
        deviceQueueInfo.pQueuePriorities = &queuePriority;

        deviceQueueInfos.push_back(deviceQueueInfo);
        uniqueIndices.push_back(index);
    }

    VkDeviceCreateInfo deviceCI = vkinfo::DeviceInfo();
    deviceCI.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCI.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCI.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueInfos.size());
    deviceCI.pQueueCreateInfos = deviceQueueInfos.data();
    deviceCI.pEnabledFeatures = &m_GPUFeatures;
    if (m_EnableValidationLayer)
    {
        deviceCI.enabledLayerCount = static_cast<uint32_t>(deviceLayers.size());
        deviceCI.ppEnabledLayerNames = deviceLayers.data();
    }
    CHECK_VK_RESULT(vkCreateDevice(m_GPU, &deviceCI, p_Allocator, &m_Device));
    INFO("Device %p is created with %u extensions %u layers!\n", m_Device, deviceCI.enabledExtensionCount, deviceCI.enabledLayerCount);

    vkGetDeviceQueue(m_Device, pIndices->Compute, 0, &pQueues->Compute);
    vkGetDeviceQueue(m_Device, pIndices->Graphics, 0, &pQueues->Graphics);
    vkGetDeviceQueue(m_Device, pIndices->Transfer, 0, &pQueues->Transfer);
    vkGetDeviceQueue(m_Device, pIndices->Present, 0, &pQueues->Present);

    p_QueueFamilyIndices = pIndices;
    p_UniqueQueueFamilyIndices = uniqueIndices.data();
    m_UniqueQueueFamilyIndexCount = static_cast<uint32_t>(uniqueIndices.size());
    p_Queues = pQueues;

    if (queueType & QUEUE_TYPE_COMPUTE)
    {
        INFO("Compute queue family index: %d.\n", pIndices->Compute);
    }
    if (queueType & QUEUE_TYPE_GRAPHICS)
    {
        INFO("Graphics queue family index: %d.\n", pIndices->Graphics);
    }
    if (queueType & QUEUE_TYPE_TRANSFER)
    {
        INFO("Transfer queue family index: %d.\n", pIndices->Transfer);
    }
    if (queueType & QUEUE_TYPE_PRESENT)
    {
        INFO("Present queue family index: %d.\n", pIndices->Present);
    }

    m_TransferCmdPool = CreateCommandPool(p_QueueFamilyIndices->Transfer, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    INFO("Transfer command pool %p created!\n", m_TransferCmdPool);
}

VkCommandPool VulkanDevice::CreateCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("No valid device!");
    }

    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo poolCI = vkinfo::CommandPoolInfo(queueFamilyIndex, flags);
    CHECK_VK_RESULT(vkCreateCommandPool(m_Device, &poolCI, p_Allocator, &cmdPool));
    if (cmdPool != VK_NULL_HANDLE)
    {
        return cmdPool;
    }
    return VK_NULL_HANDLE;
}

void VulkanDevice::AllocateCommandBuffers(VkCommandPool pool, VkCommandBufferLevel level, uint32_t bufferCount, VkCommandBuffer *pBuffer)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("No valid device!");
    }
    if (pool == VK_NULL_HANDLE)
    {
        FATAL("Command buffer must be allocated in a valid command pool!");
    }
    if (pBuffer == nullptr)
    {
        FATAL("The address must be valid!");
    }

    VkCommandBufferAllocateInfo allocInfo = vkinfo::CommandBufferAllocteInfo(pool, level, bufferCount);
    CHECK_VK_RESULT(vkAllocateCommandBuffers(m_Device, &allocInfo, pBuffer));
}

void VulkanDevice::CreateBuffer(VkDeviceSize size,
                                VkBufferUsageFlags usage,
                                VkMemoryPropertyFlags properties,
                                VulkanBuffer *pBuffer,
                                const void *data)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("No valid device!");
    }

    VkBufferCreateInfo bufferInfo = vkinfo::BufferInfo(size, usage);
    CHECK_VK_RESULT(vkCreateBuffer(m_Device, &bufferInfo, p_Allocator, &pBuffer->Buffer));

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(m_Device, pBuffer->Buffer, &requirements);

    uint32_t idx = FindMemoryTypeIndex(m_GPU, requirements.memoryTypeBits, properties);
    VkMemoryAllocateInfo allocInfo = vkinfo::MemoryAllocInfo(requirements.size, idx);
    // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT flag, we need to enable the appropriate flag during allocation
    VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        allocInfo.pNext = &allocFlagsInfo;
    }
    CHECK_VK_RESULT(vkAllocateMemory(m_Device, &allocInfo, p_Allocator, &pBuffer->Memory));

    pBuffer->Device = m_Device;
    pBuffer->IsInitialized = true;
    pBuffer->Allocator = p_Allocator;
    pBuffer->Size = size;
    pBuffer->Alignment = requirements.alignment;
    pBuffer->Usage = usage;
    pBuffer->MemoryProperty = properties;

    if (data != nullptr)
    {
        pBuffer->Map();
        pBuffer->CopyData(data, size);
        if ((properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
        {
            pBuffer->Flush();
        }
        pBuffer->Unmap();
    }
    pBuffer->SetDescriptorBuffer();
    pBuffer->Bind();
}

VkCommandBuffer VulkanDevice::CreateCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin)
{
    VkCommandBuffer cmdBuffer;
    AllocateCommandBuffers(pool, level, 1, &cmdBuffer);
    // If requested, also start recording for the new command buffer
    if (begin)
    {
        VkCommandBufferBeginInfo cmdBufInfo = vkinfo::CommandBufferBeginInfo();
        CHECK_VK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
    }
    return cmdBuffer;
}

VkCommandBuffer VulkanDevice::CreateCommandBuffer(VkCommandBufferLevel level, bool begin)
{
    return CreateCommandBuffer(level, m_TransferCmdPool, begin);
}

void VulkanDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free)
{
    if (commandBuffer == VK_NULL_HANDLE)
    {
        return;
    }

    CHECK_VK_RESULT(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = vkinfo::SubmitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceInfo = vkinfo::FenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkFence fence;
    CHECK_VK_RESULT(vkCreateFence(m_Device, &fenceInfo, p_Allocator, &fence));
    // Reset fence
    CHECK_VK_RESULT(vkResetFences(m_Device, 1, &fence));
    // Submit to the queue
    CHECK_VK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
    // Wait for the fence to signal that command buffer has finished executing
    CHECK_VK_RESULT(vkWaitForFences(m_Device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
    vkDestroyFence(m_Device, fence, p_Allocator);
    if (free)
    {
        vkFreeCommandBuffers(m_Device, pool, 1, &commandBuffer);
    }
}

void VulkanDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
    FlushCommandBuffer(commandBuffer, queue, m_TransferCmdPool, free);
}

void VulkanDevice::CopyBuffer(VulkanBuffer *src, VulkanBuffer *dst, VkQueue queue, VkBufferCopy *copyRegin)
{
    if (dst->Size < src->Size)
    {
        FATAL("Destination buffer size must not less than source buffer size!");
    }
    if (src->Buffer == VK_NULL_HANDLE)
    {
        FATAL("The src buffer is not initialized!");
    }
    if (dst->Buffer == VK_NULL_HANDLE)
    {
        FATAL("The dst buffer is not initialized!");
    }

    VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    VkBufferCopy bufferCopy{};
    if (copyRegin == nullptr)
    {
        bufferCopy.size = src->Size;
    }
    else
    {
        bufferCopy = *copyRegin;
    }

    vkCmdCopyBuffer(cmdBuffer, src->Buffer, dst->Buffer, 1, &bufferCopy);

    FlushCommandBuffer(cmdBuffer, queue);
}

void VulkanDevice::CopyBuffer(VulkanBuffer *src, VulkanBuffer *dst, VkBufferCopy *copyRegin)
{
    if (p_QueueFamilyIndices->TransferHasValue || p_QueueFamilyIndices->GraphicsHasValue)
    {
        if (dst->Size < src->Size)
        {
            FATAL("Destination buffer size must not less than source buffer size!");
        }
        if (src->Buffer == VK_NULL_HANDLE)
        {
            FATAL("The src buffer is not initialized!");
        }
        if (dst->Buffer == VK_NULL_HANDLE)
        {
            FATAL("The dst buffer is not initialized!");
        }

        VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkBufferCopy bufferCopy{};
        if (copyRegin == nullptr)
        {
            bufferCopy.size = src->Size;
        }
        else
        {
            bufferCopy = *copyRegin;
        }

        vkCmdCopyBuffer(cmdBuffer, src->Buffer, dst->Buffer, 1, &bufferCopy);

        if (p_QueueFamilyIndices->TransferHasValue)
        {
            FlushCommandBuffer(cmdBuffer, p_Queues->Transfer);
        }
        else
        {
            FlushCommandBuffer(cmdBuffer, p_Queues->Graphics);
        }
    }
    else
    {
        FATAL("It seems neither the transfer queue nor the graphics queue are enabled when initializing device!");
    }
}

void VulkanDevice::CopyBufferToImage(VulkanBuffer *src,
                                     VulkanTexture *dst,
                                     VkQueue queue,
                                     uint32_t width,
                                     uint32_t height,
                                     uint32_t copyReginCount,
                                     VkBufferImageCopy *pCopyRegins)
{
    if (src->Buffer == VK_NULL_HANDLE)
    {
        FATAL("The src buffer is not initialized!");
    }
    if (dst->Image == VK_NULL_HANDLE)
    {
        FATAL("The dst image is not initialized!");
    }

    VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    if (copyReginCount == 0)
    {
        VkBufferImageCopy bufferCopy{};
        bufferCopy.imageExtent.width = width;
        bufferCopy.imageExtent.height = height;
        bufferCopy.imageExtent.depth = 1U;
        bufferCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopy.imageSubresource.baseArrayLayer = 0;
        bufferCopy.imageSubresource.layerCount = 1;
        bufferCopy.imageSubresource.mipLevel = 0;

        vkCmdCopyBufferToImage(cmdBuffer, src->Buffer, dst->Image, dst->Layout, 1, &bufferCopy);
        FlushCommandBuffer(cmdBuffer, queue);
    }
    else
    {
        vkCmdCopyBufferToImage(cmdBuffer, src->Buffer, dst->Image, dst->Layout, copyReginCount, pCopyRegins);
        FlushCommandBuffer(cmdBuffer, queue);
    }
}

void VulkanDevice::CopyBufferToImage(VulkanBuffer *src,
                                     VulkanTexture *dst,
                                     uint32_t width,
                                     uint32_t height,
                                     uint32_t copyReginCount,
                                     VkBufferImageCopy *pCopyRegins)
{
    if (p_QueueFamilyIndices->TransferHasValue || p_QueueFamilyIndices->GraphicsHasValue)
    {
        if (src->Buffer == VK_NULL_HANDLE)
        {
            FATAL("The src buffer is not initialized!");
        }
        if (dst->Image == VK_NULL_HANDLE)
        {
            FATAL("The dst image is not initialized!");
        }

        VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        if (copyReginCount == 0)
        {
            VkBufferImageCopy bufferCopy{};
            bufferCopy.imageExtent.width = width;
            bufferCopy.imageExtent.height = height;
            bufferCopy.imageExtent.depth = 1U;
            bufferCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopy.imageSubresource.baseArrayLayer = 0;
            bufferCopy.imageSubresource.layerCount = 1;
            bufferCopy.imageSubresource.mipLevel = 0;

            vkCmdCopyBufferToImage(cmdBuffer, src->Buffer, dst->Image, dst->Layout, 1, &bufferCopy);
            if (p_QueueFamilyIndices->TransferHasValue)
            {
                FlushCommandBuffer(cmdBuffer, p_Queues->Transfer);
            }
            else
            {
                FlushCommandBuffer(cmdBuffer, p_Queues->Graphics);
            }
        }
        else
        {
            vkCmdCopyBufferToImage(cmdBuffer, src->Buffer, dst->Image, dst->Layout, copyReginCount, pCopyRegins);
            if (p_QueueFamilyIndices->TransferHasValue)
            {
                FlushCommandBuffer(cmdBuffer, p_Queues->Transfer);
            }
            else
            {
                FlushCommandBuffer(cmdBuffer, p_Queues->Graphics);
            }
        }
    }
    else
    {
        FATAL("It seems neither the transfer queue nor the graphics queue are enabled when initializing device!");
    }
}

void VulkanDevice::DestroyCommandPool(VkCommandPool pool)
{
    vkDestroyCommandPool(m_Device, pool, p_Allocator);
}

void VulkanDevice::DestroyLogicalDevice()
{
    vkDestroyDevice(m_Device, p_Allocator);
    m_Device = VK_NULL_HANDLE;
}

bool VulkanDevice::ExtensionSupport(const std::string &extensionName)
{
    if (m_SupportedExtensions.size() != 0)
    {
        return !(std::find(m_SupportedExtensions.begin(), m_SupportedExtensions.end(), extensionName) == m_SupportedExtensions.end());
    }
    return false;
}

bool VulkanDevice::LayerSupport(const std::string &layerName)
{
    if (m_SupportedLayers.size() != 0)
    {
        return !(std::find(m_SupportedLayers.begin(), m_SupportedLayers.end(), layerName) == m_SupportedLayers.end());
    }
    return false;
}
