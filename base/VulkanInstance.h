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

#ifndef VULKAN_INSTANCE_HEADER
#define VULKAN_INSTANCE_HEADER

#pragma once

#include "VulkanCore.h"
#include "vulkan/vulkan.h"

#include <string>
#include <vector>

class DVAPI_ATTR VulkanInstance final
{
private:
    bool m_EnableValidationLayer = true;
    const VkAllocationCallbacks *p_Allocator = nullptr;
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

public:
    explicit VulkanInstance(bool enableValidationLayer, const VkAllocationCallbacks *pAllocator = nullptr);
    ~VulkanInstance();
    VulkanInstance(const VulkanInstance &) = delete;
    VulkanInstance(VulkanInstance &&) = delete;
    VulkanInstance &operator=(const VulkanInstance &) = delete;
    VulkanInstance &operator=(VulkanInstance &&) = delete;

    std::vector<std::string> m_EnabledEextensions = {};
    std::vector<std::string> m_EnabledLayers = {};

    // Create instance
    void CreateInstance();
    // Create debug messenger
    void SetUpDebugMessenger();

    inline const VkInstance GetInstance() const { return m_Instance; }
    inline VkInstance GetInstance() { return m_Instance; }

    // Destroy debug messenger
    void DestroyDebugMessenger();
    // Destroy instance
    void DestroyInstance();

public:
    static std::vector<std::string> s_SupportedExtensions;
    static std::vector<std::string> s_SupportedLayers;

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData);
};

#endif