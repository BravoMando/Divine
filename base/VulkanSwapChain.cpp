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

#include "VulkanSwapChain.h"
#include "VulkanTools.h"
#include "VulkanInitializer.hpp"

#include <algorithm>

VulkanSwapChain::VulkanSwapChain(VkInstance instance, const VkAllocationCallbacks *pAllocator)
{
    p_Allocator = pAllocator;
    if (instance == VK_NULL_HANDLE)
    {
        FATAL("Instance must be valid!");
    }
    this->m_Instance = instance;
}

VulkanSwapChain::~VulkanSwapChain()
{
    DestroyFrameBuffers();
    DestroyImageBuffer(&m_DepthStencilImageBuffer, 1);
    if (m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_Device, m_RenderPass, p_Allocator);
    }
    if (m_SwapChain != VK_NULL_HANDLE)
    {
        CleanUpSwapChain();
    }
    if (m_Surface != VK_NULL_HANDLE)
    {
        DestroySurface();
    }
}

void VulkanSwapChain::CreateSurface(GLFWwindow *pWindow)
{
    CHECK_VK_RESULT(glfwCreateWindowSurface(m_Instance, pWindow, p_Allocator, &m_Surface));
}

void VulkanSwapChain::Connect(VkPhysicalDevice gpu,
                              VkDevice logicalDevice,
                              QueueFamilyIndices *pIndices,
                              const uint32_t *pUniqueIndices,
                              uint32_t uniqueIndexCount,
                              Queues *pQueues,
                              uint32_t maxFramesInFlight)
{
    if (gpu == VK_NULL_HANDLE || logicalDevice == VK_NULL_HANDLE)
    {
        FATAL("GPU and Device entity must be valid!");
    }
    if (pIndices == nullptr || pQueues == nullptr)
    {
        FATAL("The address must not be nullptr!");
    }

    m_GPU = gpu;
    m_Device = logicalDevice;
    p_QueueFamilyIndices = pIndices;
    p_UniqueQueueFamilyIndices = pUniqueIndices;
    m_UniqueQueueFamilyIndexCount = uniqueIndexCount;
    p_Queues = pQueues;
    m_MaxFramesInFlight = maxFramesInFlight;
    m_IsConnected = true;
}

void VulkanSwapChain::InitSwapChain(bool vsync)
{
    if (m_Surface == VK_NULL_HANDLE)
    {
        FATAL("No valid surface!");
    }
    if (!m_IsConnected)
    {
        FATAL("VulkanSwapChain must connect to a valid device!");
    }

    // Counter
    uint32_t count = 0;
    // Surface format
    VkSurfaceFormatKHR swapChainSurfaceFormat{};
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_GPU, m_Surface, &count, nullptr));
    if (count > 0)
    {
        std::vector<VkSurfaceFormatKHR> surfaceFormats(count);
        CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_GPU, m_Surface, &count, surfaceFormats.data()));
        swapChainSurfaceFormat = surfaceFormats[0];
        if (m_Settings.DesiredColorSpaces.size() > 0 && m_Settings.DesiredColorFormats.size() > 0)
        {
            for (const auto &surfaceFormat : surfaceFormats)
            {
                if (std::find(m_Settings.DesiredColorFormats.begin(), m_Settings.DesiredColorFormats.end(), surfaceFormat.format) != m_Settings.DesiredColorFormats.end() &&
                    std::find(m_Settings.DesiredColorSpaces.begin(), m_Settings.DesiredColorSpaces.end(), surfaceFormat.colorSpace) != m_Settings.DesiredColorSpaces.end())
                {
                    swapChainSurfaceFormat = surfaceFormat;
                }
            }
        }
        else
        {
            for (const auto &surfaceFormat : surfaceFormats)
            {
                if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    swapChainSurfaceFormat = surfaceFormat;
                }
            }
        }
        m_SwapChainImageFormat = swapChainSurfaceFormat.format;
        m_SwapChainImageColorSpace = swapChainSurfaceFormat.colorSpace;
    }
    else
    {
        FATAL("Surface format count is 0!");
    }

    // Counter
    count = 0;
    // Present mode
    m_SwapChainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_GPU, m_Surface, &count, nullptr));
    if (count > 0)
    {
        std::vector<VkPresentModeKHR> presentModes(count);
        CHECK_VK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_GPU, m_Surface, &count, presentModes.data()));
        if (vsync)
        {
            for (const auto &presentMode : presentModes)
            {
                if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    m_SwapChainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    INFO("Present mode: VK_PRESENT_MODE_MAILBOX_KHR.\n");
                    break;
                }
            }
            if (m_SwapChainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR)
            {
                m_SwapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
                INFO("Present mode: VK_PRESENT_MODE_FIFO_KHR.\n");
            }
        }
        else
        {
            INFO("Present mode: VK_PRESENT_MODE_IMMEDIATE_KHR.\n");
        }
    }
    else
    {
        FATAL("Surface present mode count is 0!");
    }
}

