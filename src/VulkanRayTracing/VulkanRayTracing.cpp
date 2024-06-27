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

#include "VulkanCore.h"
#include "VulkanInitializer.hpp"
#include "VulkanSceneObject.h"
#include "VulkanRenderer.h"

class VulkanComputeRayTracing : public VulkanRenderer
{
public:
    VulkanComputeRayTracing()
        : VulkanRenderer(CAMERA_TYPE_LOOK_AT, ALLOCATE_CALLBACK)
    {
        m_Settings.FullScreenMode = true;
        p_Camera->SetPosition({0.0, 0.0, 0.0});
        // p_Camera->SetDirection({1.0, 0.0, 0.0}); // Look at +X axis
    }

    ~VulkanComputeRayTracing()
    {
        for (size_t i = 0; i < m_ComputeFinishedSemaphores.size(); ++i)
        {
            vkDestroySemaphore(p_Device->GetDevice(), m_ComputeFinishedSemaphores[i], p_Allocator);
        }
        for (size_t i = 0; i < m_ComputeInFlightFences.size(); ++i)
        {
            vkDestroyFence(p_Device->GetDevice(), m_ComputeInFlightFences[i], p_Allocator);
        }
        if (p_Scene != nullptr)
        {
            delete p_Scene;
        }
        for (size_t i = 0; i < m_StorageTextures.size(); ++i)
        {
            m_StorageTextures[i].Destroy();
        }
        vkFreeCommandBuffers(p_Device->GetDevice(), m_ComputePool, static_cast<uint32_t>(m_ComputeCmdBuffers.size()), m_ComputeCmdBuffers.data());
        if (m_ComputePipeline != VK_NULL_HANDLE)
        {
            p_RenderSystem->DestroyPipeline(m_ComputePipeline);
        }
        if (m_ComputePipelineLayout != VK_NULL_HANDLE)
        {
            p_RenderSystem->DestroyPipelineLayout(m_ComputePipelineLayout);
        }
        p_Device->DestroyCommandPool(m_ComputePool);
        m_SkyBox.Destroy();
        if (m_GraphicsPipeline != VK_NULL_HANDLE)
        {
            p_RenderSystem->DestroyPipeline(m_GraphicsPipeline);
        }
        if (m_GraphicsLayout != VK_NULL_HANDLE)
        {
            p_RenderSystem->DestroyPipelineLayout(m_GraphicsLayout);
        }
        if (p_GraphicsConfigInfo != nullptr)
        {
            delete p_GraphicsConfigInfo;
        }
        p_RenderSystem->DestroyDescriptorPool(VulkanRenderSystem::GetGlobalDescriptorPool());
        for (size_t i = 0; i < m_GraphicsSetLayouts.size(); ++i)
        {
            p_RenderSystem->DestroyDescriptorSetLayout(m_GraphicsSetLayouts[i]);
        }
        for (size_t i = 0; i < m_ComputeSetLayouts.size(); ++i)
        {
            p_RenderSystem->DestroyDescriptorSetLayout(m_ComputeSetLayouts[i]);
        }
    }

public:
    virtual void Prepare() override;
    virtual void RenderUI() override;
    virtual void CommitAllSubmits() override;
    virtual void Render() override;

private:
    void CreateRenderPasses();
    void CreateStorageBuffers();
    void CreateStorageImages();
    void LoadModels();
    void CreateDescriptorPool();
    void CreateComputePipeline();
    void CreateGraphicsPipeline();
    void CreateComputeCmdBuffers();
    void BuildComputeCmdBuffer();

private:
    // Graphics
    std::vector<VkDescriptorSetLayout> m_GraphicsSetLayouts = {};
    std::vector<VkDescriptorSet> m_GraphicsSets = {};
    PipelineConfigInfo *p_GraphicsConfigInfo = nullptr;
    VkPipelineLayout m_GraphicsLayout = VK_NULL_HANDLE;
    VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
    VulkanTexture m_SkyBox{};

    // Compute
    VulkanScene *p_Scene = nullptr;
    std::vector<VulkanTexture> m_StorageTextures = {};
    VkCommandPool m_ComputePool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_ComputeCmdBuffers{};
    std::vector<VkFence> m_ComputeInFlightFences{};
    std::vector<VkSemaphore> m_ComputeFinishedSemaphores{};
    std::vector<VkDescriptorSetLayout> m_ComputeSetLayouts = {};
    std::vector<VkDescriptorSet> m_ComputeSets = {};
    VkPipelineLayout m_ComputePipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_ComputePipeline = VK_NULL_HANDLE;
};

