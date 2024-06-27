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

#ifndef VULKAN_SWAP_CHAIN_HEADER
#define VULKAN_SWAP_CHAIN_HEADER

#pragma once

#include "VulkanCore.h"
#include "VulkanMedium.hpp"
#include "GLFW/glfw3.h"

#include <vector>

class DVAPI_ATTR VulkanSwapChain final
{
public:
    struct Settings
    {
        // Desired swap chain image format correspond to DesiredColorSpaces
        std::vector<VkFormat> DesiredColorFormats = {};
        // Desired swap chain image color space correspond to DesiredColorFormats
        std::vector<VkColorSpaceKHR> DesiredColorSpaces = {};
    } m_Settings;

    /**
     * @brief Use these resources to create render pass.
     * @note These must be initialized before push or change content.
     */
    struct RenderPassResource
    {
        std::vector<VkAttachmentDescription> Attachments = {};
        std::vector<VkAttachmentReference> References = {};
        std::vector<VkSubpassDescription> Subpasses = {};
        std::vector<VkSubpassDependency> Dependencies = {};
    } m_RenderPassResource;

    /**
     * @brief Use these resources to create frame buffer. m_FrameBufferAttachments[][0] is reserved for swap chain image view,
     * m_FrameBufferAttachments[][1] is reserved for depth stencil image view.
     * @note These must be initialized before push or change content.
     */
    std::vector<std::vector<VkImageView>> m_FrameBufferAttachments = {};

private:
    const VkAllocationCallbacks *p_Allocator = nullptr;
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_GPU = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    QueueFamilyIndices *p_QueueFamilyIndices = nullptr;
    const uint32_t *p_UniqueQueueFamilyIndices = nullptr;
    uint32_t m_UniqueQueueFamilyIndexCount = 0;
    Queues *p_Queues = nullptr;
    uint32_t m_MaxFramesInFlight = 0;
    bool m_IsConnected = false;
    VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
    VkFormat m_SwapChainImageFormat{};
    VkColorSpaceKHR m_SwapChainImageColorSpace{};
    VkPresentModeKHR m_SwapChainPresentMode{};
    VkExtent2D m_SwapChainImageExtent{};
    uint32_t m_SwapChainImageCount = 0;
    // Swap chain image handle and image view handle
    std::vector<SwapChainImageBuffer> m_SwapChainColorImageBuffers = {};
    // Depth-Stencil image format
    VkFormat m_DepthStencilFormat{};
    // Stencil support flag
    bool m_HasStencilComponent = false;
    // Depth-Stencil resources
    SwapChainImageBuffer m_DepthStencilImageBuffer{};
    // Render pass initialization flag
    bool m_IsRenderPassInitialized = false;
    // Render pass
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    // Frame buffers
    std::vector<VkFramebuffer> m_FrameBuffers = {};

public:
    explicit VulkanSwapChain(VkInstance instance, const VkAllocationCallbacks *pAllocator = nullptr);
    ~VulkanSwapChain();
    VulkanSwapChain(const VulkanSwapChain &) = delete;
    VulkanSwapChain(VulkanSwapChain &&) = delete;
    VulkanSwapChain &operator=(const VulkanSwapChain &) = delete;
    VulkanSwapChain &operator=(VulkanSwapChain &&) = delete;

    // The current frame index
    uint32_t m_CurrentFrame = 0;

    void CreateSurface(GLFWwindow *pWindow);

