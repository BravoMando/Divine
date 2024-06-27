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

#ifndef VULKAN_BUFFER_HEADER
#define VULKAN_BUFFER_HEADER

#pragma once

#include "VulkanCore.h"
#include "vulkan/vulkan.h"

class DVAPI_ATTR VulkanBuffer final
{
public:
    VkDevice Device = VK_NULL_HANDLE;
    bool IsInitialized = false;
    const VkAllocationCallbacks *Allocator = nullptr;
    VkBufferUsageFlags Usage = 0U;
    VkMemoryPropertyFlags MemoryProperty = 0U;
    VkBuffer Buffer = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo DescriptorBufferInfo{};
    VkDeviceSize Size = 0U;
    VkDeviceSize Alignment = 0U;
    void *Mapped = nullptr;

    VulkanBuffer(const VkAllocationCallbacks *pAllocator = nullptr)
    {
        Allocator = pAllocator;
    }
    ~VulkanBuffer() {}
    VulkanBuffer(const VulkanBuffer &) = delete;
    VulkanBuffer &operator=(const VulkanBuffer &) = delete;
    VulkanBuffer(VulkanBuffer &&) = default;
    VulkanBuffer &operator=(VulkanBuffer &&) = default;

    void Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0U);
    void Unmap();
    void Bind(VkDeviceSize offset = 0U);
    void SetDescriptorBuffer(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0U);
    void CopyData(const void *data, VkDeviceSize size);
    void Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0U);
    /**
     * @brief Invalidate a memory range of the buffer to make it visible to the host.
     * @note Only required for non-coherent memory.
     * @param size Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
     * @param offset Byte offset from beginning.
     */
    void Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0U);
    void Destroy();
};

#endif
