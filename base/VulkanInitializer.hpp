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

#ifndef VULKAN_INITIALIZER_HEADER
#define VULKAN_INITIALIZER_HEADER

#pragma once

#include "vulkan/vulkan.h"

#include <vector>

namespace vkinfo
{
    static inline VkApplicationInfo AppInfo()
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        return appInfo;
    }

    static inline VkInstanceCreateInfo InstanceInfo()
    {
        VkInstanceCreateInfo instanceCI{};
        instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        return instanceCI;
    }

    static inline VkDebugUtilsMessengerCreateInfoEXT DebugMessengerInfo(
        PFN_vkDebugUtilsMessengerCallbackEXT pfnCallback,
        VkDebugUtilsMessageSeverityFlagsEXT msgSeverity =
            /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/
        /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |*/
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        VkDebugUtilsMessageTypeFlagsEXT msgType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity = msgSeverity;
        debugInfo.messageType = msgType;
        debugInfo.pfnUserCallback = reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(pfnCallback);
        return debugInfo;
    }

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    static inline VkWin32SurfaceCreateInfoKHR SurfaceInfo()
    {
        VkWin32SurfaceCreateInfoKHR surfaceCI{};
        surfaceCI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        return surfaceCI;
    }
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    static inline VkWaylandSurfaceCreateInfoKHR SurfaceInfo()
    {
        VkWaylandSurfaceCreateInfoKHR surfaceCI{};
        surfaceCI.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        return surfaceCI;
    }
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    static inline VkXcbSurfaceCreateInfoKHR SurfaceInfo()
    {
        VkXcbSurfaceCreateInfoKHR surfaceCI{};
        surfaceCI.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        return surfaceCI;
    }
