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

#include "VulkanUI.h"
#include "VulkanTools.h"
#include "VulkanLogger.h"
#include "VulkanInitializer.hpp"

#include <fstream>

VulkanUI::VulkanUI(uint32_t width,
                   uint32_t height,
                   uint32_t maxFramesInFlight,
                   VulkanDevice *pDevice,
                   const VkAllocationCallbacks *Allocator)
    : p_Allocator{Allocator}, p_Device{pDevice}, m_MaxFramesInFlight{maxFramesInFlight}
{
    if (p_Device == nullptr)
    {
        FATAL("Vulkan device must be valid!");
    }

    m_VertexBuffers.resize(m_MaxFramesInFlight);
    m_VertexCounts.resize(m_MaxFramesInFlight);
    m_IndexBuffers.resize(m_MaxFramesInFlight);
    m_IndexCounts.resize(m_MaxFramesInFlight);
    m_DescriptorSets.resize(m_MaxFramesInFlight);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    io.FontGlobalScale = m_GlobalScale;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(m_GlobalScale);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
    style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
}

VulkanUI::~VulkanUI()
{
    FreeResources();
}

void VulkanUI::PrepareDescriptors()
{
    ImGuiIO &io = ImGui::GetIO();

    // Load font data
    unsigned char *fonts = nullptr;
    int width, height;
    io.Fonts->AddFontFromFileTTF(HOME_DIR "res/fonts/Roboto-Medium.ttf", 16.0f * m_GlobalScale);
    io.Fonts->GetTexDataAsRGBA32(&fonts, &width, &height);
    VkDeviceSize fontSize = width * height * sizeof(unsigned char) * 4;

    // Create font texture
    VkImageCreateInfo imageCI = vkinfo::ImageInfo();
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageCI.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    CHECK_VK_RESULT(vkCreateImage(p_Device->GetDevice(), &imageCI, p_Allocator, &m_FontTexture.Image));
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(p_Device->GetDevice(), m_FontTexture.Image, &memRequirements);
    VkMemoryAllocateInfo allocInfo = vkinfo::MemoryAllocInfo(memRequirements.size, FindMemoryTypeIndex(p_Device->GetGPU(), memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
    CHECK_VK_RESULT(vkAllocateMemory(p_Device->GetDevice(), &allocInfo, p_Allocator, &m_FontTexture.Memory));
    CHECK_VK_RESULT(vkBindImageMemory(p_Device->GetDevice(), m_FontTexture.Image, m_FontTexture.Memory, 0));

    VkImageViewCreateInfo viewCI = vkinfo::ImageViewInfo();
    viewCI.image = m_FontTexture.Image;
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCI.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCI.subresourceRange.baseArrayLayer = 0;
    viewCI.subresourceRange.layerCount = 1;
    viewCI.subresourceRange.baseMipLevel = 0;
    viewCI.subresourceRange.levelCount = 1;
    viewCI.components = {VK_COMPONENT_SWIZZLE_R,
                         VK_COMPONENT_SWIZZLE_G,
                         VK_COMPONENT_SWIZZLE_B,
                         VK_COMPONENT_SWIZZLE_A};
    CHECK_VK_RESULT(vkCreateImageView(p_Device->GetDevice(), &viewCI, p_Allocator, &m_FontTexture.View));

    VulkanBuffer staging;
    p_Device->CreateBuffer(fontSize,
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &staging,
                           fonts);

    VkCommandBuffer copyCmd = p_Device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    TransitionImageLayout(copyCmd,
                          m_FontTexture.Image,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          viewCI.subresourceRange,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy copyRegin{};
    copyRegin.imageExtent = imageCI.extent;
    copyRegin.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegin.imageSubresource.baseArrayLayer = 0;
    copyRegin.imageSubresource.layerCount = 1;
    copyRegin.imageSubresource.mipLevel = 0;
    vkCmdCopyBufferToImage(copyCmd,
                           staging.Buffer,
                           m_FontTexture.Image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &copyRegin);
    TransitionImageLayout(copyCmd,
                          m_FontTexture.Image,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          viewCI.subresourceRange,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    p_Device->FlushCommandBuffer(copyCmd, (p_Device->GetDeviceQueues())->Transfer);
    staging.Destroy();

    VkSamplerCreateInfo samplerCI = vkinfo::SamplerInfo();
    samplerCI.magFilter = VK_FILTER_LINEAR;
    samplerCI.minFilter = VK_FILTER_LINEAR;
    samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    CHECK_VK_RESULT(vkCreateSampler(p_Device->GetDevice(), &samplerCI, p_Allocator, &m_FontTexture.Sampler));

    m_FontTexture.Device = p_Device->GetDevice();
    m_FontTexture.IsInitialized = true;
    m_FontTexture.Allocator = p_Allocator;
    m_FontTexture.Width = 1;
    m_FontTexture.Height = 1;
    m_FontTexture.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    m_FontTexture.MipMapLevelCount = imageCI.mipLevels;
    m_FontTexture.ArrayLayerCount = 1;
    m_FontTexture.SetDescriptorImage();

    // Descriptors
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {vkinfo::SetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)};
    VkDescriptorSetLayoutCreateInfo setLayoutCI = vkinfo::SetLayoutInfo();
    setLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    setLayoutCI.pBindings = setLayoutBindings.data();
    CHECK_VK_RESULT(vkCreateDescriptorSetLayout(p_Device->GetDevice(), &setLayoutCI, p_Allocator, &m_DescriptorSetLayout));

    std::vector<VkDescriptorPoolSize> poolSizes = {vkinfo::PoolSize(m_MaxFramesInFlight, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)};
    VkDescriptorPoolCreateInfo poolCI = vkinfo::DescripotrPoolInfo(m_MaxFramesInFlight);
    poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCI.pPoolSizes = poolSizes.data();
    CHECK_VK_RESULT(vkCreateDescriptorPool(p_Device->GetDevice(), &poolCI, p_Allocator, &m_DescriptorPool));

    std::vector<VkDescriptorSetLayout> setLayouts(m_DescriptorSets.size(), m_DescriptorSetLayout);
    VkDescriptorSetAllocateInfo setAllocInfo = vkinfo::DescriptorSetAllocateInfo(m_DescriptorPool, setLayouts.data(), setLayouts.size());
    CHECK_VK_RESULT(vkAllocateDescriptorSets(p_Device->GetDevice(), &setAllocInfo, m_DescriptorSets.data()));

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    for (size_t i = 0; i < m_DescriptorSets.size(); ++i)
    {
        VkWriteDescriptorSet write = vkinfo::DescriptorWriteInfo(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_DescriptorSets[i], 0, 0, 1);
        write.pImageInfo = &m_FontTexture.DescriptorImageInfo;
        descriptorWrites.push_back(write);
    }
    vkUpdateDescriptorSets(p_Device->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void VulkanUI::PreparePipeline(const std::string &vertFilePath,
                               const std::string &fragFilePath,
                               VkRenderPass renderPass,
                               uint32_t subpass,
                               VkFormat colorFormat = VK_FORMAT_R32G32B32_SFLOAT,
                               VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT,
                               VkFormat stencilFormat = VK_FORMAT_D32_SFLOAT_S8_UINT)
{
    // Pipeline
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
    CHECK_VK_RESULT(vkCreateShaderModule(p_Device->GetDevice(), &vertCI, p_Allocator, &vertShader));
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
    CHECK_VK_RESULT(vkCreateShaderModule(p_Device->GetDevice(), &fragCI, p_Allocator, &fragShader));
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

    VkPushConstantRange pushConstantRange = vkinfo::PushConstant(0, sizeof(PushConstant), VK_SHADER_STAGE_VERTEX_BIT);
    VkPipelineLayoutCreateInfo pipelineLayoutCI = vkinfo::PipelineLayoutInfo(1, &pushConstantRange, 1, &m_DescriptorSetLayout);
    CHECK_VK_RESULT(vkCreatePipelineLayout(p_Device->GetDevice(), &pipelineLayoutCI, p_Allocator, &m_PipelineLayout));

    PipelineConfigInfo configInfo{};
    // Use default Dear ImGui vertex layout
    configInfo.BindingDescriptions = {vkinfo::VertexInputBinding(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX)};
    configInfo.AttributeDescriptions = {vkinfo::VertexInputAttribute(0, 0, offsetof(ImDrawVert, pos), VK_FORMAT_R32G32_SFLOAT),
                                        vkinfo::VertexInputAttribute(0, 1, offsetof(ImDrawVert, uv), VK_FORMAT_R32G32_SFLOAT),
                                        vkinfo::VertexInputAttribute(0, 2, offsetof(ImDrawVert, col), VK_FORMAT_R8G8B8A8_UNORM)};
    // enable color blend
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    // Dynamic states
    configInfo.DynamicStatesEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    configInfo.VertexInputInfo = vkinfo::VertexInputStateInfo(configInfo.BindingDescriptions, configInfo.AttributeDescriptions);
    configInfo.InputAssemblyInfo = vkinfo::InputAssemblyInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    // configInfo.TessellationInfo = vkinfo::TessellationInfo(...);
    configInfo.ViewportInfo = vkinfo::ViewportInfo(1, 1);
    configInfo.RasterizationInfo = vkinfo::RasterizationInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    configInfo.MultisampleInfo = vkinfo::MultisampleStateInfo(VK_SAMPLE_COUNT_1_BIT);
    configInfo.DepthStencilInfo = vkinfo::DepthStencilInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
    configInfo.ColorBlendInfo = vkinfo::ColorBlendStateInfo(1, &colorBlendAttachment);
    configInfo.DynamicStateInfo = vkinfo::DynamicStateInfo(static_cast<uint32_t>(configInfo.DynamicStatesEnables.size()), configInfo.DynamicStatesEnables.data());

    VkGraphicsPipelineCreateInfo pipelineCI = vkinfo::GraphicsPipelineInfo();
    pipelineCI.layout = m_PipelineLayout;
    pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCI.pStages = shaderStages.data();
    pipelineCI.pVertexInputState = &configInfo.VertexInputInfo;
    pipelineCI.pInputAssemblyState = &configInfo.InputAssemblyInfo;
    // pipelineCI.pTessellationState = &configInfo.TessellationInfo;
    pipelineCI.pViewportState = &configInfo.ViewportInfo;
    pipelineCI.pRasterizationState = &configInfo.RasterizationInfo;
    pipelineCI.pMultisampleState = &configInfo.MultisampleInfo;
    pipelineCI.pDepthStencilState = &configInfo.DepthStencilInfo;
    pipelineCI.pColorBlendState = &configInfo.ColorBlendInfo;
    pipelineCI.pDynamicState = &configInfo.DynamicStateInfo;
    pipelineCI.renderPass = renderPass;
    pipelineCI.subpass = subpass;

#if defined(VK_KHR_dynamic_rendering)
    // If we are using dynamic rendering (i.e. renderPass null), must define color, depth and stencil attachments at pipeline create time.
    if (renderPass == VK_NULL_HANDLE)
    {
        VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
        pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;
        pipelineRenderingCreateInfo.stencilAttachmentFormat = stencilFormat;
        pipelineCI.pNext = &pipelineRenderingCreateInfo;
    }
#endif

    CHECK_VK_RESULT(vkCreateGraphicsPipelines(p_Device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineCI, p_Allocator, &m_Pipeline));

    vkDestroyShaderModule(p_Device->GetDevice(), vertShader, p_Allocator);
    vkDestroyShaderModule(p_Device->GetDevice(), fragShader, p_Allocator);
}

void VulkanUI::Update(uint32_t currentFrame)
{
    ImDrawData *drawData = ImGui::GetDrawData();
    if (drawData == nullptr)
    {
        return;
    }

    VkDeviceSize vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
    VkDeviceSize indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

    if (vertexBufferSize == 0 || indexBufferSize == 0)
    {
        return;
    }

    if (m_VertexBuffers[currentFrame].Buffer == VK_NULL_HANDLE)
    {
        p_Device->CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &m_VertexBuffers[currentFrame]);
        m_VertexCounts[currentFrame] = drawData->TotalVtxCount;
        m_VertexBuffers[currentFrame].Map();
    }
    else if (m_VertexCounts[currentFrame] != drawData->TotalVtxCount)
    {
        m_VertexBuffers[currentFrame].Unmap();
        m_VertexBuffers[currentFrame].Destroy();
        p_Device->CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &m_VertexBuffers[currentFrame]);
        m_VertexCounts[currentFrame] = drawData->TotalVtxCount;
        m_VertexBuffers[currentFrame].Map();
    }

    if (m_IndexBuffers[currentFrame].Buffer == VK_NULL_HANDLE)
    {
        p_Device->CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &m_IndexBuffers[currentFrame]);
        m_IndexCounts[currentFrame] = drawData->TotalIdxCount;
        m_IndexBuffers[currentFrame].Map();
    }
    else if (m_IndexCounts[currentFrame] != drawData->TotalIdxCount)
    {
        m_IndexBuffers[currentFrame].Unmap();
        m_IndexBuffers[currentFrame].Destroy();
        p_Device->CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &m_IndexBuffers[currentFrame]);
        m_IndexCounts[currentFrame] = drawData->TotalIdxCount;
        m_IndexBuffers[currentFrame].Map();
    }

    // Update data
    ImDrawVert *vertexData = reinterpret_cast<ImDrawVert *>(m_VertexBuffers[currentFrame].Mapped);
    ImDrawIdx *indexData = reinterpret_cast<ImDrawIdx *>(m_IndexBuffers[currentFrame].Mapped);

    for (int i = 0; i < drawData->CmdListsCount; ++i)
    {
        const ImDrawList *cmd_list = drawData->CmdLists[i];
        memcpy(vertexData, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(indexData, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vertexData += cmd_list->VtxBuffer.Size;
        indexData += cmd_list->IdxBuffer.Size;
    }

    m_VertexBuffers[currentFrame].Flush();
    m_IndexBuffers[currentFrame].Flush();
}

void VulkanUI::Draw(VkCommandBuffer cmdBuffer, uint32_t currentFrame)
{
    if (cmdBuffer == VK_NULL_HANDLE)
    {
        FATAL("Command buffer must be valid!");
    }

    ImDrawData *drawData = ImGui::GetDrawData();
    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    if (drawData == nullptr || drawData->CmdListsCount == 0)
    {
        return;
    }

    ImGuiIO &io = ImGui::GetIO();

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[currentFrame], 0, nullptr);
    m_PushConstanceBlock.Scale = opm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
    m_PushConstanceBlock.Translation = opm::vec2(-1.0f);
    vkCmdPushConstants(cmdBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant), &m_PushConstanceBlock);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_VertexBuffers[currentFrame].Buffer, &offset);
    vkCmdBindIndexBuffer(cmdBuffer, m_IndexBuffers[currentFrame].Buffer, 0, VK_INDEX_TYPE_UINT16);

    for (int i = 0; i < drawData->CmdListsCount; ++i)
    {
        const ImDrawList *cmd_list = drawData->CmdLists[i];
        for (int j = 0; j < cmd_list->CmdBuffer.Size; ++j)
        {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
            VkRect2D scissor{};
            scissor.offset.x = std::max(static_cast<int>(pcmd->ClipRect.x), 0);
            scissor.offset.y = std::max(static_cast<int>(pcmd->ClipRect.y), 0);
            scissor.extent.width = static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x);
            scissor.extent.height = static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y);
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
            vkCmdDrawIndexed(cmdBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
            indexOffset += pcmd->ElemCount;
        }
        vertexOffset += cmd_list->VtxBuffer.Size;
    }
}

void VulkanUI::Resize(uint32_t width, uint32_t height)
{
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
}

void VulkanUI::FreeResources()
{
    for (uint32_t i = 0; i < m_MaxFramesInFlight; ++i)
    {
        m_VertexBuffers[i].Destroy();
        m_IndexBuffers[i].Destroy();
    }
    m_FontTexture.Destroy();

    vkDestroyDescriptorPool(p_Device->GetDevice(), m_DescriptorPool, p_Allocator);
    vkDestroyDescriptorSetLayout(p_Device->GetDevice(), m_DescriptorSetLayout, p_Allocator);
    vkDestroyPipeline(p_Device->GetDevice(), m_Pipeline, p_Allocator);
    vkDestroyPipelineLayout(p_Device->GetDevice(), m_PipelineLayout, p_Allocator);
}
