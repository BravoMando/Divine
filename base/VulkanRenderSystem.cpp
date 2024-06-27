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

#include "VulkanRenderSystem.h"
#include "VulkanTools.h"
#include "VulkanInitializer.hpp"

#include <fstream>

VkDescriptorSetLayout VulkanRenderSystem::s_GlobalDescriptorSetLayout = VK_NULL_HANDLE;
VkDescriptorPool VulkanRenderSystem::s_GlobalDescriptorPool = VK_NULL_HANDLE;
std::vector<VkDescriptorSet> VulkanRenderSystem::s_GlobalDescriptorSets = {};

VulkanRenderSystem::VulkanRenderSystem(const VkAllocationCallbacks *pAllocator)
{
    p_Allocator = pAllocator;
}

VulkanRenderSystem::~VulkanRenderSystem()
{
    for (size_t i = 0; i < m_ShaderModules.size(); ++i)
    {
        vkDestroyShaderModule(m_Device, m_ShaderModules[i], p_Allocator);
    }
}

VulkanRenderSystem &VulkanRenderSystem::InitSystem(uint32_t maxFramesInFlight, VkDevice device)
{
    if (device == VK_NULL_HANDLE)
    {
        FATAL("Device must be valid!");
    }
    if (m_Device != VK_NULL_HANDLE)
    {
        FATAL("Render system: 0x%p had already been intialized!", this);
    }
    if (VulkanRenderSystem::s_GlobalDescriptorSetLayout == VK_NULL_HANDLE &&
        VulkanRenderSystem::s_GlobalDescriptorPool == VK_NULL_HANDLE &&
        VulkanRenderSystem::s_GlobalDescriptorSets.size() == 0)
    {
        VulkanRenderSystem::s_GlobalDescriptorSets.resize(maxFramesInFlight);
    }
    m_Device = device;

    return *this;
}

VulkanRenderSystem &VulkanRenderSystem::AddSetLayoutBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags stageFlags, const VkSampler *pImmutableSamplers)
{
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = descriptorType;
    layoutBinding.descriptorCount = descriptorCount;
    layoutBinding.stageFlags = stageFlags;
    layoutBinding.pImmutableSamplers = pImmutableSamplers;
    m_SetLayoutBindings.push_back(layoutBinding);

    return *this;
}

VkDescriptorSetLayout VulkanRenderSystem::BuildDescriptorSetLayout(VkDescriptorSetLayoutCreateFlags flags)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayoutCreateInfo layoutCI = vkinfo::SetLayoutInfo();
    layoutCI.bindingCount = static_cast<uint32_t>(m_SetLayoutBindings.size());
    layoutCI.pBindings = m_SetLayoutBindings.data();
    layoutCI.flags = flags;
    CHECK_VK_RESULT(vkCreateDescriptorSetLayout(m_Device, &layoutCI, p_Allocator, &setLayout));
    INFO("Descriptor set layout %p is created with %u bindings!\n", setLayout, layoutCI.bindingCount);
    m_SetLayoutBindings.clear();
    return setLayout;
}

void VulkanRenderSystem::DestroyDescriptorSetLayout(VkDescriptorSetLayout setLayout)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    vkDestroyDescriptorSetLayout(m_Device, setLayout, p_Allocator);
}

VulkanRenderSystem &VulkanRenderSystem::SetMaxSets(uint32_t maxSets)
{
    m_MaxSets = maxSets;
    return *this;
}

VulkanRenderSystem &VulkanRenderSystem::AddPoolSize(VkDescriptorType descriptorType, uint32_t count)
{
    m_PoolSize.push_back({descriptorType, count});
    return *this;
}

VkDescriptorPool VulkanRenderSystem::BuildDescriptorPool(VkDescriptorPoolCreateFlags flags)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    if (m_MaxSets == 0)
    {
        FATAL("Max sets is 0! You may forget to set max sets!");
    }

    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkDescriptorPoolCreateInfo poolCI = vkinfo::DescripotrPoolInfo(m_MaxSets);
    poolCI.poolSizeCount = static_cast<uint32_t>(m_PoolSize.size());
    poolCI.pPoolSizes = m_PoolSize.data();
    poolCI.flags = flags;
    CHECK_VK_RESULT(vkCreateDescriptorPool(m_Device, &poolCI, p_Allocator, &pool));
    INFO("Descriptor pool %p is created with %u max sets and %u pool sizes!\n", pool, m_MaxSets, poolCI.poolSizeCount);

    m_MaxSets = 0;
    m_PoolSize.clear();
    return pool;
}

