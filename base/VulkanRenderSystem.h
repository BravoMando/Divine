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

#ifndef VULKAN_RENDER_SYSTEM_HEADER
#define VULKAN_RENDER_SYSTEM_HEADER

#pragma once

#include "VulkanCore.h"
#include "VulkanMedium.hpp"

#include <string>
#include <vector>
#include <unordered_set>

class DVAPI_ATTR VulkanRenderSystem final
{
private:
    const VkAllocationCallbacks *p_Allocator = nullptr;
    // Device
    VkDevice m_Device = VK_NULL_HANDLE;

    std::vector<VkDescriptorSetLayoutBinding> m_SetLayoutBindings = {};
    uint32_t m_MaxSets = 0;
    std::vector<VkDescriptorPoolSize> m_PoolSize = {};
    std::vector<VkWriteDescriptorSet> m_Writes = {};
    std::vector<VkShaderModule> m_ShaderModules = {};
    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages = {};

private:
    static VkDescriptorSetLayout s_GlobalDescriptorSetLayout;
    static VkDescriptorPool s_GlobalDescriptorPool;
    static std::vector<VkDescriptorSet> s_GlobalDescriptorSets;

public:
    explicit VulkanRenderSystem(const VkAllocationCallbacks *pAllocator = nullptr);
    ~VulkanRenderSystem();
    VulkanRenderSystem(const VulkanRenderSystem &) = delete;
    VulkanRenderSystem &operator=(const VulkanRenderSystem &) = delete;
    VulkanRenderSystem(VulkanRenderSystem &&) = delete;
    VulkanRenderSystem &operator=(VulkanRenderSystem &&) = delete;