void VulkanComputeRayTracing::CreateRenderPasses()
{
    p_SwapChain->InitRenderPass();
    // p_SwapChain->m_RenderPassResource.Attachments.push_back(depthStencilAttachment);
    // p_SwapChain->m_RenderPassResource.References.push_back(...);
    // p_SwapChain->m_RenderPassResource.Subpasses.push_back(...);
    // p_SwapChain->m_RenderPassResource.Dependencies.push_back(...);
    p_SwapChain->SetUpRenderPass();
    // std::vector<SwapChainImageBuffer> imageBuffers;
    // p_SwapChain->CreateSwapChainImageBuffer(imageBuffers,...,false);
    // for (size_t i = 0; i < p_SwapChain->m_FrameBufferAttachments.size(); ++i)
    // {
    //     p_SwapChain->m_FrameBufferAttachments[i].push_back(imageView);
    // }
    p_SwapChain->CreateFrameBuffers();
}

void VulkanComputeRayTracing::CreateStorageBuffers()
{
    p_Scene->ResizeAllBuffers(1);
    CreateUniformBuffers(sizeof(p_Scene->SceneProperty), p_Scene->SceneBuffers.data(), p_Scene->SceneBuffers.size());

    opm::quat rotate = opm::RotateQuat(opm::radians(0.00005), {0.0, -1.0, 0.0});

    p_Scene->AddPointLight({0.0, -5.0, 10.0, 0.1}, {1.0}, 32.0, 0.22, 0.2, rotate)
        .BuildPointLightBuffer();
    p_Scene->AddDirectLight(opm::normalize({1.0, 1.0, -10.0}), {1.0, 1.0, 1.0}, 32.0, rotate)
        .BuildDirectLightBuffer();

    p_Scene->AddSphere({0.0, 512.0, 10.0, 512.0},
                       VulkanScene::CreateMaterial({0.5, 0.7, 0.4}, 16.0, 0.0)) // Ground
        .AddSphere({-2.0, -1.0, 10.0, 1.0},
                   VulkanScene::CreateMaterial({0.3, 0.5, 0.8}, 128.0, 0.5)) // Metal
        .AddSphere({2.0, -1.0, 10.0, 1.0},
                   VulkanScene::CreateMaterial({0.4, 0.7, 0.5}, 128.0, 0.2)) // Metal
        .AddSphere({0.0, -0.5, 8.0, 0.5},
                   VulkanScene::CreateMaterial({1.0, 0.0, 1.0}, 32.0, 0.0)) // Diffuse
        .AddSphere({0.0, -2.0, 4.0, 2.0},
                   VulkanScene::CreateMaterial({1.0, 1.0, 1.0}, 32.0, 0.2, 1.4), true) // Glass
        // .AddSphere({-1.0, -2.0, 64.0, 4.0},
        //            VulkanScene::CreateMaterial({1.0, 1.0, 1.0}, 2.0, 0.0, 0.0, 0)) // Marse
        // .AddSphere({2.0, -1.0, 128.0, 8.0},
        //            VulkanScene::CreateMaterial({1.0, 1.0, 1.0}, 2.0, 0.0, 0.0, 1)) // Jupiter
        .BuildSphereBuffer();
}