void VulkanRenderSystem::DestroyDescriptorPool(VkDescriptorPool pool)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    vkDestroyDescriptorPool(m_Device, pool, p_Allocator);
}

void VulkanRenderSystem::AllocateDescriptorSets(VkDescriptorPool pool, VkDescriptorSetLayout setLayout, VkDescriptorSet *pDescriptorSets, size_t setCount)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    if (pool == VK_NULL_HANDLE || setLayout == VK_NULL_HANDLE)
    {
        FATAL("Descriptor pool and descriptor set layout must be valid!");
    }

    std::vector<VkDescriptorSetLayout> setLayouts(setCount, setLayout);
    VkDescriptorSetAllocateInfo allocInfo = vkinfo::DescriptorSetAllocateInfo(pool, setLayouts.data(), setCount);
    CHECK_VK_RESULT(vkAllocateDescriptorSets(m_Device, &allocInfo, pDescriptorSets));
}

void VulkanRenderSystem::FreeDescriptorSets(VkDescriptorPool pool, VkDescriptorSet *pDescriptorSets, size_t setCount)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    if (pool == VK_NULL_HANDLE)
    {
        FATAL("Descriptor Pool must be valid!");
    }
    CHECK_VK_RESULT(vkFreeDescriptorSets(m_Device, pool, static_cast<uint32_t>(setCount), pDescriptorSets));
}

void VulkanRenderSystem::ResetDescriptorPool(VkDescriptorPool pool, VkDescriptorPoolCreateFlags flags)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    if (pool == VK_NULL_HANDLE)
    {
        FATAL("Descriptor Pool must be valid!");
    }
    CHECK_VK_RESULT(vkResetDescriptorPool(m_Device, pool, flags));
}

void VulkanRenderSystem::WriteDescriptorSets(VkDescriptorType descriptorType, VkDescriptorSet set, uint32_t binding, void *pInfo, size_t dstArrayElement, size_t descriptorCount)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }

    VkWriteDescriptorSet write = vkinfo::DescriptorWriteInfo(descriptorType, set, binding, dstArrayElement, descriptorCount);
    switch (descriptorType)
    {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    {
        write.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo *>(pInfo);
        m_Writes.push_back(write);
    }
    break;

    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
    {
        write.pImageInfo = reinterpret_cast<VkDescriptorImageInfo *>(pInfo);
        m_Writes.push_back(write);
    }
    break;

    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    {
        write.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo *>(pInfo);
        m_Writes.push_back(write);
    }
    break;

    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
    {
        write.pImageInfo = reinterpret_cast<VkDescriptorImageInfo *>(pInfo);
        m_Writes.push_back(write);
    }
    break;

    default:
        break;
    }
}

void VulkanRenderSystem::UpdateDescriptorSets()
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(m_Writes.size()), m_Writes.data(), 0, nullptr);
    m_Writes.clear();
}

VkPipelineLayout VulkanRenderSystem::BuildPipelineLayout(size_t pushConstantCount, const VkPushConstantRange *pPushConstant, VkDescriptorSetLayout *pSetLayouts, size_t setLayoutCount)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayoutCreateInfo layoutCI = vkinfo::PipelineLayoutInfo(pushConstantCount, pPushConstant, static_cast<uint32_t>(setLayoutCount), pSetLayouts);
    CHECK_VK_RESULT(vkCreatePipelineLayout(m_Device, &layoutCI, p_Allocator, &pipelineLayout));

    return pipelineLayout;
}

void VulkanRenderSystem::DestroyPipelineLayout(VkPipelineLayout pipelineLayout)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    vkDestroyPipelineLayout(m_Device, pipelineLayout, p_Allocator);
}