    VulkanRenderSystem &InitSystem(uint32_t maxFramesInFlight, VkDevice device);
    // Add descriptor set layout binding
    VulkanRenderSystem &AddSetLayoutBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags stageFlags, const VkSampler *pImmutableSamplers = nullptr);
    // Build descriptor set layout with added set layout bindings and clear bindings
    VkDescriptorSetLayout BuildDescriptorSetLayout(VkDescriptorSetLayoutCreateFlags flags = 0);
    // Destroy descriptor set layout
    void DestroyDescriptorSetLayout(VkDescriptorSetLayout setLayout);
    // Set the maximum number of descriptor set that can be allocated
    VulkanRenderSystem &SetMaxSets(uint32_t maxSets);
    // Add descriptor pool size, specify descriptor type and how many of them
    VulkanRenderSystem &AddPoolSize(VkDescriptorType descriptorType, uint32_t count);
    // Build descriptor pool and clear max sets and pool sizes
    VkDescriptorPool BuildDescriptorPool(VkDescriptorPoolCreateFlags flags);
    // Destroy descriptor pool
    void DestroyDescriptorPool(VkDescriptorPool pool);
    // Allocate descriptor sets from specified pool, and their layout and number of them
    void AllocateDescriptorSets(VkDescriptorPool pool, VkDescriptorSetLayout setLayout, VkDescriptorSet *pDescriptorSets, size_t setCount);
    // Free descriptor sets
    void FreeDescriptorSets(VkDescriptorPool pool, VkDescriptorSet *pDescriptorSets, size_t setCount);
    // Reset descriptor pool
    void ResetDescriptorPool(VkDescriptorPool pool, VkDescriptorPoolCreateFlags flags);
    // Record descriptor set data
    void WriteDescriptorSets(VkDescriptorType descriptorType, VkDescriptorSet set, uint32_t binding, void *pInfo, size_t dstArrayElement = 0, size_t descriptorCount = 1);
    // Update descriptor sets and clear writes
    void UpdateDescriptorSets();
    // Build pipeline layout
    VkPipelineLayout BuildPipelineLayout(size_t pushConstantCount, const VkPushConstantRange *pPushConstant, VkDescriptorSetLayout *pSetLayouts, size_t setLayoutCount);
    // Destroy pipeline layout
    void DestroyPipelineLayout(VkPipelineLayout pipelineLayout);
    /**
     * @brief Generate a default graphics pipeline configuration data.
     * @param pConfigInfo The address of the struct object.
     */
    void MakeDefaultGraphicsPipelineConfigInfo(PipelineConfigInfo *pConfigInfo,
                                               VkPipelineLayout pipelineLayout,
                                               VkRenderPass renderPass,
                                               uint32_t subpass,
                                               const std::vector<VkVertexInputBindingDescription> &vertexBindingDescription,
                                               const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescription,
                                               VkPipeline basePipeline = VK_NULL_HANDLE,
                                               int32_t basePipelineIndex = -1);
    /**
     * @brief Generate a default compute pipeline configuration data.
     * @param pConfigInfo The address of the struct object.
     */
    void MakeDefaultComputePipelineConfigInfo(PipelineConfigInfo *pConfigInfo,
                                              VkPipelineLayout pipelineLayout,
                                              VkRenderPass renderPass,
                                              uint32_t subpass,
                                              const std::vector<VkVertexInputBindingDescription> &vertexBindingDescription,
                                              const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescription,
                                              VkPipeline basePipeline = VK_NULL_HANDLE,
                                              int32_t basePipelineIndex = -1);
    /**
     * @brief Generate a default ray-tracing pipeline configuration data.
     * @param pConfigInfo The address of the struct object.
     */
    void MakeDefaultRayTracingPipelineConfigInfo(PipelineConfigInfo *pConfigInfo,
                                                 VkPipelineLayout pipelineLayout,
                                                 VkRenderPass renderPass,
                                                 uint32_t subpass,
                                                 const std::vector<VkVertexInputBindingDescription> &vertexBindingDescription,
                                                 const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescription,
                                                 VkPipeline basePipeline = VK_NULL_HANDLE,
                                                 int32_t basePipelineIndex = -1);
    /**
     * @brief Create shader module stages stored in shader cache.
     * @param path The compiled shader file path.
     * @param stage The pipeline shader stage specifying which stages are included in the pipeline.
     */
    VulkanRenderSystem &BuildShaderStage(const std::string &path, VkShaderStageFlagBits stage);
    /**
     * @brief Build graphics pipeline and clear shader cache.
     * @param pConfigInfo The address of the struct object.
     */
    VkPipeline BuildGraphicsPipeline(PipelineConfigInfo *pConfigInfo);
    /**
     * @brief Build compute pipeline and clear shader cache.
     * @param pipelineLayout The compute pipeline layout.
     * @note Creating compute pipeline only need one compute shader stage module.
     */
    VkPipeline BuildComputePipeline(VkPipelineLayout pipelineLayout);
    /**
     * @brief Build ray-tracing pipeline and clear shader cache.
     * @param pConfigInfo The address of the struct object.
     */
    VkPipeline BuildRayTracingPipeline(PipelineConfigInfo *pConfigInfo,
                                       const std::string &vertFilePath,
                                       const std::string &fragFilePath);
    // Destroy pipeline
    void DestroyPipeline(VkPipeline pipeline);

    static inline VkDescriptorSetLayout &GetGlobalDescriptorSetLayout()
    {
        return VulkanRenderSystem::s_GlobalDescriptorSetLayout;
    }

    static inline VkDescriptorPool &GetGlobalDescriptorPool()
    {
        return VulkanRenderSystem::s_GlobalDescriptorPool;
    }

    static inline VkDescriptorSet *GetGlobalDescriptorSets()
    {
        return VulkanRenderSystem::s_GlobalDescriptorSets.data();
    }

    static inline uint32_t GetDescriptorSetCount()
    {
        return static_cast<uint32_t>(VulkanRenderSystem::s_GlobalDescriptorSets.size());
    }

    static inline VkDescriptorSet &GetGlobalDescriptorSet(size_t idx)
    {
        return VulkanRenderSystem::s_GlobalDescriptorSets[idx];
    }
};

#endif
