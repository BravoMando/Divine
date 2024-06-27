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

#include "VulkanBuffer.h"
#include "VulkanTools.h"

#include <cstring>

void VulkanBuffer::Map(VkDeviceSize size, VkDeviceSize offset)
{
    if (Device == VK_NULL_HANDLE)
    {
        FATAL("No valid device for buffer operation!");
    }

    CHECK_VK_RESULT(vkMapMemory(Device, Memory, offset, size, 0, &Mapped));
}

void VulkanBuffer::Unmap()
{
    if (Device == VK_NULL_HANDLE)
    {
        FATAL("No valid device for buffer operation!");
    }

    if (Mapped != nullptr)
    {
        vkUnmapMemory(Device, Memory);
    }
}

void VulkanBuffer::Bind(VkDeviceSize offset)
{
    if (Device == VK_NULL_HANDLE)
    {
        FATAL("No valid device for buffer operation!");
    }

    CHECK_VK_RESULT(vkBindBufferMemory(Device, Buffer, Memory, offset));
}

void VulkanBuffer::SetDescriptorBuffer(VkDeviceSize size, VkDeviceSize offset)
{
    DescriptorBufferInfo.buffer = Buffer;
    DescriptorBufferInfo.offset = offset;
    DescriptorBufferInfo.range = size;
}

void VulkanBuffer::CopyData(const void *data, VkDeviceSize size)
{
    if (Device == VK_NULL_HANDLE)
    {
        FATAL("No valid device for buffer operation!");
    }

    if (Mapped != nullptr)
    {
        memcpy(Mapped, data, size);
    }
}

void VulkanBuffer::Flush(VkDeviceSize size, VkDeviceSize offset)
{
    if (Device == VK_NULL_HANDLE)
    {
        FATAL("No valid device for buffer operation!");
    }

    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = Memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    CHECK_VK_RESULT(vkFlushMappedMemoryRanges(Device, 1, &mappedRange));
}

void VulkanBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset)
{
    if (Device == VK_NULL_HANDLE)
    {
        FATAL("No valid device for buffer operation!");
    }

    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = Memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    CHECK_VK_RESULT(vkInvalidateMappedMemoryRanges(Device, 1, &mappedRange));
}

void VulkanBuffer::Destroy()
{
    if (IsInitialized == false)
    {
        return;
    }
    if (Device == VK_NULL_HANDLE)
    {
        FATAL("No valid device for buffer operation!");
    }

    if (Memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(Device, Memory, Allocator);
    }
    if (Buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(Device, Buffer, Allocator);
    }
}