void VulkanComputeRayTracing::CreateStorageImages()
{
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(p_Device->GetGPU(), format, &formatProps);
    if ((formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) == 0)
    {
        FATAL("Image format tiling doesn't support storage!");
    }
    VkImageCreateInfo imageCI = vkinfo::ImageInfo();
    uint32_t width = m_Width + 16 - m_Width % 16;
    uint32_t height = m_Height + 16 - m_Height % 16;
    imageCI.extent = {width, height, 1U};
    imageCI.format = format;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

    m_StorageTextures.resize(m_Settings.MaxFramesInFlight);
    for (size_t i = 0; i < m_StorageTextures.size(); ++i)
    {
        p_SwapChain->CreateImageWithInfo(&imageCI, &m_StorageTextures[i].Image, &m_StorageTextures[i].Memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseArrayLayer = 0;
        range.layerCount = 1;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        VkCommandBuffer CmdBuffer = p_Device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        TransitionImageLayout(CmdBuffer,
                              m_StorageTextures[i].Image,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_GENERAL,
                              range);
        p_Device->FlushCommandBuffer(CmdBuffer, m_Queues.Transfer);

        VkSamplerCreateInfo samplerCI = vkinfo::SamplerInfo();
        samplerCI.magFilter = VK_FILTER_LINEAR;
        samplerCI.minFilter = VK_FILTER_LINEAR;
        samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerCI.mipLodBias = 0.0f;
        samplerCI.maxAnisotropy = 1.0f;
        samplerCI.compareEnable = VK_FALSE;
        samplerCI.compareOp = VK_COMPARE_OP_NEVER;
        samplerCI.maxLod = static_cast<float>(imageCI.mipLevels);
        samplerCI.minLod = 0.0f;
        samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        CHECK_VK_RESULT(vkCreateSampler(p_Device->GetDevice(), &samplerCI, p_Allocator, &m_StorageTextures[i].Sampler));

        VkImageViewCreateInfo viewCI = vkinfo::ImageViewInfo();
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format = format;
        viewCI.image = m_StorageTextures[i].Image;
        viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount = 1;
        viewCI.subresourceRange.baseMipLevel = 0;
        viewCI.subresourceRange.levelCount = 1;
        CHECK_VK_RESULT(vkCreateImageView(p_Device->GetDevice(), &viewCI, p_Allocator, &m_StorageTextures[i].View));

        m_StorageTextures[i].Device = p_Device->GetDevice();
        m_StorageTextures[i].IsInitialized = true;
        m_StorageTextures[i].Allocator = p_Allocator;
        m_StorageTextures[i].Width = static_cast<uint32_t>(imageCI.extent.width);
        m_StorageTextures[i].Height = static_cast<uint32_t>(imageCI.extent.height);
        m_StorageTextures[i].Layout = VK_IMAGE_LAYOUT_GENERAL;
        m_StorageTextures[i].MipMapLevelCount = imageCI.mipLevels;
        m_StorageTextures[i].ArrayLayerCount = imageCI.arrayLayers;
        m_StorageTextures[i].SetDescriptorImage();
    }
}

void VulkanComputeRayTracing::LoadModels()
{
    LoadSkyBoxTextures(&m_SkyBox,
                       1,
                       {HOME_DIR "res/textures/skybox_universe/GalaxyTex_PositiveX.png",
                        HOME_DIR "res/textures/skybox_universe/GalaxyTex_NegativeX.png",
                        HOME_DIR "res/textures/skybox_universe/GalaxyTex_PositiveY.png",
                        HOME_DIR "res/textures/skybox_universe/GalaxyTex_NegativeY.png",
                        HOME_DIR "res/textures/skybox_universe/GalaxyTex_PositiveZ.png",
                        HOME_DIR "res/textures/skybox_universe/GalaxyTex_NegativeZ.png"},
                       true,
                       true);

    // LoadSkyBoxTextures(&m_SkyBox,
    //                    1,
    //                    {HOME_DIR "res/textures/skybox_scene/Right.jpg",
    //                     HOME_DIR "res/textures/skybox_scene/Left.jpg",
    //                     HOME_DIR "res/textures/skybox_scene/Bottom.jpg",
    //                     HOME_DIR "res/textures/skybox_scene/Top.jpg",
    //                     HOME_DIR "res/textures/skybox_scene/Front.jpg",
    //                     HOME_DIR "res/textures/skybox_scene/Back.jpg"},
    //                    true,
    //                    true);

    CreateStorageBuffers();
    CreateStorageImages();
}

void VulkanComputeRayTracing::CreateDescriptorPool()
{
    VulkanRenderSystem::GetGlobalDescriptorPool() = p_RenderSystem->InitSystem(m_Settings.MaxFramesInFlight, p_Device->GetDevice())
                                                        .SetMaxSets(4)
                                                        .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2)             // compute uniform
                                                        .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 + 2) // 2 for compute sampling sky box, 2 for graphics sampling compute result
                                                        .AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 200)           // compute storage buffer
                                                        .AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2)              // compute storage image
                                                        .BuildDescriptorPool(0);
}