void VulkanRenderSystem::MakeDefaultGraphicsPipelineConfigInfo(PipelineConfigInfo *pConfigInfo,
                                                               VkPipelineLayout pipelineLayout,
                                                               VkRenderPass renderPass,
                                                               uint32_t subpass,
                                                               const std::vector<VkVertexInputBindingDescription> &vertexBindingDescription,
                                                               const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescription,
                                                               VkPipeline basePipeline,
                                                               int32_t basePipelineIndex)
{
    if (pipelineLayout == VK_NULL_HANDLE || renderPass == VK_NULL_HANDLE)
    {
        FATAL("Pipeline layout and render pass must be valid!");
    }
    if (pConfigInfo == nullptr)
    {
        FATAL("No valid PipelineConfigInfo provided!");
    }

    // Vertex input
    pConfigInfo->VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pConfigInfo->VertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescription.size());
    pConfigInfo->VertexInputInfo.pVertexBindingDescriptions = vertexBindingDescription.data();
    pConfigInfo->VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescription.size());
    pConfigInfo->VertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

    // Input assembly
    pConfigInfo->InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pConfigInfo->InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pConfigInfo->InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // Tessellation
    pConfigInfo->TessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    // TODO: Tessellation state info

    // Dynamic viewport and scissor
    pConfigInfo->ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pConfigInfo->ViewportInfo.viewportCount = 1;
    pConfigInfo->ViewportInfo.pViewports = nullptr;
    pConfigInfo->ViewportInfo.scissorCount = 1;
    pConfigInfo->ViewportInfo.pScissors = nullptr;

    pConfigInfo->RasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pConfigInfo->RasterizationInfo.depthClampEnable = VK_FALSE;
    pConfigInfo->RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    pConfigInfo->RasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    pConfigInfo->RasterizationInfo.lineWidth = 1.0f;
    pConfigInfo->RasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    pConfigInfo->RasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pConfigInfo->RasterizationInfo.depthBiasEnable = VK_FALSE;
    pConfigInfo->RasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
    pConfigInfo->RasterizationInfo.depthBiasClamp = 0.0f;          // Optional
    pConfigInfo->RasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional

    pConfigInfo->MultisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pConfigInfo->MultisampleInfo.sampleShadingEnable = VK_FALSE;
    pConfigInfo->MultisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Optional
    pConfigInfo->MultisampleInfo.minSampleShading = 1.0f;                      // Optional
    pConfigInfo->MultisampleInfo.pSampleMask = nullptr;                        // Optional
    pConfigInfo->MultisampleInfo.alphaToCoverageEnable = VK_FALSE;             // Optional
    pConfigInfo->MultisampleInfo.alphaToOneEnable = VK_FALSE;                  // Optional

    pConfigInfo->DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pConfigInfo->DepthStencilInfo.depthTestEnable = VK_TRUE;
    pConfigInfo->DepthStencilInfo.depthWriteEnable = VK_TRUE;
    pConfigInfo->DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    pConfigInfo->DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    pConfigInfo->DepthStencilInfo.minDepthBounds = 0.0f; // Optional
    pConfigInfo->DepthStencilInfo.maxDepthBounds = 1.0f; // Optional
    pConfigInfo->DepthStencilInfo.stencilTestEnable = VK_FALSE;
    pConfigInfo->DepthStencilInfo.front = {}; // Optional
    pConfigInfo->DepthStencilInfo.back = {};  // Optional

    pConfigInfo->ColorBlendAttachments.resize(1);
    pConfigInfo->ColorBlendAttachments[0].colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pConfigInfo->ColorBlendAttachments[0].blendEnable = VK_FALSE;
    pConfigInfo->ColorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    pConfigInfo->ColorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    pConfigInfo->ColorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;             // Optional
    pConfigInfo->ColorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    pConfigInfo->ColorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    pConfigInfo->ColorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

    pConfigInfo->ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pConfigInfo->ColorBlendInfo.logicOpEnable = VK_FALSE;
    pConfigInfo->ColorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    pConfigInfo->ColorBlendInfo.attachmentCount = static_cast<uint32_t>(pConfigInfo->ColorBlendAttachments.size());
    pConfigInfo->ColorBlendInfo.pAttachments = pConfigInfo->ColorBlendAttachments.data();
    pConfigInfo->ColorBlendInfo.blendConstants[0] = 0.0f; // Optional
    pConfigInfo->ColorBlendInfo.blendConstants[1] = 0.0f; // Optional
    pConfigInfo->ColorBlendInfo.blendConstants[2] = 0.0f; // Optional
    pConfigInfo->ColorBlendInfo.blendConstants[3] = 0.0f; // Optional

    pConfigInfo->DynamicStatesEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    pConfigInfo->DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pConfigInfo->DynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(pConfigInfo->DynamicStatesEnables.size());
    pConfigInfo->DynamicStateInfo.pDynamicStates = pConfigInfo->DynamicStatesEnables.data();

    pConfigInfo->PipelineLayout = pipelineLayout;
    pConfigInfo->RenderPass = renderPass;
    pConfigInfo->Subpass = subpass;
    pConfigInfo->BasePipelineHandle = basePipeline;
    pConfigInfo->BasePipelineIndex = basePipelineIndex;
}