#endif

    static inline VkDeviceCreateInfo DeviceInfo()
    {
        VkDeviceCreateInfo deviceCI{};
        deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        return deviceCI;
    }

    static inline VkDeviceQueueCreateInfo DeviceQueueInfo()
    {
        VkDeviceQueueCreateInfo deviceQueueInfo{};
        deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        return deviceQueueInfo;
    }

    static inline VkSwapchainCreateInfoKHR SwapChainInfo()
    {
        VkSwapchainCreateInfoKHR swapChainCI{};
        swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        return swapChainCI;
    }

    static inline VkImageViewCreateInfo ImageViewInfo()
    {
        VkImageViewCreateInfo imageviewCI{};
        imageviewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        return imageviewCI;
    }

    static inline VkCommandPoolCreateInfo CommandPoolInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
    {
        VkCommandPoolCreateInfo poolCI{};
        poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCI.queueFamilyIndex = queueFamilyIndex;
        poolCI.flags = flags;
        return poolCI;
    }

    static inline VkCommandBufferAllocateInfo CommandBufferAllocteInfo(VkCommandPool pool, VkCommandBufferLevel level, uint32_t bufferCount)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = pool;
        allocInfo.level = level;
        allocInfo.commandBufferCount = bufferCount;
        return allocInfo;
    }

    static inline VkCommandBufferBeginInfo CommandBufferBeginInfo()
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        return beginInfo;
    }

    static inline VkSubmitInfo SubmitInfo()
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        return submitInfo;
    }

    static inline VkFenceCreateInfo FenceInfo(VkFenceCreateFlags flags)
    {
        VkFenceCreateInfo fenceCI{};
        fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags = flags;
        return fenceCI;
    }

    static inline VkSemaphoreCreateInfo SemaphoreInfo()
    {
        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        return semaphoreCI;
    }

    static inline VkPresentInfoKHR PresentInfo()
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        return presentInfo;
    }

    static inline VkImageCreateInfo ImageInfo()
    {
        VkImageCreateInfo imageCI{};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        return imageCI;
    }

    static inline VkMemoryAllocateInfo MemoryAllocInfo(VkDeviceSize size, uint32_t memTypeIndex)
    {
        VkMemoryAllocateInfo memAlloc{};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = size;
        memAlloc.memoryTypeIndex = memTypeIndex;
        return memAlloc;
    }

    static inline VkRenderPassCreateInfo RenderPassInfo()
    {
        VkRenderPassCreateInfo renderPassCI{};
        renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        return renderPassCI;
    }

    static inline VkFramebufferCreateInfo FrameBufferInfo()
    {
        VkFramebufferCreateInfo frameCI{};
        frameCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        return frameCI;
    }

    static inline VkBufferCreateInfo BufferInfo(VkDeviceSize size, VkBufferUsageFlags usage)
    {
        VkBufferCreateInfo bufferCI{};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = size;
        bufferCI.usage = usage;
        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        return bufferCI;
    }

    static inline VkShaderModuleCreateInfo ShaderModuleInfo(size_t codeSize, const void *pCode)
    {
        VkShaderModuleCreateInfo shaderModuleCI{};
        shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCI.codeSize = codeSize;
        shaderModuleCI.pCode = reinterpret_cast<const uint32_t *>(pCode);
        return shaderModuleCI;
    }

    static inline VkPipelineShaderStageCreateInfo ShaderStageInfo()
    {
        VkPipelineShaderStageCreateInfo shaderStageCI{};
        shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        return shaderStageCI;
    }

    static inline VkGraphicsPipelineCreateInfo GraphicsPipelineInfo()
    {
        VkGraphicsPipelineCreateInfo pipelineCI{};
        pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.basePipelineIndex = -1;
        pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
        return pipelineCI;
    }

    static inline VkComputePipelineCreateInfo ComputePipelineInfo()
    {
        VkComputePipelineCreateInfo pipelineCI{};
        pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineCI.basePipelineIndex = -1;
        pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
        return pipelineCI;
    }

    static inline VkRayTracingPipelineCreateInfoKHR RayTracingPipelineInfo()
    {
        VkRayTracingPipelineCreateInfoKHR pipelineCI{};
        pipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        pipelineCI.basePipelineIndex = -1;
        pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
        return pipelineCI;
    }

    static inline VkDescriptorSetLayoutBinding SetLayoutBinding(uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkShaderStageFlags stage, const VkSampler *immutableSampler = nullptr)
    {
        VkDescriptorSetLayoutBinding setBinding{};
        setBinding.binding = binding;
        setBinding.descriptorCount = descriptorCount;
        setBinding.descriptorType = type;
        setBinding.stageFlags = stage;
        setBinding.pImmutableSamplers = immutableSampler;
        return setBinding;
    }

    static inline VkDescriptorPoolSize PoolSize(uint32_t count, VkDescriptorType type)
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.descriptorCount = count;
        poolSize.type = type;
        return poolSize;
    }

    static inline VkDescriptorSetLayoutCreateInfo SetLayoutInfo()
    {
        VkDescriptorSetLayoutCreateInfo layoutCI{};
        layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        return layoutCI;
    }

    static inline VkPushConstantRange PushConstant(uint32_t offset, uint32_t size, VkShaderStageFlags shaderStage)
    {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.offset = offset;
        pushConstantRange.size = size;
        pushConstantRange.stageFlags = shaderStage;
        return pushConstantRange;
    }

    static inline VkPipelineLayoutCreateInfo PipelineLayoutInfo(size_t pushConstantCount, const VkPushConstantRange *pPushConstant, size_t setLayoutCount, const VkDescriptorSetLayout *pSetLayout)
    {
        VkPipelineLayoutCreateInfo layoutCI{};
        layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCI.pushConstantRangeCount = static_cast<uint32_t>(pushConstantCount);
        layoutCI.pPushConstantRanges = pPushConstant;
        layoutCI.setLayoutCount = static_cast<uint32_t>(setLayoutCount);
        layoutCI.pSetLayouts = pSetLayout;
        return layoutCI;
    }

    static inline VkVertexInputBindingDescription VertexInputBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
    {
        VkVertexInputBindingDescription vertexInputBinding{};
        vertexInputBinding.binding = binding;
        vertexInputBinding.stride = stride;
        vertexInputBinding.inputRate = inputRate;
        return vertexInputBinding;
    }

    static inline VkVertexInputAttributeDescription VertexInputAttribute(uint32_t binding, uint32_t location, uint32_t offset, VkFormat format)
    {
        VkVertexInputAttributeDescription vertexInputAttribute{};
        vertexInputAttribute.binding = binding;
        vertexInputAttribute.location = location;
        vertexInputAttribute.format = format;
        vertexInputAttribute.offset = offset;
        return vertexInputAttribute;
    }

    static inline VkPipelineVertexInputStateCreateInfo VertexInputStateInfo()
    {
        VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
        vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        return vertexInputStateCI;
    }

    static inline VkPipelineVertexInputStateCreateInfo VertexInputStateInfo(const std::vector<VkVertexInputBindingDescription> &bindingDescription,
                                                                            const std::vector<VkVertexInputAttributeDescription> &attributeDescription)
    {
        VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
        vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCI.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
        vertexInputStateCI.pVertexBindingDescriptions = bindingDescription.data();
        vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
        vertexInputStateCI.pVertexAttributeDescriptions = attributeDescription.data();
        return vertexInputStateCI;
    }

    static inline VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo(VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flags, VkBool32 primitiveRestartEnable)
    {
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{};
        inputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyCI.topology = topology;
        inputAssemblyCI.flags = flags;
        inputAssemblyCI.primitiveRestartEnable = primitiveRestartEnable;
        return inputAssemblyCI;
    }

    static inline VkPipelineRasterizationStateCreateInfo RasterizationInfo(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace, VkPipelineRasterizationStateCreateFlags flags = 0)
    {
        VkPipelineRasterizationStateCreateInfo rasterizationCI{};
        rasterizationCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationCI.polygonMode = polygonMode;
        rasterizationCI.cullMode = cullMode;
        rasterizationCI.frontFace = frontFace;
        rasterizationCI.depthClampEnable = VK_FALSE;
        rasterizationCI.lineWidth = 1.0f;
        rasterizationCI.flags = flags;
        return rasterizationCI;
    }

    static inline VkPipelineMultisampleStateCreateInfo MultisampleStateInfo(VkSampleCountFlagBits rasterizationSamples,
                                                                            VkPipelineMultisampleStateCreateFlags flags = 0)
    {
        VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
        multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCI.rasterizationSamples = rasterizationSamples;
        multisampleStateCI.flags = flags;
        return multisampleStateCI;
    }

    static inline VkPipelineDepthStencilStateCreateInfo DepthStencilInfo(VkBool32 enableDepthTest,
                                                                         VkBool32 enableWriteDepth,
                                                                         VkCompareOp depthCompareOp)
    {
        VkPipelineDepthStencilStateCreateInfo depthStencilCI{};
        depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilCI.depthTestEnable = enableDepthTest;
        depthStencilCI.depthWriteEnable = enableWriteDepth;
        depthStencilCI.depthCompareOp = depthCompareOp;
        depthStencilCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
        return depthStencilCI;
    }

    static inline VkPipelineColorBlendAttachmentState ColorBlendAttachmentInfo(VkColorComponentFlags colorWriteMask, VkBool32 blendEnable)
    {
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = colorWriteMask;
        colorBlendAttachment.blendEnable = blendEnable;
        return colorBlendAttachment;
    }

    static inline VkPipelineColorBlendStateCreateInfo ColorBlendStateInfo(size_t attachmentCount, const VkPipelineColorBlendAttachmentState *pAttachments)
    {
        VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
        colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCI.attachmentCount = static_cast<uint32_t>(attachmentCount);
        colorBlendStateCI.pAttachments = pAttachments;
        return colorBlendStateCI;
    }

    static inline VkPipelineViewportStateCreateInfo ViewportInfo(uint32_t viewportCount, uint32_t scissorCount, VkPipelineViewportStateCreateFlags flags = 0)
    {
        VkPipelineViewportStateCreateInfo viewportCI{};
        viewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportCI.viewportCount = viewportCount;
        viewportCI.scissorCount = scissorCount;
        viewportCI.flags = flags;
        return viewportCI;
    }

    static inline VkPipelineDynamicStateCreateInfo DynamicStateInfo(size_t dynamicStateCount,
                                                                    const VkDynamicState *pDynamicStates,
                                                                    VkPipelineDynamicStateCreateFlags flags = 0)
    {
        VkPipelineDynamicStateCreateInfo dynamicStateCI{};
        dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCI.pDynamicStates = pDynamicStates;
        dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateCount);
        dynamicStateCI.flags = flags;
        return dynamicStateCI;
    }

    static inline VkPipelineTessellationStateCreateInfo TessellationInfo(uint32_t patchControllPoints)
    {
        VkPipelineTessellationStateCreateInfo tessellationCI{};
        tessellationCI.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        tessellationCI.patchControlPoints = patchControllPoints;
        return tessellationCI;
    }

    static inline VkRenderPassBeginInfo RenderPassBeginInfo(VkRenderPass renderPass, VkFramebuffer frameBuffer)
    {
        VkRenderPassBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.renderPass = renderPass;
        beginInfo.framebuffer = frameBuffer;
        return beginInfo;
    }

    static inline VkDescriptorPoolCreateInfo DescripotrPoolInfo(uint32_t maxSets)
    {
        VkDescriptorPoolCreateInfo poolCI{};
        poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCI.maxSets = maxSets;
        return poolCI;
    }

    static inline VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo(VkDescriptorPool pool, const VkDescriptorSetLayout *pSetLayout, size_t descriptorSetCount)
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.pSetLayouts = pSetLayout;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetCount);
        return allocInfo;
    }

    static inline VkWriteDescriptorSet DescriptorWriteInfo(VkDescriptorType type, VkDescriptorSet set, uint32_t binding, size_t dstArrayElement, size_t descriptorCount)
    {
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.dstBinding = binding;
        write.descriptorType = type;
        write.dstArrayElement = dstArrayElement;
        write.descriptorCount = static_cast<uint32_t>(descriptorCount);
        return write;
    }

    static inline VkImageMemoryBarrier ImageMemoryBarrier()
    {
        VkImageMemoryBarrier imageBarrier{};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        return imageBarrier;
    }

    static inline VkSamplerCreateInfo SamplerInfo()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        return samplerInfo;
    }
}

#endif
