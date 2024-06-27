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

#ifndef VULKAN_MEDIUM_HEADER
#define VULKAN_MEDIUM_HEADER

#pragma once

#include "VulkanCore.h"
#include "vulkan/vulkan.h"

#include <vector>

typedef uint32_t TypeFlags;

typedef enum QueueTypeFlagBits
{
#define _BIT_(i) (1U << i)
    QUEUE_TYPE_NONE = 0U,
    QUEUE_TYPE_COMPUTE = _BIT_(0),
    QUEUE_TYPE_GRAPHICS = _BIT_(1),
    QUEUE_TYPE_TRANSFER = _BIT_(2),
    QUEUE_TYPE_PRESENT = _BIT_(3),
    QUEUE_TYPE_ALL = QUEUE_TYPE_COMPUTE | QUEUE_TYPE_GRAPHICS | QUEUE_TYPE_TRANSFER | QUEUE_TYPE_PRESENT
#undef _BIT_
} QueueTypeFlagBits;

typedef TypeFlags QueueTypeFlags;

typedef enum ModelTypeFlagBits
{
    MODEL_TYPE_NONE = 0U,
    MODEL_TYPE_OBJ = 1U,
    MODEL_TYPE_GLTF = 2U
} ModelTypeFlagBits;

typedef TypeFlags ModelTypeFlags;

typedef enum CameraType
{
    CAMERA_TYPE_NONE = 0U,
    CAMERA_TYPE_LOOK_AT = 1U,
    CAMERA_TYPE_FIRST_PERSON = 2U
} CameraType;

typedef TypeFlags CameraTypeFlags;

// Including Graphics, Present, Transfer and Compute queue index
struct DVAPI_ATTR QueueFamilyIndices
{
    uint32_t Compute = 0xFFFFFFFFU;
    uint32_t Graphics = 0xFFFFFFFFU;
    uint32_t Transfer = 0xFFFFFFFFU;
    uint32_t Present = 0xFFFFFFFFU;
    bool ComputeHasValue = false;
    bool GraphicsHasValue = false;
    bool TransferHasValue = false;
    bool PresentHasValue = false;

    QueueFamilyIndices() = default;
    ~QueueFamilyIndices() = default;
    QueueFamilyIndices(const QueueFamilyIndices &) = default;
    QueueFamilyIndices &operator=(const QueueFamilyIndices &) = default;
    QueueFamilyIndices(QueueFamilyIndices &&) = default;
    QueueFamilyIndices &operator=(QueueFamilyIndices &&) = default;
};

// Including Graphics, Present, Transfer and Compute queue
struct DVAPI_ATTR Queues
{
    VkQueue Compute = VK_NULL_HANDLE;
    VkQueue Graphics = VK_NULL_HANDLE;
    VkQueue Transfer = VK_NULL_HANDLE;
    VkQueue Present = VK_NULL_HANDLE;

    Queues() = default;
    ~Queues() = default;
    Queues(const Queues &) = default;
    Queues &operator=(const Queues &) = default;
    Queues(Queues &&) = default;
    Queues &operator=(Queues &&) = default;
};

// Image handle, image view handle and image memory handle
struct DVAPI_ATTR SwapChainImageBuffer
{
    VkImage Image = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;
    VkImageView View = VK_NULL_HANDLE;
    VkDescriptorImageInfo DescriptorImageInfo{};

    SwapChainImageBuffer() = default;
    ~SwapChainImageBuffer() = default;
    SwapChainImageBuffer(const SwapChainImageBuffer &) = default;
    SwapChainImageBuffer &operator=(const SwapChainImageBuffer &) = default;
    SwapChainImageBuffer(SwapChainImageBuffer &&) = default;
    SwapChainImageBuffer &operator=(SwapChainImageBuffer &&) = default;

    inline void SetDescriptorImage(VkSampler sampler, VkImageLayout layoutInSubpass)
    {
        DescriptorImageInfo.imageLayout = layoutInSubpass;
        DescriptorImageInfo.sampler = sampler;
        DescriptorImageInfo.imageView = View;
    }
};

// Pipeline configuration data
struct DVAPI_ATTR PipelineConfigInfo
{
    /**
     * @note By default this shall not be set staticly
     */
    VkViewport Viewport;
    /**
     * @note By default this shall not be set staticly
     */
    VkRect2D Scissor;

    std::vector<VkVertexInputBindingDescription> BindingDescriptions{};
    std::vector<VkVertexInputAttributeDescription> AttributeDescriptions{};
    VkPipelineVertexInputStateCreateInfo VertexInputInfo{};
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo{};
    VkPipelineTessellationStateCreateInfo TessellationInfo{};
    VkPipelineViewportStateCreateInfo ViewportInfo{};
    VkPipelineRasterizationStateCreateInfo RasterizationInfo{};
    VkPipelineMultisampleStateCreateInfo MultisampleInfo{};
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo{};
    std::vector<VkPipelineColorBlendAttachmentState> ColorBlendAttachments{};
    VkPipelineColorBlendStateCreateInfo ColorBlendInfo{};
    std::vector<VkDynamicState> DynamicStatesEnables{};
    VkPipelineDynamicStateCreateInfo DynamicStateInfo{};
    VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
    VkRenderPass RenderPass = VK_NULL_HANDLE;
    uint32_t Subpass = 0;
    VkPipeline BasePipelineHandle = VK_NULL_HANDLE;
    int32_t BasePipelineIndex = -1;

    PipelineConfigInfo() = default;
    ~PipelineConfigInfo() = default;
    PipelineConfigInfo(const PipelineConfigInfo &) = delete;
    PipelineConfigInfo &operator=(const PipelineConfigInfo &) = delete;
    PipelineConfigInfo(PipelineConfigInfo &&) = delete;
    PipelineConfigInfo &operator=(PipelineConfigInfo &&) = delete;
};

#endif