void VulkanRenderSystem::MakeDefaultComputePipelineConfigInfo(PipelineConfigInfo *pConfigInfo,
                                                              VkPipelineLayout pipelineLayout,
                                                              VkRenderPass renderPass,
                                                              uint32_t subpass,
                                                              const std::vector<VkVertexInputBindingDescription> &vertexBindingDescription,
                                                              const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescription,
                                                              VkPipeline basePipeline,
                                                              int32_t basePipelineIndex)
{
    if (pipelineLayout == VK_NULL_HANDLE || renderPass == VK_NULL_HANDLE)
    {
        FATAL("Pipeline layout and render pass must be valid!");
    }
    if (pConfigInfo == nullptr)
    {
        FATAL("No valid PipelineConfigInfo provided!");
    }

    // Vertex input
    pConfigInfo->VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pConfigInfo->VertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescription.size());
    pConfigInfo->VertexInputInfo.pVertexBindingDescriptions = vertexBindingDescription.data();
    pConfigInfo->VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescription.size());
    pConfigInfo->VertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

    // Input assembly
    pConfigInfo->InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pConfigInfo->InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pConfigInfo->InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // Tessellation
    pConfigInfo->TessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    // TODO: Tessellation state info

    // Dynamic viewport and scissor
    pConfigInfo->ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pConfigInfo->ViewportInfo.viewportCount = 1;
    pConfigInfo->ViewportInfo.pViewports = nullptr;
    pConfigInfo->ViewportInfo.scissorCount = 1;
    pConfigInfo->ViewportInfo.pScissors = nullptr;

    pConfigInfo->RasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pConfigInfo->RasterizationInfo.depthClampEnable = VK_FALSE;
    pConfigInfo->RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    pConfigInfo->RasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    pConfigInfo->RasterizationInfo.lineWidth = 1.0f;
    pConfigInfo->RasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    pConfigInfo->RasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    pConfigInfo->RasterizationInfo.depthBiasEnable = VK_FALSE;
    pConfigInfo->RasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
    pConfigInfo->RasterizationInfo.depthBiasClamp = 0.0f;          // Optional
    pConfigInfo->RasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional

    pConfigInfo->MultisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pConfigInfo->MultisampleInfo.sampleShadingEnable = VK_FALSE;
    pConfigInfo->MultisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Optional
    pConfigInfo->MultisampleInfo.minSampleShading = 1.0f;                      // Optional
    pConfigInfo->MultisampleInfo.pSampleMask = nullptr;                        // Optional
    pConfigInfo->MultisampleInfo.alphaToCoverageEnable = VK_FALSE;             // Optional
    pConfigInfo->MultisampleInfo.alphaToOneEnable = VK_FALSE;                  // Optional

    pConfigInfo->DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pConfigInfo->DepthStencilInfo.depthTestEnable = VK_TRUE;
    pConfigInfo->DepthStencilInfo.depthWriteEnable = VK_TRUE;
    pConfigInfo->DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    pConfigInfo->DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    pConfigInfo->DepthStencilInfo.minDepthBounds = 0.0f; // Optional
    pConfigInfo->DepthStencilInfo.maxDepthBounds = 1.0f; // Optional
    pConfigInfo->DepthStencilInfo.stencilTestEnable = VK_FALSE;
    pConfigInfo->DepthStencilInfo.front = {}; // Optional
    pConfigInfo->DepthStencilInfo.back = {};  // Optional

    pConfigInfo->ColorBlendAttachments.resize(1);
    pConfigInfo->ColorBlendAttachments[0].colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pConfigInfo->ColorBlendAttachments[0].blendEnable = VK_FALSE;
    pConfigInfo->ColorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    pConfigInfo->ColorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    pConfigInfo->ColorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;             // Optional
    pConfigInfo->ColorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    pConfigInfo->ColorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    pConfigInfo->ColorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

    pConfigInfo->ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pConfigInfo->ColorBlendInfo.logicOpEnable = VK_FALSE;
    pConfigInfo->ColorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    pConfigInfo->ColorBlendInfo.attachmentCount = static_cast<uint32_t>(pConfigInfo->ColorBlendAttachments.size());
    pConfigInfo->ColorBlendInfo.pAttachments = pConfigInfo->ColorBlendAttachments.data();
    pConfigInfo->ColorBlendInfo.blendConstants[0] = 0.0f; // Optional
    pConfigInfo->ColorBlendInfo.blendConstants[1] = 0.0f; // Optional
    pConfigInfo->ColorBlendInfo.blendConstants[2] = 0.0f; // Optional
    pConfigInfo->ColorBlendInfo.blendConstants[3] = 0.0f; // Optional

    pConfigInfo->DynamicStatesEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    pConfigInfo->DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pConfigInfo->DynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(pConfigInfo->DynamicStatesEnables.size());
    pConfigInfo->DynamicStateInfo.pDynamicStates = pConfigInfo->DynamicStatesEnables.data();

    pConfigInfo->PipelineLayout = pipelineLayout;
    pConfigInfo->RenderPass = renderPass;
    pConfigInfo->Subpass = subpass;
    pConfigInfo->BasePipelineHandle = basePipeline;
    pConfigInfo->BasePipelineIndex = basePipelineIndex;
}