void VulkanSwapChain::CreateSwapChain(uint32_t width, uint32_t height)
{
    if (m_Surface == VK_NULL_HANDLE)
    {
        FATAL("No valid surface!");
    }
    if (!m_IsConnected)
    {
        FATAL("VulkanSwapChain must connect to a valid device!");
    }

    // Old swap chain
    VkSwapchainKHR oldSwapChain = m_SwapChain;

    // Extent
    VkSurfaceCapabilitiesKHR swapChainSurfaceCapabilities;
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_GPU, m_Surface, &swapChainSurfaceCapabilities));
    if (swapChainSurfaceCapabilities.currentExtent.width == 0xFFFFFFFFU || swapChainSurfaceCapabilities.currentExtent.height == 0xFFFFFFFFU)
    {
        m_SwapChainImageExtent.width = width;
        m_SwapChainImageExtent.height = height;
    }
    else
    {
        m_SwapChainImageExtent = swapChainSurfaceCapabilities.currentExtent;
        width = m_SwapChainImageExtent.width;
        height = m_SwapChainImageExtent.height;
    }

    // Minimum image count
    uint32_t imageCount = swapChainSurfaceCapabilities.minImageCount + 1;
    if (swapChainSurfaceCapabilities.maxImageCount > 0 && imageCount > swapChainSurfaceCapabilities.maxImageCount)
    {
        imageCount = swapChainSurfaceCapabilities.maxImageCount;
    }

    // Pretransform
    VkSurfaceTransformFlagBitsKHR preTransform;
    if (swapChainSurfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        preTransform = swapChainSurfaceCapabilities.currentTransform;
    }

    // Composite alpha
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (const auto &compositeAlphaFlag : compositeAlphaFlags)
    {
        if (swapChainSurfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlag)
        {
            compositeAlpha = compositeAlphaFlag;
            break;
        };
    }

    VkSwapchainCreateInfoKHR swapChainCI = vkinfo::SwapChainInfo();
    swapChainCI.oldSwapchain = oldSwapChain;
    swapChainCI.surface = m_Surface;
    swapChainCI.minImageCount = imageCount;
    swapChainCI.imageExtent = m_SwapChainImageExtent;
    swapChainCI.imageFormat = m_SwapChainImageFormat;
    swapChainCI.imageColorSpace = m_SwapChainImageColorSpace;
    swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCI.imageArrayLayers = 1;
    swapChainCI.presentMode = m_SwapChainPresentMode;
    swapChainCI.clipped = VK_TRUE;
    swapChainCI.preTransform = preTransform;
    swapChainCI.compositeAlpha = compositeAlpha;

    // Enable transfer if support
    if (swapChainSurfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        swapChainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (swapChainSurfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        swapChainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    if (m_UniqueQueueFamilyIndexCount > 1)
    {
        swapChainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCI.queueFamilyIndexCount = m_UniqueQueueFamilyIndexCount;
        swapChainCI.pQueueFamilyIndices = p_UniqueQueueFamilyIndices;
    }
    else
    {
        swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCI.queueFamilyIndexCount = 0;
        swapChainCI.pQueueFamilyIndices = nullptr;
    }

    CHECK_VK_RESULT(vkCreateSwapchainKHR(m_Device, &swapChainCI, p_Allocator, &m_SwapChain));

    // Free old swap chain resources
    if (oldSwapChain != VK_NULL_HANDLE)
    {
        for (size_t i = 0; i < m_SwapChainColorImageBuffers.size(); ++i)
        {
            vkDestroyImageView(m_Device, m_SwapChainColorImageBuffers[i].View, p_Allocator);
        }
        vkDestroySwapchainKHR(m_Device, oldSwapChain, p_Allocator);
    }

    std::vector<VkImage> swapChainImages = {};
    // Get swap chain images
    CHECK_VK_RESULT(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &m_SwapChainImageCount, nullptr));
    swapChainImages.resize(m_SwapChainImageCount);
    CHECK_VK_RESULT(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &m_SwapChainImageCount, swapChainImages.data()));

    // Create swap chain image views
    m_SwapChainColorImageBuffers.resize(m_SwapChainImageCount);
    VkImageViewCreateInfo viewCI = vkinfo::ImageViewInfo();
    for (uint32_t i = 0; i < m_SwapChainImageCount; ++i)
    {
        viewCI.image = swapChainImages[i];
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format = m_SwapChainImageFormat;
        viewCI.components = {VK_COMPONENT_SWIZZLE_R,
                             VK_COMPONENT_SWIZZLE_G,
                             VK_COMPONENT_SWIZZLE_B,
                             VK_COMPONENT_SWIZZLE_A};
        viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount = 1;
        viewCI.subresourceRange.baseMipLevel = 0;
        viewCI.subresourceRange.levelCount = 1;

        m_SwapChainColorImageBuffers[i].Image = swapChainImages[i];
        CHECK_VK_RESULT(vkCreateImageView(m_Device, &viewCI, p_Allocator, &m_SwapChainColorImageBuffers[i].View));
    }

    // When recreate swap chain, clear frame buffer attachment
    for (size_t i = 0; i < m_FrameBufferAttachments.size(); ++i)
    {
        m_FrameBufferAttachments[i].clear();
    }
    m_FrameBufferAttachments.clear();
    m_FrameBufferAttachments.resize(m_SwapChainImageCount);
    for (uint32_t i = 0; i < m_SwapChainImageCount; ++i)
    {
        m_FrameBufferAttachments[i].push_back(m_SwapChainColorImageBuffers[i].View);
    }

    INFO("SwapChain %p is created with %u images!\n", m_SwapChain, m_SwapChainImageCount);
}

