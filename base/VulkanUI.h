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

#ifndef VULKAN_UI_HEADER
#define VULKAN_UI_HEADER

#include "VulkanCore.h"
#define OPM_ENABLE_HASH
#include "opm.hpp"
#include "vulkan/vulkan.h"
#include "VulkanMedium.hpp"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"

#include "imgui.h"

#include <string>
#include <array>
#include <vector>

class DVAPI_ATTR VulkanUI final
{
public:
    struct PushConstant
    {
        opm::vec2 Scale{1.0};
        opm::vec2 Translation{0.0};
    } m_PushConstanceBlock;
    float m_GlobalScale = 1.0f;
    bool m_Visible = true;
    bool m_Update = false;

private:
    const VkAllocationCallbacks *p_Allocator = nullptr;
    VulkanDevice *p_Device = nullptr;
    uint32_t m_MaxFramesInFlight = 0;
    std::vector<VulkanBuffer> m_VertexBuffers = {};
    std::vector<uint32_t> m_VertexCounts = {0};
    std::vector<VulkanBuffer> m_IndexBuffers = {};
    std::vector<uint32_t> m_IndexCounts = {0};
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_DescriptorSets;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VulkanTexture m_FontTexture;

public:
    explicit VulkanUI(uint32_t width,
                      uint32_t height,
                      uint32_t maxFramesInFlight,
                      VulkanDevice *pDevice,
                      const VkAllocationCallbacks *Allocator = nullptr);
    ~VulkanUI();
    VulkanUI(const VulkanUI &) = delete;
    VulkanUI &operator=(const VulkanUI &) = delete;
    VulkanUI(VulkanUI &&) = delete;
    VulkanUI &operator=(VulkanUI &&) = delete;

    // Prepare Vulkan descriptors for UI rendering
    void PrepareDescriptors();
    /**
     * @brief Prepare Vulkan pipeline for UI rendering.
     * @param colorFormat Used for dynamic rendering.
     * @param depthFormat Used for dynamic rendering.
     * @param stencilFormat Used for dynamic rendering.
     */
    void PreparePipeline(const std::string &vertFilePath,
                         const std::string &fragFilePath,
                         VkRenderPass renderPass,
                         uint32_t subpass,
                         VkFormat colorFormat,
                         VkFormat depthFormat,
                         VkFormat stencilFormat);
    // Update Dear ImGui vertex and index buffer
    void Update(uint32_t currentFrame);
    // Draw Dear ImGui data
    void Draw(VkCommandBuffer cmdBuffer, uint32_t currentFrame);
    // Handle window resize
    void Resize(uint32_t width, uint32_t height);
    // Free resources
    void FreeResources();
};

#endif