void VulkanComputeRayTracing::CreateComputePipeline()
{
    m_ComputeSetLayouts.resize(1);
    m_ComputeSetLayouts[0] = p_RenderSystem->AddSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
                                 .AddSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
                                 .AddSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
                                 .AddSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
                                 .AddSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
                                 .AddSetLayoutBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT)
                                 .BuildDescriptorSetLayout();

    m_ComputeSets.resize(m_Settings.MaxFramesInFlight);
    p_RenderSystem->AllocateDescriptorSets(VulkanRenderSystem::GetGlobalDescriptorPool(), m_ComputeSetLayouts[0], m_ComputeSets.data(), m_ComputeSets.size());
    for (size_t i = 0; i < m_ComputeSets.size(); ++i)
    {
        p_RenderSystem->WriteDescriptorSets(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_ComputeSets[i], 0, &p_Scene->SceneBuffers[0].DescriptorBufferInfo);
        p_RenderSystem->WriteDescriptorSets(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_ComputeSets[i], 1, &m_SkyBox.DescriptorImageInfo);
        p_RenderSystem->WriteDescriptorSets(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_ComputeSets[i], 2, &p_Scene->PointLightsBuffer[0].DescriptorBufferInfo);
        p_RenderSystem->WriteDescriptorSets(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_ComputeSets[i], 3, &p_Scene->DirectLightsBuffer[0].DescriptorBufferInfo);
        p_RenderSystem->WriteDescriptorSets(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_ComputeSets[i], 4, &p_Scene->SpheresBuffer[0].DescriptorBufferInfo);
        p_RenderSystem->WriteDescriptorSets(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_ComputeSets[i], 5, &m_StorageTextures[i].DescriptorImageInfo);
    }
    p_RenderSystem->UpdateDescriptorSets();

    m_ComputePipelineLayout = p_RenderSystem->BuildPipelineLayout(0, nullptr, m_ComputeSetLayouts.data(), m_ComputeSetLayouts.size());
    m_ComputePipeline = p_RenderSystem->BuildShaderStage(SHADER_DIR "Comp.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT)
                            .BuildComputePipeline(m_ComputePipelineLayout);
}

