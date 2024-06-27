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

#include "VulkanInstance.h"
#include "VulkanTools.h"
#include "VulkanInitializer.hpp"
#include "GLFW/glfw3.h"

#include <algorithm>

std::vector<std::string> VulkanInstance::s_SupportedExtensions = {};
std::vector<std::string> VulkanInstance::s_SupportedLayers = {};

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    ERROR("[Validation Layer]: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

VulkanInstance::VulkanInstance(bool enableValidationLayer, const VkAllocationCallbacks *pAllocator)
{
    m_EnableValidationLayer = enableValidationLayer;
    if (m_EnableValidationLayer)
    {
        INFO("Validation layer is enabled!\n");
    }
    p_Allocator = pAllocator;
}

VulkanInstance::~VulkanInstance()
{
    if (m_DebugMessenger != VK_NULL_HANDLE)
    {
        DestroyDebugMessenger();
    }
    if (m_Instance != VK_NULL_HANDLE)
    {
        DestroyInstance();
    }
}

void VulkanInstance::CreateInstance()
{
    VkApplicationInfo appInfo = vkinfo::AppInfo();
    appInfo.pApplicationName = APP_NAME;
    appInfo.applicationVersion = APP_VERSION;
    appInfo.pEngineName = ENGINE_NAME;
    appInfo.engineVersion = ENGINE_VERSION;
    appInfo.apiVersion = API_VERSION;

    VkInstanceCreateInfo instanceCI = vkinfo::InstanceInfo();
    instanceCI.pApplicationInfo = &appInfo;

    uint32_t extensionCount = 0;
    const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    std::vector<const char *> instanceExtensions(extensions, extensions + extensionCount);

    if (m_EnableValidationLayer)
    {
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    CHECK_VK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
    if (extensionCount > 0)
    {
        if (extensionCount != static_cast<uint32_t>(VulkanInstance::s_SupportedExtensions.size()))
        {
            std::vector<VkExtensionProperties> extensions(extensionCount);
            CHECK_VK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()));
            for (const auto &extensionProps : extensions)
            {
                VulkanInstance::s_SupportedExtensions.push_back(std::string(extensionProps.extensionName));
            }
        }
    }
    else
    {
        FATAL("Instance extension count is 0!");
    }

    if (m_EnabledEextensions.size() > 0)
    {
        for (const auto &enableExtension : m_EnabledEextensions)
        {
            if (std::find(instanceExtensions.begin(), instanceExtensions.end(), enableExtension.c_str()) == instanceExtensions.end())
            {
                instanceExtensions.push_back(enableExtension.c_str());
            }
        }
    }

    for (const char *instancExtension : instanceExtensions)
    {
        if (std::find(VulkanInstance::s_SupportedExtensions.begin(), VulkanInstance::s_SupportedExtensions.end(), std::string(instancExtension)) == VulkanInstance::s_SupportedExtensions.end())
        {
            FATAL("%s required, but not available at instance level!", instancExtension);
        }
    }

    if (m_EnableValidationLayer)
    {
        std::vector<const char *> instanceLayers = {"VK_LAYER_KHRONOS_validation"};
        uint32_t layerCount = 0;
        CHECK_VK_RESULT(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
        if (layerCount > 0)
        {
            if (layerCount != static_cast<uint32_t>(VulkanInstance::s_SupportedLayers.size()))
            {
                std::vector<VkLayerProperties> layers(layerCount);
                CHECK_VK_RESULT(vkEnumerateInstanceLayerProperties(&layerCount, layers.data()));
                for (const auto &layerProps : layers)
                {
                    VulkanInstance::s_SupportedLayers.push_back(std::string(layerProps.layerName));
                }
            }
        }
        else
        {
            FATAL("Instance layer count is 0!");
        }

        if (m_EnabledLayers.size() > 0)
        {
            for (const auto &enableLayer : m_EnabledLayers)
            {
                if (std::find(instanceLayers.begin(), instanceLayers.end(), enableLayer.c_str()) == instanceLayers.end())
                {
                    instanceLayers.push_back(enableLayer.c_str());
                }
            }
        }

        for (const char *instanceLayer : instanceLayers)
        {
            if (std::find(VulkanInstance::s_SupportedLayers.begin(), VulkanInstance::s_SupportedLayers.end(), std::string(instanceLayer)) == VulkanInstance::s_SupportedLayers.end())
            {
                FATAL("%s required, but not available at instance level!", instanceLayer);
            }
        }

        VkDebugUtilsMessengerCreateInfoEXT debugInfo = vkinfo::DebugMessengerInfo(VulkanInstance::DebugCallback);
        instanceCI.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        instanceCI.ppEnabledExtensionNames = instanceExtensions.data();
        instanceCI.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
        instanceCI.ppEnabledLayerNames = instanceLayers.data();
        instanceCI.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT *>(&debugInfo);

        CHECK_VK_RESULT(vkCreateInstance(&instanceCI, p_Allocator, &m_Instance));
    }
    else
    {
        instanceCI.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        instanceCI.ppEnabledExtensionNames = instanceExtensions.data();

        CHECK_VK_RESULT(vkCreateInstance(&instanceCI, p_Allocator, &m_Instance));
    }
    INFO("Instance %p is created with %u extensions %u layers!\n", m_Instance, instanceCI.enabledExtensionCount, instanceCI.enabledLayerCount);
}

void VulkanInstance::SetUpDebugMessenger()
{
    if (m_Instance == VK_NULL_HANDLE)
    {
        FATAL("No valid instance!");
    }
    if (m_EnableValidationLayer)
    {
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
        if (func != nullptr)
        {
            VkDebugUtilsMessengerCreateInfoEXT debugInfo = vkinfo::DebugMessengerInfo(VulkanInstance::DebugCallback);
            CHECK_VK_RESULT(func(m_Instance, &debugInfo, p_Allocator, &m_DebugMessenger));
        }
        else
        {
            FATAL("Failed to set up debug messenger!");
        }
        INFO("Debug messenger %p is created!\n", m_DebugMessenger);
    }
    else
    {
        INFO("Validation layer is not being enabled!\n");
    }
}

void VulkanInstance::DestroyDebugMessenger()
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr)
    {
        func(m_Instance, m_DebugMessenger, p_Allocator);
    }
    m_DebugMessenger = VK_NULL_HANDLE;
}

void VulkanInstance::DestroyInstance()
{
    vkDestroyInstance(m_Instance, p_Allocator);
    m_Instance = VK_NULL_HANDLE;
}