    /**
     * @brief Connect to device and store the queue information.
     * @param gpu A valid physical device handle.
     * @param logicalDevice A valid device handle.
     * @param pIndices The address of QueueFamilyIndices memory, must not be nullptr.
     * @param pQueues The address of Queues memory, must not be nullptr.
     */
    void Connect(VkPhysicalDevice gpu,
                 VkDevice logicalDevice,
                 QueueFamilyIndices *pIndices,
                 const uint32_t *pUniqueIndices,
                 uint32_t uniqueIndexCount,
                 Queues *pQueues,
                 uint32_t maxFramesInFlight);
    // Initialize swap chain surface format and present mode
    void InitSwapChain(bool vsync);
    /**
     * @brief Create swap chain and image view.
     * @param width The address of window width.
     * @param height The address of window height.
     */
    void CreateSwapChain(uint32_t width, uint32_t height);
    // Find supported format among provided candidates
    VkFormat FindSupportedImageFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    // Create image and allocate image memory, image view needs user to create
    void CreateImageWithInfo(VkImageCreateInfo *pImageCI,
                             VkImage *pImage,
                             VkDeviceMemory *pMemory,
                             VkMemoryPropertyFlags memProperty,
                             VkDeviceSize memoryOffset);
    /**
     * @brief Create one-to-one(image-memory-view) image buffer resized to swap chain image count.
     * @param pBuffers The image buffer array pointer.
     * @param bufferCount The array size.
     * @param pImageCI A pointer to image create info struct.
     * @param memProperty The image memory property.
     * @param memoryOffset The offset of the memory.
     * @param pViewCI A pointer to image view create info struct, note the pViewCI->image will be automatically set to the corresponding image.
     * @param addToFrameBufferAttachment Add the buffers' view into frame buffer attachment. If true, execute only if the buffer count match swap chain image count or the buffer count equal to 1.
     */
    void CreateSwapChainImageBuffer(SwapChainImageBuffer *pBuffers,
                                    size_t bufferCount,
                                    VkImageCreateInfo *pImageCI,
                                    VkMemoryPropertyFlags memProperty,
                                    VkDeviceSize memoryOffset,
                                    VkImageViewCreateInfo *pViewCI,
                                    bool addToFrameBufferAttachment = true);
    // Create Depth-Stencil resources, and these will be destroied automatically while other custom image buffer will not
    void CreateDepthStencilImageBuffer();
    // Initialize render pass resources for default color and depth attachments which contain 2 attachment, 2 attachment reference, 1 subpass(at index 0) and 2 subpass dependencies
    void InitRenderPass();
    // Create render pass
    void SetUpRenderPass();
    // Create frame buffer
    void CreateFrameBuffers();
    // Recreate swap chain resources
    void RecreateSwapChainResources(uint32_t width, uint32_t height);

    inline const VkSurfaceKHR GetSurface() const { return m_Surface; }
    inline VkSurfaceKHR GetSurface() { return m_Surface; }
    inline const VkSwapchainKHR GetSwapChain() const { return m_SwapChain; }
    inline VkSwapchainKHR GetSwapChain() { return m_SwapChain; }
    inline const VkExtent2D GetImageExtent() const { return m_SwapChainImageExtent; }
    inline VkExtent2D GetImageExtent() { return m_SwapChainImageExtent; }
    inline const uint32_t GetImageCount() const { return m_SwapChainImageCount; }
    inline uint32_t GetImageCount() { return m_SwapChainImageCount; }
    inline const VkFormat GetImageFormat() const { return m_SwapChainImageFormat; }
    inline VkFormat GetImageFormat() { return m_SwapChainImageFormat; }
    inline const VkRenderPass GetRenderPass() const { return m_RenderPass; }
    inline VkRenderPass GetRenderPass() { return m_RenderPass; }
    inline const VkFramebuffer GetFrameBuffer(uint32_t index) const { return m_FrameBuffers[index]; }
    inline VkFramebuffer GetFrameBuffer(uint32_t index) { return m_FrameBuffers[index]; }

    // Destroy frame buffer
    void DestroyFrameBuffers();
    // Destroy render pass
    void DestroyRenderPass();
    // Destroy image buffer(destroy image, free memory, destroy image view)
    void DestroyImageBuffer(SwapChainImageBuffer *pBuffers, size_t bufferCount);
    // Destroy swap chain and image view
    void CleanUpSwapChain();
    // Destroy surface
    void DestroySurface();
};

#endif
