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

#ifndef VULKAN_TEXTURE_HEADER
#define VULKAN_TEXTURE_HEADER

#pragma once

#include "VulkanCore.h"
#include "vulkan/vulkan.h"

class DVAPI_ATTR VulkanTexture final
{
public:
    VkDevice Device = VK_NULL_HANDLE;
    bool IsInitialized = false;
    const VkAllocationCallbacks *Allocator = nullptr;
    VkImageLayout Layout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t Width = 0U;
    uint32_t Height = 0U;
    uint32_t MipMapLevelCount = 1U;
    uint32_t ArrayLayerCount = 1U;
    VkImage Image = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;
    VkImageView View = VK_NULL_HANDLE;
    VkDescriptorImageInfo DescriptorImageInfo{};
    VkSampler Sampler = VK_NULL_HANDLE;
    uint32_t Index = 0U;

    VulkanTexture(const VkAllocationCallbacks *pAllocator = nullptr)
    {
        Allocator = pAllocator;
    }
    ~VulkanTexture() {}
    VulkanTexture(const VulkanTexture &) = delete;
    VulkanTexture &operator=(const VulkanTexture &) = delete;
    VulkanTexture(VulkanTexture &&) = default;
    VulkanTexture &operator=(VulkanTexture &&) = default;

    void SetDescriptorImage();
    void Destroy();
};

#endif