void VulkanRenderSystem::MakeDefaultRayTracingPipelineConfigInfo(PipelineConfigInfo *pConfigInfo,
                                                                 VkPipelineLayout pipelineLayout,
                                                                 VkRenderPass renderPass,
                                                                 uint32_t subpass,
                                                                 const std::vector<VkVertexInputBindingDescription> &vertexBindingDescription,
                                                                 const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescription,
                                                                 VkPipeline basePipeline,
                                                                 int32_t basePipelineIndex)
{
    if (pipelineLayout == VK_NULL_HANDLE || renderPass == VK_NULL_HANDLE)
    {
        FATAL("Pipeline layout and render pass must be valid!");
    }
    if (pConfigInfo == nullptr)
    {
        FATAL("No valid PipelineConfigInfo provided!");
    }

    // Vertex input
    pConfigInfo->VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pConfigInfo->VertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescription.size());
    pConfigInfo->VertexInputInfo.pVertexBindingDescriptions = vertexBindingDescription.data();
    pConfigInfo->VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescription.size());
    pConfigInfo->VertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

    // Input assembly
    pConfigInfo->InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pConfigInfo->InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pConfigInfo->InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // Tessellation
    pConfigInfo->TessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    // TODO: Tessellation state info

    // Dynamic viewport and scissor
    pConfigInfo->ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pConfigInfo->ViewportInfo.viewportCount = 1;
    pConfigInfo->ViewportInfo.pViewports = nullptr;
    pConfigInfo->ViewportInfo.scissorCount = 1;
    pConfigInfo->ViewportInfo.pScissors = nullptr;

    pConfigInfo->RasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pConfigInfo->RasterizationInfo.depthClampEnable = VK_FALSE;
    pConfigInfo->RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    pConfigInfo->RasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    pConfigInfo->RasterizationInfo.lineWidth = 1.0f;
    pConfigInfo->RasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    pConfigInfo->RasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    pConfigInfo->RasterizationInfo.depthBiasEnable = VK_FALSE;
    pConfigInfo->RasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
    pConfigInfo->RasterizationInfo.depthBiasClamp = 0.0f;          // Optional
    pConfigInfo->RasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional

    pConfigInfo->MultisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pConfigInfo->MultisampleInfo.sampleShadingEnable = VK_FALSE;
    pConfigInfo->MultisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Optional
    pConfigInfo->MultisampleInfo.minSampleShading = 1.0f;                      // Optional
    pConfigInfo->MultisampleInfo.pSampleMask = nullptr;                        // Optional
    pConfigInfo->MultisampleInfo.alphaToCoverageEnable = VK_FALSE;             // Optional
    pConfigInfo->MultisampleInfo.alphaToOneEnable = VK_FALSE;                  // Optional

    pConfigInfo->DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pConfigInfo->DepthStencilInfo.depthTestEnable = VK_TRUE;
    pConfigInfo->DepthStencilInfo.depthWriteEnable = VK_TRUE;
    pConfigInfo->DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    pConfigInfo->DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    pConfigInfo->DepthStencilInfo.minDepthBounds = 0.0f; // Optional
    pConfigInfo->DepthStencilInfo.maxDepthBounds = 1.0f; // Optional
    pConfigInfo->DepthStencilInfo.stencilTestEnable = VK_FALSE;
    pConfigInfo->DepthStencilInfo.front = {}; // Optional
    pConfigInfo->DepthStencilInfo.back = {};  // Optional

    pConfigInfo->ColorBlendAttachments.resize(1);
    pConfigInfo->ColorBlendAttachments[0].colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pConfigInfo->ColorBlendAttachments[0].blendEnable = VK_FALSE;
    pConfigInfo->ColorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    pConfigInfo->ColorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    pConfigInfo->ColorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;             // Optional
    pConfigInfo->ColorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    pConfigInfo->ColorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    pConfigInfo->ColorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

    pConfigInfo->ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pConfigInfo->ColorBlendInfo.logicOpEnable = VK_FALSE;
    pConfigInfo->ColorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    pConfigInfo->ColorBlendInfo.attachmentCount = static_cast<uint32_t>(pConfigInfo->ColorBlendAttachments.size());
    pConfigInfo->ColorBlendInfo.pAttachments = pConfigInfo->ColorBlendAttachments.data();
    pConfigInfo->ColorBlendInfo.blendConstants[0] = 0.0f; // Optional
    pConfigInfo->ColorBlendInfo.blendConstants[1] = 0.0f; // Optional
    pConfigInfo->ColorBlendInfo.blendConstants[2] = 0.0f; // Optional
    pConfigInfo->ColorBlendInfo.blendConstants[3] = 0.0f; // Optional

    pConfigInfo->DynamicStatesEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    pConfigInfo->DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pConfigInfo->DynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(pConfigInfo->DynamicStatesEnables.size());
    pConfigInfo->DynamicStateInfo.pDynamicStates = pConfigInfo->DynamicStatesEnables.data();

    pConfigInfo->PipelineLayout = pipelineLayout;
    pConfigInfo->RenderPass = renderPass;
    pConfigInfo->Subpass = subpass;
    pConfigInfo->BasePipelineHandle = basePipeline;
    pConfigInfo->BasePipelineIndex = basePipelineIndex;
}