void VulkanComputeRayTracing::CreateGraphicsPipeline()
{
    m_GraphicsSetLayouts.resize(1);
    m_GraphicsSetLayouts[0] = p_RenderSystem->AddSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                                  .BuildDescriptorSetLayout();
    m_GraphicsSets.resize(m_Settings.MaxFramesInFlight);
    p_RenderSystem->AllocateDescriptorSets(VulkanRenderSystem::GetGlobalDescriptorPool(), m_GraphicsSetLayouts[0], m_GraphicsSets.data(), m_GraphicsSets.size());
    for (size_t i = 0; i < m_GraphicsSets.size(); ++i)
    {
        p_RenderSystem->WriteDescriptorSets(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_GraphicsSets[i], 0, &m_StorageTextures[i].DescriptorImageInfo);
    }
    p_RenderSystem->UpdateDescriptorSets();

    m_GraphicsLayout = p_RenderSystem->BuildPipelineLayout(0, nullptr, m_GraphicsSetLayouts.data(), m_GraphicsSetLayouts.size());
    p_GraphicsConfigInfo = new PipelineConfigInfo();
    p_RenderSystem->MakeDefaultGraphicsPipelineConfigInfo(p_GraphicsConfigInfo,
                                                          m_GraphicsLayout,
                                                          p_SwapChain->GetRenderPass(),
                                                          0,
                                                          {},
                                                          {});
    m_GraphicsPipeline = p_RenderSystem->BuildShaderStage(SHADER_DIR "Vert.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
                             .BuildShaderStage(SHADER_DIR "Frag.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
                             .BuildGraphicsPipeline(p_GraphicsConfigInfo);
}

void VulkanComputeRayTracing::CreateComputeCmdBuffers()
{
    m_ComputeCmdBuffers.resize(m_Settings.MaxFramesInFlight);
    if (!m_QueueFamilyIndices.ComputeHasValue)
    {
        FATAL("Compute queue is required but NOT available! Check device creation!");
    }
    m_ComputePool = p_Device->CreateCommandPool(m_QueueFamilyIndices.Compute, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    p_Device->AllocateCommandBuffers(m_ComputePool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(m_ComputeCmdBuffers.size()), m_ComputeCmdBuffers.data());

    m_ComputeInFlightFences.resize(m_Settings.MaxFramesInFlight);
    m_ComputeFinishedSemaphores.resize(m_Settings.MaxFramesInFlight);
    VkFenceCreateInfo fenceCI = vkinfo::FenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaCI = vkinfo::SemaphoreInfo();
    for (uint32_t i = 0; i < m_Settings.MaxFramesInFlight; ++i)
    {
        CHECK_VK_RESULT(vkCreateFence(p_Device->GetDevice(), &fenceCI, p_Allocator, &m_ComputeInFlightFences[i]));
        CHECK_VK_RESULT(vkCreateSemaphore(p_Device->GetDevice(), &semaCI, p_Allocator, &m_ComputeFinishedSemaphores[i]));
    }
}

void VulkanComputeRayTracing::BuildComputeCmdBuffer()
{
    // Compute frame synchronization
    CHECK_VK_RESULT(vkWaitForFences(p_Device->GetDevice(), 1, &m_ComputeInFlightFences[p_SwapChain->m_CurrentFrame], VK_TRUE, DEFAULT_FENCE_TIMEOUT));
    CHECK_VK_RESULT(vkResetFences(p_Device->GetDevice(), 1, &m_ComputeInFlightFences[p_SwapChain->m_CurrentFrame]));
    VkCommandBufferBeginInfo beginInfo = vkinfo::CommandBufferBeginInfo();
    VkCommandBuffer cmdBuffer = m_ComputeCmdBuffers[p_SwapChain->m_CurrentFrame];
    CHECK_VK_RESULT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

    VkImageMemoryBarrier barrier = vkinfo::ImageMemoryBarrier();
    barrier.oldLayout = m_StorageTextures[p_SwapChain->m_CurrentFrame].Layout;
    barrier.newLayout = m_StorageTextures[p_SwapChain->m_CurrentFrame].Layout;
    barrier.image = m_StorageTextures[p_SwapChain->m_CurrentFrame].Image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    if (m_QueueFamilyIndices.Graphics != m_QueueFamilyIndices.Compute)
    {
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.srcQueueFamilyIndex = m_QueueFamilyIndices.Graphics;
        barrier.dstQueueFamilyIndex = m_QueueFamilyIndices.Compute;
        vkCmdPipelineBarrier(cmdBuffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
    }

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipelineLayout, 0, 1, &m_ComputeSets[p_SwapChain->m_CurrentFrame], 0, nullptr);

    vkCmdDispatch(cmdBuffer, m_StorageTextures[p_SwapChain->m_CurrentFrame].Width / 16, m_StorageTextures[p_SwapChain->m_CurrentFrame].Height / 16, 1);

    if (m_QueueFamilyIndices.Graphics != m_QueueFamilyIndices.Compute)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_NONE;
        barrier.srcQueueFamilyIndex = m_QueueFamilyIndices.Compute;
        barrier.dstQueueFamilyIndex = m_QueueFamilyIndices.Graphics;
        vkCmdPipelineBarrier(cmdBuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
    }

    CHECK_VK_RESULT(vkEndCommandBuffer(cmdBuffer));
}

void VulkanComputeRayTracing::Prepare()
{
    p_Scene = new VulkanScene();
    p_Scene->Connect(p_Device);
    VulkanRenderer::Prepare();
    CreateRenderPasses();
    LoadModels();
    CreateDescriptorPool();
    CreateComputePipeline();
    CreateGraphicsPipeline();
    PrepareUI(p_SwapChain->GetRenderPass(), 0);
    CreateComputeCmdBuffers();
}

void VulkanComputeRayTracing::RenderUI()
{
    VulkanRenderer::RenderUI();

    ImGui::SetWindowPos(ImVec2(static_cast<float>(40 * p_UI->m_GlobalScale), static_cast<float>(20 * p_UI->m_GlobalScale)), ImGuiCond_FirstUseEver);
    ImGui::SetWindowSize(ImVec2(200 * p_UI->m_GlobalScale, 100 * p_UI->m_GlobalScale), ImGuiCond_FirstUseEver);
    ImGui::Begin("Test");
    ImGui::TextUnformatted("Hello World!");
    ImGui::Text("Camera Position: %.3f, %.3f, %.3f", p_Camera->GetPosition().x, p_Camera->GetPosition().y, p_Camera->GetPosition().z);
    ImGui::Text("Camera Front: %.3f, %.3f, %.3f", p_Camera->GetDirection().x, p_Camera->GetDirection().y, p_Camera->GetDirection().z);
    const uint32_t rflmin = 1;
    const uint32_t rflmax = 10;
    ImGui::SliderScalar("Ray Reflection", ImGuiDataType_U32, &p_Scene->SceneProperty.ReflectDepth, &rflmin, &rflmax);
    const uint32_t rfrmin = 1;
    const uint32_t rfrmax = 10;
    ImGui::SliderScalar("Ray Refraction", ImGuiDataType_U32, &p_Scene->SceneProperty.RefractDepth, &rfrmin, &rfrmax);
    ImGui::End();
}

void VulkanComputeRayTracing::CommitAllSubmits()
{
    // Compute device synchronization
    // VkPipelineStageFlags computeWaitstages[] = {};
    VkSemaphore computeSignalSemaphores[] = {m_ComputeFinishedSemaphores[p_SwapChain->m_CurrentFrame]};
    VkSubmitInfo computeSubmit = vkinfo::SubmitInfo();
    computeSubmit.commandBufferCount = 1;
    computeSubmit.pCommandBuffers = &m_ComputeCmdBuffers[p_SwapChain->m_CurrentFrame];
    computeSubmit.signalSemaphoreCount = static_cast<uint32_t>(STATIC_ARRAY_SIZE(computeSignalSemaphores));
    computeSubmit.pSignalSemaphores = computeSignalSemaphores;
    CHECK_VK_RESULT(vkQueueSubmit(m_Queues.Compute, 1, &computeSubmit, m_ComputeInFlightFences[p_SwapChain->m_CurrentFrame]));

    // Graphics device synchronization
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT};
    VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[p_SwapChain->m_CurrentFrame], m_ComputeFinishedSemaphores[p_SwapChain->m_CurrentFrame]};
    VkSubmitInfo submit = vkinfo::SubmitInfo();
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &m_DrawCmdBuffers[p_SwapChain->m_CurrentFrame];
    submit.waitSemaphoreCount = static_cast<uint32_t>(STATIC_ARRAY_SIZE(waitSemaphores));
    submit.pWaitSemaphores = waitSemaphores;
    submit.pWaitDstStageMask = waitStages;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &m_RenderFinishedSemaphores[p_SwapChain->m_CurrentFrame];

    CHECK_VK_RESULT(vkQueueSubmit(m_Queues.Graphics, 1, &submit, m_GraphicsInFlightFences[p_SwapChain->m_CurrentFrame]));
}

void VulkanComputeRayTracing::Render()
{
    p_Camera->UpdateViewMat();
    p_Camera->UpdatePerspectiveMat(opm::MATH_PI_4, static_cast<float>(m_Width) / static_cast<float>(m_Height), 0.1, 256.0);

    p_Scene->SceneProperty.CanvasWidth = m_Width;
    p_Scene->SceneProperty.CanvasHeight = m_Height;
    p_Scene->SceneProperty.AmbientColor = {0.05, 0.05, 0.05};
    p_Scene->SceneProperty.Camera.Position = p_Camera->GetPosition();
    p_Scene->SceneProperty.Camera.fov = p_Camera->GetFov();
    p_Scene->SceneProperty.Camera.Front = p_Camera->GetFront();
    p_Scene->SceneProperty.Camera.Up = p_Camera->GetUp();
    p_Scene->SceneProperty.ViewMat = p_Camera->GetViewMat(false);
    p_Scene->SceneProperty.InverseViewMat = p_Camera->GetInverseViewMat(false);
    p_Scene->SceneProperty.ProjectionMat = p_Camera->GetProjectionMat(false);
    p_Scene->SceneProperty.InverseProjectionMat = p_Camera->GetInverseProjectionMat(false);
    p_Scene->SceneProperty.Definitions = GetSceneDefinition(*p_Scene);

    UpdateUniformBuffers(&p_Scene->SceneBuffers[0], 1, &p_Scene->SceneProperty);

    BuildComputeCmdBuffer();

    /*============================== Begin frame ==============================*/
    VkCommandBuffer cmdBuffer = BeginFrame();
    if (cmdBuffer != VK_NULL_HANDLE)
    {
        /*============================== Update uniforms ==============================*/

        /*============================== Begin render pass ==============================*/
        BeginRenderPass(cmdBuffer, p_SwapChain->GetRenderPass());

        /*============================== Draw objects ==============================*/
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
        vkCmdBindDescriptorSets(cmdBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_GraphicsLayout,
                                0,
                                1,
                                &m_GraphicsSets[p_SwapChain->m_CurrentFrame],
                                0,
                                nullptr);
        vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

        /*============================== End render pass ==============================*/
        EndRenderPass(cmdBuffer);

        /*============================== End frame ==============================*/
        EndFrame();
    }
}

VULKAN_EXAMPLE_MAIN(VulkanComputeRayTracing)