VkFormat VulkanSwapChain::FindSupportedImageFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (const auto format : candidates)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(m_GPU, format, &formatProps);
        if (tiling == VK_IMAGE_TILING_LINEAR && (formatProps.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProps.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    FATAL("No supported format found!");
}

void VulkanSwapChain::CreateImageWithInfo(VkImageCreateInfo *pImageCI,
                                          VkImage *pImage,
                                          VkDeviceMemory *pMemory,
                                          VkMemoryPropertyFlags memProperty,
                                          VkDeviceSize memoryOffset)
{
    if (!m_IsConnected)
    {
        FATAL("VulkanSwapChain must connect to a valid device!");
    }

    CHECK_VK_RESULT(vkCreateImage(m_Device, pImageCI, p_Allocator, pImage));

    VkMemoryRequirements memRequires{};
    vkGetImageMemoryRequirements(m_Device, *pImage, &memRequires);
    uint32_t idx = FindMemoryTypeIndex(m_GPU, memRequires.memoryTypeBits, memProperty);
    VkMemoryAllocateInfo memAlloc = vkinfo::MemoryAllocInfo(memRequires.size, idx);
    CHECK_VK_RESULT(vkAllocateMemory(m_Device, &memAlloc, p_Allocator, pMemory));
    CHECK_VK_RESULT(vkBindImageMemory(m_Device, *pImage, *pMemory, memoryOffset));
}

void VulkanSwapChain::CreateSwapChainImageBuffer(SwapChainImageBuffer *pBuffers,
                                                 size_t bufferCount,
                                                 VkImageCreateInfo *pImageCI,
                                                 VkMemoryPropertyFlags memProperty,
                                                 VkDeviceSize memoryOffset,
                                                 VkImageViewCreateInfo *pViewCI,
                                                 bool addToFrameBufferAttachment)
{
    if (!m_IsConnected)
    {
        FATAL("VulkanSwapChain must connect to a valid device!");
    }

    for (uint32_t i = 0; i < bufferCount; ++i)
    {
        CreateImageWithInfo(pImageCI, &(pBuffers + i)->Image, &(pBuffers + i)->Memory, memProperty, memoryOffset);
        pViewCI->image = (pBuffers + i)->Image;
        CHECK_VK_RESULT(vkCreateImageView(m_Device, pViewCI, p_Allocator, &(pBuffers + i)->View));
    }

    if (addToFrameBufferAttachment)
    {
        if (static_cast<uint32_t>(bufferCount) == m_SwapChainImageCount)
        {
            for (uint32_t i = 0; i < m_SwapChainImageCount; ++i)
            {
                m_FrameBufferAttachments[i].push_back((pBuffers + i)->View);
            }
        }
        else if (bufferCount == 1)
        {
            for (uint32_t i = 0; i < m_SwapChainImageCount; ++i)
            {
                m_FrameBufferAttachments[i].push_back(pBuffers->View);
            }
        }
        else
        {
            ERROR("Trying to add swap chain image buffer to frame buffer attachment without matching swap chain image count at %s line: %d!\n", __FILE__, __LINE__);
        }
    }
}

void VulkanSwapChain::CreateDepthStencilImageBuffer()
{
    if (!m_IsConnected)
    {
        FATAL("VulkanSwapChain must connect to a valid device!");
    }

    m_DepthStencilFormat = FindSupportedImageFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                                    VK_IMAGE_TILING_OPTIMAL,
                                                    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    VkImageCreateInfo imageCI = vkinfo::ImageInfo();
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = m_DepthStencilFormat,
    imageCI.extent = {m_SwapChainImageExtent.width, m_SwapChainImageExtent.height, 1};
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (m_UniqueQueueFamilyIndexCount > 1)
    {
        imageCI.queueFamilyIndexCount = m_UniqueQueueFamilyIndexCount;
        imageCI.pQueueFamilyIndices = p_UniqueQueueFamilyIndices;
        imageCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
    }
    else
    {
        imageCI.queueFamilyIndexCount = 0;
        imageCI.pQueueFamilyIndices = nullptr;
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VkImageViewCreateInfo viewCI = vkinfo::ImageViewInfo();
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCI.image = VK_NULL_HANDLE;
    viewCI.format = m_DepthStencilFormat;
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewCI.subresourceRange.baseArrayLayer = 0;
    viewCI.subresourceRange.layerCount = 1;
    viewCI.subresourceRange.baseMipLevel = 0;
    viewCI.subresourceRange.levelCount = 1;
    if (HasStencilFormat(m_DepthStencilFormat))
    {
        viewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        m_HasStencilComponent = true;
    }

    CreateSwapChainImageBuffer(&m_DepthStencilImageBuffer, 1, &imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, &viewCI);

    INFO("Swap chain: 0x%p create default depth-sencil image!\n", m_SwapChain);
}

void VulkanSwapChain::InitRenderPass()
{
    // Attachments
    m_RenderPassResource.Attachments.resize(2);
    // Color attachment
    m_RenderPassResource.Attachments[0].format = m_SwapChainImageFormat;
    m_RenderPassResource.Attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    m_RenderPassResource.Attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_RenderPassResource.Attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    m_RenderPassResource.Attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    m_RenderPassResource.Attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_RenderPassResource.Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_RenderPassResource.Attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    if (m_HasStencilComponent)
    {
        m_RenderPassResource.Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    }
    else
    {
        m_RenderPassResource.Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    // Depth-Stencil attachment
    m_RenderPassResource.Attachments[1].format = m_DepthStencilFormat;
    m_RenderPassResource.Attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    m_RenderPassResource.Attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_RenderPassResource.Attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    m_RenderPassResource.Attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    m_RenderPassResource.Attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_RenderPassResource.Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_RenderPassResource.Attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // References
    m_RenderPassResource.References.resize(2);
    // Color reference
    m_RenderPassResource.References[0].attachment = 0;
    m_RenderPassResource.References[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Depth stencil reference
    m_RenderPassResource.References[1].attachment = 1;
    m_RenderPassResource.References[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Descriptors
    for (uint32_t i = 0; i < m_SwapChainImageCount; ++i)
    {
        m_SwapChainColorImageBuffers[i].SetDescriptorImage(VK_NULL_HANDLE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }
    m_DepthStencilImageBuffer.SetDescriptorImage(VK_NULL_HANDLE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    // Subpasses
    m_RenderPassResource.Subpasses.resize(1);
    m_RenderPassResource.Subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    m_RenderPassResource.Subpasses[0].colorAttachmentCount = 1;
    m_RenderPassResource.Subpasses[0].pColorAttachments = &m_RenderPassResource.References[0];
    m_RenderPassResource.Subpasses[0].pDepthStencilAttachment = &m_RenderPassResource.References[1];

    // Dependencies
    m_RenderPassResource.Dependencies.resize(2);
    // Color attachment
    m_RenderPassResource.Dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    m_RenderPassResource.Dependencies[0].dstSubpass = 0;
    m_RenderPassResource.Dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    m_RenderPassResource.Dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    m_RenderPassResource.Dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // LoadOp clear, StoreOp store
    m_RenderPassResource.Dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;  // All done
    // Depth stencil attacment
    m_RenderPassResource.Dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    m_RenderPassResource.Dependencies[1].dstSubpass = 0;
    m_RenderPassResource.Dependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    m_RenderPassResource.Dependencies[1].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    m_RenderPassResource.Dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // LoadOp clear, StoreOp store
    m_RenderPassResource.Dependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;  // All done

    m_IsRenderPassInitialized = true;
    INFO("Swap chain: 0x%p create default color and depth-stencil subpass attachment!\n", m_SwapChain);
}

void VulkanSwapChain::SetUpRenderPass()
{
    if (!m_IsConnected)
    {
        FATAL("VulkanSwapChain must connect to a valid device!");
    }
    if (!m_IsRenderPassInitialized)
    {
        FATAL("Render pass resources are not initialized!");
    }

    VkRenderPassCreateInfo renderPassCI = vkinfo::RenderPassInfo();
    renderPassCI.attachmentCount = static_cast<uint32_t>(m_RenderPassResource.Attachments.size());
    renderPassCI.pAttachments = m_RenderPassResource.Attachments.data();
    renderPassCI.subpassCount = static_cast<uint32_t>(m_RenderPassResource.Subpasses.size());
    renderPassCI.pSubpasses = m_RenderPassResource.Subpasses.data();
    renderPassCI.dependencyCount = static_cast<uint32_t>(m_RenderPassResource.Dependencies.size());
    renderPassCI.pDependencies = m_RenderPassResource.Dependencies.data();
    CHECK_VK_RESULT(vkCreateRenderPass(m_Device, &renderPassCI, p_Allocator, &m_RenderPass));

    INFO("Swap chain: 0x%p create render pass: 0x%p with %d subpass(es), %d subpass dependency(ies) and %d subpass attachment(s)!\n", m_SwapChain, m_RenderPass, renderPassCI.subpassCount, renderPassCI.dependencyCount, renderPassCI.attachmentCount);
}

void VulkanSwapChain::CreateFrameBuffers()
{
    if (!m_IsConnected)
    {
        FATAL("VulkanSwapChain must connect to a valid device!");
    }

    m_FrameBuffers.resize(m_SwapChainImageCount);
    VkFramebufferCreateInfo frameCI = vkinfo::FrameBufferInfo();
    frameCI.width = m_SwapChainImageExtent.width;
    frameCI.height = m_SwapChainImageExtent.height;
    frameCI.layers = 1;
    frameCI.renderPass = m_RenderPass;
    for (uint32_t i = 0; i < m_SwapChainImageCount; ++i)
    {
        frameCI.attachmentCount = static_cast<uint32_t>(m_FrameBufferAttachments[i].size());
        frameCI.pAttachments = m_FrameBufferAttachments[i].data();
        CHECK_VK_RESULT(vkCreateFramebuffer(m_Device, &frameCI, p_Allocator, &m_FrameBuffers[i]));
    }

    INFO("Render pass: 0x%p create frame buffer with %u image view attachment(s)!\n", m_RenderPass, frameCI.attachmentCount);
}

void VulkanSwapChain::RecreateSwapChainResources(uint32_t width, uint32_t height)
{
    m_SwapChainImageCount = 0;
    CreateSwapChain(width, height);
    DestroyImageBuffer(&m_DepthStencilImageBuffer, 1);
    CreateDepthStencilImageBuffer();
    DestroyFrameBuffers();
    CreateFrameBuffers();
}

void VulkanSwapChain::DestroyFrameBuffers()
{
    for (size_t i = 0; i < m_FrameBuffers.size(); ++i)
    {
        vkDestroyFramebuffer(m_Device, m_FrameBuffers[i], p_Allocator);
    }
    m_FrameBuffers.clear();
}

void VulkanSwapChain::DestroyRenderPass()
{
    vkDestroyRenderPass(m_Device, m_RenderPass, p_Allocator);
}

void VulkanSwapChain::DestroyImageBuffer(SwapChainImageBuffer *pBuffers, size_t bufferCount)
{
    for (size_t i = 0; i < bufferCount; ++i)
    {
        vkDestroyImageView(m_Device, (pBuffers + i)->View, p_Allocator);
        vkFreeMemory(m_Device, (pBuffers + i)->Memory, p_Allocator);
        vkDestroyImage(m_Device, (pBuffers + i)->Image, p_Allocator);
    }
}

void VulkanSwapChain::CleanUpSwapChain()
{
    for (size_t i = 0; i < m_SwapChainColorImageBuffers.size(); ++i)
    {
        vkDestroyImageView(m_Device, m_SwapChainColorImageBuffers[i].View, p_Allocator);
    }
    vkDestroySwapchainKHR(m_Device, m_SwapChain, p_Allocator);
    m_SwapChain = VK_NULL_HANDLE;
}

void VulkanSwapChain::DestroySurface()
{
    vkDestroySurfaceKHR(m_Instance, m_Surface, p_Allocator);
    m_Surface = VK_NULL_HANDLE;
}
