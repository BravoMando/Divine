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

#ifndef VULKAN_DEVICE_HEADER
#define VULKAN_DEVICE_HEADER

#pragma once

#include "VulkanCore.h"
#include "VulkanMedium.hpp"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"

#include <string>
#include <vector>

class DVAPI_ATTR VulkanDevice final
{
private:
    bool m_EnableValidationLayer = true;
    const VkAllocationCallbacks *p_Allocator = nullptr;
    VkPhysicalDevice m_GPU = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    QueueFamilyIndices *p_QueueFamilyIndices = nullptr;
    const uint32_t *p_UniqueQueueFamilyIndices = nullptr;
    uint32_t m_UniqueQueueFamilyIndexCount = 0;
    Queues *p_Queues = nullptr;
    // For buffer operation(copy/create/destroy buffer)
    VkCommandPool m_TransferCmdPool = VK_NULL_HANDLE;

public:
    explicit VulkanDevice(bool enableValidationLayer, const VkAllocationCallbacks *pAllocator = nullptr);
    ~VulkanDevice();
    VulkanDevice(const VulkanDevice &) = delete;
    VulkanDevice(VulkanDevice &&) = delete;
    VulkanDevice &operator=(const VulkanDevice &) = delete;
    VulkanDevice &operator=(VulkanDevice &&) = delete;

    std::vector<std::string> m_SupportedExtensions = {};
    std::vector<std::string> m_EnabledExtensions = {};
    std::vector<std::string> m_SupportedLayers = {};
    std::vector<std::string> m_EnabledLayers = {};
    VkPhysicalDeviceProperties m_GPUProperties{};
    VkPhysicalDeviceDriverProperties m_GPUDriverProperties{};
    VkPhysicalDeviceFeatures m_GPUFeatures{};
    std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties{};

    /**
     * @brief Pick a GPU and create logical device.
     * @param instance A valid instance handle.
     * @param surface A valid surface handle.
     * @param queueType A enum class to enable which queue is required.
     * @param pIndices The address of QueueFamilyIndices memory, must not be nullptr.
     * @param pQueues The address of Queues memory, must not be nullptr.
     */
    void InitDevice(VkInstance instance,
                    VkSurfaceKHR surface,
                    QueueTypeFlags queueType,
                    QueueFamilyIndices *pIndices,
                    std::vector<uint32_t> &uniqueIndices,
                    Queues *pQueues);
    // Create outer command pool
    VkCommandPool CreateCommandPool(uint32_t queueFamilyIndex,
                                    VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    // Allocate one command buffer
    void AllocateCommandBuffers(VkCommandPool pool, VkCommandBufferLevel level, uint32_t bufferCount, VkCommandBuffer *pBuffer);
    /**
     * @brief Create a buffer object with exclusive sharing mode, and bind to its memory automatically.
     * @param pBuffer The address of the buffer object.
     * @param data The data address to be copied from, if it is nullptr, we don't copy data but still bind buffer to its memory.
     */
    void CreateBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VulkanBuffer *pBuffer,
                      const void *data = nullptr);
    /**
     * @brief Allocate a command buffer from the command pool.
     * @param level Level(primary or secondary) of the new command buffer.
     * @param pool Command pool from which the command buffer will be allocated.
     * @param begin If true, recording on the new command buffer will be started.
     * @return A handle to the allocated command buffer.
     */
    VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin = false);
    /**
     * @brief Allocate a command buffer from the command pool.
     * @param level Level(primary or secondary) of the new command buffer.
     * @param begin If true, recording on the new command buffer will be started.
     * @return A handle to the allocated command buffer.
     */
    VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin = false);
    /**
     * @brief Finish command buffer recording and submit it to a queue.
     * @param commandBuffer Command buffer to flush.
     * @param queue Queue to submit the command buffer to.
     * @param pool Command pool on which the command buffer has been created.
     * @param free Free the command buffer once it has been submitted.
     * @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from.
     * @note Uses a fence to ensure command buffer has finished executing.
     */
    void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free = true);
    /**
     * @brief Finish command buffer recording and submit it to a queue.
     * @param commandBuffer Command buffer to flush.
     * @param queue Queue to submit the command buffer to.
     * @param free Free the command buffer once it has been submitted.
     * @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from.
     * @note Uses a fence to ensure command buffer has finished executing.
     */
    void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);
    /**
     * @brief Copy buffer memory.
     * @note If the copyRegin is nullptr, then copy the whole src buffer memory size.
     */
    void CopyBuffer(VulkanBuffer *src, VulkanBuffer *dst, VkQueue queue, VkBufferCopy *copyRegin = nullptr);
    /**
     * @brief Copy buffer memory.
     * @note If the copyRegin is nullptr, then copy the whole src buffer memory size.
     */
    void CopyBuffer(VulkanBuffer *src, VulkanBuffer *dst, VkBufferCopy *copyRegin = nullptr);
    /**
     * @brief Copy buffer to image. This will change the image layout to transfer dstination optimal.
     * @param queue The transfer queue.
     * @param width The width of image.
     * @param height The height of image.
     * @param copyReginCount The count of copy regin.
     * @param copyRegin The copy regin array pointer.
     * @note If copy regin count is 0, this will only copy the mip level 0(row image data) to the image.
     */
    void CopyBufferToImage(VulkanBuffer *src,
                           VulkanTexture *dst,
                           VkQueue queue,
                           uint32_t width,
                           uint32_t height,
                           uint32_t copyReginCount = 0,
                           VkBufferImageCopy *pCopyRegins = nullptr);
    /**
     * @brief Copy buffer to image. This will change the image layout to transfer dstination optimal.
     * @param queue The transfer queue.
     * @param width The width of image.
     * @param height The height of image.
     * @param copyReginCount The count of copy regin.
     * @param copyRegin The copy regin array pointer.
     * @note If copy regin count is 0, this will only copy the mip level 0(row image data) to the image.
     */
    void CopyBufferToImage(VulkanBuffer *src,
                           VulkanTexture *dst,
                           uint32_t width,
                           uint32_t height,
                           uint32_t copyReginCount = 0,
                           VkBufferImageCopy *pCopyRegins = nullptr);

    inline const VkPhysicalDevice GetGPU() const { return m_GPU; }
    inline VkPhysicalDevice GetGPU() { return m_GPU; }
    inline const VkDevice GetDevice() const { return m_Device; }
    inline VkDevice GetDevice() { return m_Device; }
    inline const QueueFamilyIndices *const GetDeviceQueueFamilyIndices() const
    {
        return p_QueueFamilyIndices;
    }
    inline const Queues *const GetDeviceQueues() const
    {
        return p_Queues;
    }

    // Destroy command pool
    void DestroyCommandPool(VkCommandPool pool);
    // Destroy logical device
    void DestroyLogicalDevice();
    // Check extension support
    bool ExtensionSupport(const std::string &extensionName);
    // Check layer support
    bool LayerSupport(const std::string &layerName);
};

#endif