VulkanRenderSystem &VulkanRenderSystem::BuildShaderStage(const std::string &path, VkShaderStageFlagBits stage)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    if (path.empty())
    {
        FATAL("Can't build graphics pipeline without shaders!");
    }

    size_t bufferSize = 0;
    std::vector<char> buffer = {};
    std::ifstream ifs{};
    ifs.open(path, std::ios::ate | std::ios::binary);
    if (!ifs.is_open())
    {
        FATAL("Faild to open file: %s", path.c_str());
    }
    bufferSize = static_cast<size_t>(ifs.tellg());
    buffer.resize(bufferSize);
    ifs.seekg(0);
    ifs.read(buffer.data(), bufferSize);
    ifs.close();
    VkShaderModule shader = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo shaderCI = vkinfo::ShaderModuleInfo(buffer.size(), buffer.data());
    CHECK_VK_RESULT(vkCreateShaderModule(m_Device, &shaderCI, p_Allocator, &shader));
    m_ShaderModules.push_back(shader);

    // TODO: Add shader specialization
    // VkSpecializationMapEntry vertSpecialMapEntry{};
    // VkSpecializationInfo vertSpecial{};
    VkPipelineShaderStageCreateInfo shaderStage = vkinfo::ShaderStageInfo();
    shaderStage.stage = stage;
    shaderStage.module = shader;
    shaderStage.pName = "main";
    m_ShaderStages.push_back(shaderStage);

    return *this;
}

VkPipeline VulkanRenderSystem::BuildGraphicsPipeline(PipelineConfigInfo *pConfigInfo)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    if (pConfigInfo == nullptr)
    {
        FATAL("No valid PipelineConfigInfo provided!");
    }

    VkPipeline pipeline = VK_NULL_HANDLE;

    VkGraphicsPipelineCreateInfo graphicsPipelineCI = vkinfo::GraphicsPipelineInfo();
    graphicsPipelineCI.pInputAssemblyState = &pConfigInfo->InputAssemblyInfo;
    graphicsPipelineCI.pVertexInputState = &pConfigInfo->VertexInputInfo;
    graphicsPipelineCI.stageCount = static_cast<uint32_t>(m_ShaderStages.size());
    graphicsPipelineCI.pStages = m_ShaderStages.data();
    graphicsPipelineCI.pTessellationState = &pConfigInfo->TessellationInfo;
    graphicsPipelineCI.pViewportState = &pConfigInfo->ViewportInfo;
    graphicsPipelineCI.pRasterizationState = &pConfigInfo->RasterizationInfo;
    graphicsPipelineCI.pMultisampleState = &pConfigInfo->MultisampleInfo;
    graphicsPipelineCI.pDepthStencilState = &pConfigInfo->DepthStencilInfo;
    graphicsPipelineCI.pColorBlendState = &pConfigInfo->ColorBlendInfo;
    graphicsPipelineCI.pDynamicState = &pConfigInfo->DynamicStateInfo;
    graphicsPipelineCI.layout = pConfigInfo->PipelineLayout;
    graphicsPipelineCI.renderPass = pConfigInfo->RenderPass;
    graphicsPipelineCI.subpass = pConfigInfo->Subpass;
    graphicsPipelineCI.basePipelineHandle = pConfigInfo->BasePipelineHandle;
    graphicsPipelineCI.basePipelineIndex = pConfigInfo->BasePipelineIndex;

    CHECK_VK_RESULT(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineCI, p_Allocator, &pipeline));
    INFO("Graphics pipeline %p is built with layout %p and %u shader stages!\n", pipeline, pConfigInfo->PipelineLayout, static_cast<uint32_t>(m_ShaderStages.size()));

    for (size_t i = 0; i < m_ShaderModules.size(); ++i)
    {
        vkDestroyShaderModule(m_Device, m_ShaderModules[i], p_Allocator);
    }
    m_ShaderModules.clear();
    m_ShaderStages.clear();

    return pipeline;
}

VkPipeline VulkanRenderSystem::BuildComputePipeline(VkPipelineLayout pipelineLayout)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    if (m_ShaderStages.size() != 1 || m_ShaderStages[0].stage != VK_SHADER_STAGE_COMPUTE_BIT)
    {
        FATAL("Shader stage does NOT match!");
    }

    VkPipeline pipeline = VK_NULL_HANDLE;

    VkComputePipelineCreateInfo computeCI = vkinfo::ComputePipelineInfo();
    computeCI.layout = pipelineLayout;
    computeCI.stage = m_ShaderStages.front();

    CHECK_VK_RESULT(vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &computeCI, p_Allocator, &pipeline));
    INFO("Compute pipeline %p is built with layout %p!\n", pipeline, pipelineLayout);

    for (size_t i = 0; i < m_ShaderModules.size(); ++i)
    {
        vkDestroyShaderModule(m_Device, m_ShaderModules[i], p_Allocator);
    }
    m_ShaderModules.clear();
    m_ShaderStages.clear();

    return pipeline;
}

VkPipeline VulkanRenderSystem::BuildRayTracingPipeline(PipelineConfigInfo *pConfigInfo,
                                                       const std::string &vertFilePath,
                                                       const std::string &fragFilePath)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    if (pConfigInfo == nullptr)
    {
        FATAL("No valid PipelineConfigInfo provided!");
    }
    if (vertFilePath.empty() && fragFilePath.empty())
    {
        FATAL("Can't build graphics pipeline without shaders!");
    }

    VkPipeline pipeline = VK_NULL_HANDLE;

    size_t bufferSize = 0;
    std::vector<char> buffer = {};
    std::ifstream ifs{};
    ifs.open(vertFilePath, std::ios::ate | std::ios::binary);
    if (!ifs.is_open())
    {
        FATAL("Faild to open file: %s", vertFilePath.c_str());
    }
    bufferSize = static_cast<size_t>(ifs.tellg());
    buffer.resize(bufferSize);
    ifs.seekg(0);
    ifs.read(buffer.data(), bufferSize);
    ifs.close();
    VkShaderModule vertShader = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo vertCI = vkinfo::ShaderModuleInfo(buffer.size(), buffer.data());
    CHECK_VK_RESULT(vkCreateShaderModule(m_Device, &vertCI, p_Allocator, &vertShader));
    bufferSize = 0;
    buffer.clear();

    ifs.open(fragFilePath, std::ios::ate | std::ios::binary);
    if (!ifs.is_open())
    {
        FATAL("Faild to open file: %s", fragFilePath.c_str());
    }
    bufferSize = static_cast<size_t>(ifs.tellg());
    buffer.resize(bufferSize);
    ifs.seekg(0);
    ifs.read(buffer.data(), bufferSize);
    ifs.close();
    VkShaderModule fragShader = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo fragCI = vkinfo::ShaderModuleInfo(buffer.size(), buffer.data());
    CHECK_VK_RESULT(vkCreateShaderModule(m_Device, &fragCI, p_Allocator, &fragShader));
    bufferSize = 0;
    buffer.clear();

    // TODO: Add shader specialization
    // VkSpecializationMapEntry vertSpecialMapEntry{};
    // VkSpecializationInfo vertSpecial{};
    VkPipelineShaderStageCreateInfo vertShaderStage = vkinfo::ShaderStageInfo();
    vertShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStage.module = vertShader;
    vertShaderStage.pName = "main";

    // TODO: Add shader specialization
    // VkSpecializationMapEntry vertSpecialMapEntry{};
    // VkSpecializationInfo vertSpecial{};
    VkPipelineShaderStageCreateInfo fragShaderStage = vkinfo::ShaderStageInfo();
    fragShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStage.module = fragShader;
    fragShaderStage.pName = "main";

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStage, fragShaderStage};

    VkGraphicsPipelineCreateInfo graphicsPipelineCI = vkinfo::GraphicsPipelineInfo();
    graphicsPipelineCI.pInputAssemblyState = &pConfigInfo->InputAssemblyInfo;
    graphicsPipelineCI.pVertexInputState = &pConfigInfo->VertexInputInfo;
    graphicsPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    graphicsPipelineCI.pStages = shaderStages.data();
    graphicsPipelineCI.pTessellationState = &pConfigInfo->TessellationInfo;
    graphicsPipelineCI.pViewportState = &pConfigInfo->ViewportInfo;
    graphicsPipelineCI.pRasterizationState = &pConfigInfo->RasterizationInfo;
    graphicsPipelineCI.pMultisampleState = &pConfigInfo->MultisampleInfo;
    graphicsPipelineCI.pDepthStencilState = &pConfigInfo->DepthStencilInfo;
    graphicsPipelineCI.pColorBlendState = &pConfigInfo->ColorBlendInfo;
    graphicsPipelineCI.pDynamicState = &pConfigInfo->DynamicStateInfo;
    graphicsPipelineCI.layout = pConfigInfo->PipelineLayout;
    graphicsPipelineCI.renderPass = pConfigInfo->RenderPass;
    graphicsPipelineCI.subpass = pConfigInfo->Subpass;
    graphicsPipelineCI.basePipelineHandle = pConfigInfo->BasePipelineHandle;
    graphicsPipelineCI.basePipelineIndex = pConfigInfo->BasePipelineIndex;

    CHECK_VK_RESULT(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineCI, p_Allocator, &pipeline));
    INFO("Graphics pipeline %p is built with layout %p and %u shader stages!\n", pipeline, pConfigInfo->PipelineLayout, static_cast<uint32_t>(m_ShaderStages.size()));

    vkDestroyShaderModule(m_Device, vertShader, p_Allocator);
    vkDestroyShaderModule(m_Device, fragShader, p_Allocator);

    return pipeline;
}

void VulkanRenderSystem::DestroyPipeline(VkPipeline pipeline)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        FATAL("Render system must be initialized with valid device and render pass!");
    }
    vkDestroyPipeline(m_Device, pipeline, p_Allocator);
}
