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
#include "VulkanRenderer.h"

class VulkanExperiment : public VulkanRenderer
{
public:
    VulkanExperiment()
        : VulkanRenderer(CAMERA_TYPE_LOOK_AT, ALLOCATE_CALLBACK)
    {
        m_Settings.FullScreenMode = true;
    }

    ~VulkanExperiment()
    {
        if (m_SkyBoxPipeline != VK_NULL_HANDLE)
        {
            p_RenderSystem->DestroyPipeline(m_SkyBoxPipeline);
        }
        if (m_SkyBoxPipelineLayout != VK_NULL_HANDLE)
        {
            p_RenderSystem->DestroyPipelineLayout(m_SkyBoxPipelineLayout);
        }
        if (p_SkyBoxPipelineConfig != nullptr)
        {
            delete p_SkyBoxPipelineConfig;
        }
        if (m_ModelGraphicsPipeline != VK_NULL_HANDLE)
        {
            p_RenderSystem->DestroyPipeline(m_ModelGraphicsPipeline);
        }
        if (m_ModelGraphicsPipelineLayout != VK_NULL_HANDLE)
        {
            p_RenderSystem->DestroyPipelineLayout(m_ModelGraphicsPipelineLayout);
        }
        if (p_ModelGraphcisPipelineConfig != nullptr)
        {
            delete p_ModelGraphcisPipelineConfig;
        }
        for (size_t i = 0; i < p_Models.size(); ++i)
        {
            delete p_Models[i];
        }
        p_RenderSystem->DestroyDescriptorPool(VulkanRenderSystem::GetGlobalDescriptorPool());
        for (size_t i = 0; i < m_DescriptorSetLayouts.size(); ++i)
        {
            p_RenderSystem->DestroyDescriptorSetLayout(m_DescriptorSetLayouts[i]);
        }
        for (size_t i = 0; i < m_SkyBoxDescriptorSetLayout.size(); ++i)
        {
            p_RenderSystem->DestroyDescriptorSetLayout(m_SkyBoxDescriptorSetLayout[i]);
        }
    }

public:
    virtual void Prepare() override;
    virtual void RenderUI() override;
    virtual void Render() override;

private:
    void CreateRenderPasses();
    void LoadModels();
    void CreateDescriptorPool();
    /**
     * @brief Question: Enable depth test or not?
     * @if Don't enable. We draw sky box at the first time, then draw other objects.
     * @if Enable. We set the depth compare operation as VK_COMPARE_OP_LESS_OR_EQUAL so that sky box will pass the depth test because our sky box depth is always 1.0.
     */
    void CreateGraphicsPipelines();

private:
    std::vector<VulkanModel *> p_Models = {};

    // Model and camera
    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts = {};
    PipelineConfigInfo *p_ModelGraphcisPipelineConfig = nullptr;
    VkPipelineLayout m_ModelGraphicsPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_ModelGraphicsPipeline = VK_NULL_HANDLE;

    // Sky box
    std::vector<VkDescriptorSetLayout> m_SkyBoxDescriptorSetLayout = {};
    PipelineConfigInfo *p_SkyBoxPipelineConfig = nullptr;
    VkPipelineLayout m_SkyBoxPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_SkyBoxPipeline = VK_NULL_HANDLE;
};

void VulkanExperiment::CreateRenderPasses()
{
    p_SwapChain->InitRenderPass();
    // p_SwapChain->m_RenderPassResource.Attachments.push_back(depthStencilAttachment);
    // p_SwapChain->m_RenderPassResource.References.push_back(...);
    // p_SwapChain->m_RenderPassResource.Subpasses.push_back(...);
    // p_SwapChain->m_RenderPassResource.Dependencies.push_back(...);
    p_SwapChain->SetUpRenderPass();
    // std::vector<ImageBuffer> imageBuffers;
    // p_SwapChain->CreateImageBuffer(imageBuffers,...,false);
    // for (size_t i = 0; i < p_SwapChain->m_FrameBufferAttachments.size(); ++i)
    // {
    //     p_SwapChain->m_FrameBufferAttachments[i].push_back(imageView);
    // }
    p_SwapChain->CreateFrameBuffers();
}

void VulkanExperiment::LoadModels()
{
    // Set camera properties
    p_Camera->SetPosition({0.0, 0.0, -2.0});

    // Sky box
    CreateSkyBox(HOME_DIR "res/models/Cube.obj",
                 {HOME_DIR "res/textures/skybox_scene/Right.jpg",
                  HOME_DIR "res/textures/skybox_scene/Left.jpg",
                  HOME_DIR "res/textures/skybox_scene/Bottom.jpg",
                  HOME_DIR "res/textures/skybox_scene/Top.jpg",
                  HOME_DIR "res/textures/skybox_scene/Front.jpg",
                  HOME_DIR "res/textures/skybox_scene/Back.jpg"},
                 false,
                 true);
    CreateUniformBuffers(sizeof(VulkanCamera::Matrix), p_SkyBox->m_TransformBuffers.data(), p_SkyBox->m_TransformBuffers.size());

    // Global uniform buffer
    CreateUniformBuffers(sizeof(VulkanCamera::Matrix), p_Camera->m_CameraUniformBuffers.data(), p_Camera->m_CameraUniformBuffers.size());

    // Models
    p_Models.push_back(std::move(LoadModel(HOME_DIR "res/models/Viking_Room.obj", MODEL_TYPE_OBJ, 0, VK_VERTEX_INPUT_RATE_VERTEX)));
    p_Models[0]->Transform({1.0, 1.0, 1.0}, {-opm::MATH_PI_2, opm::MATH_PI_2, 0.0}, {0.0, -1.0, 3.0});
    std::vector<VulkanVertex> vertex = {{{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {}, {0.0f, 0.0f}},
                                        {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {}, {1.0f, 0.0f}},
                                        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {}, {1.0f, 1.0f}},
                                        {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {}, {0.0f, 1.0}}};
    std::vector<IndexType> index = {0, 1, 2, 2, 3, 0};
    p_Models.push_back(std::move(LoadModel(vertex, 0, VK_VERTEX_INPUT_RATE_VERTEX, index)));
    p_Models[1]->Transform({1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, -1.0});

    // p_Models.push_back(std::move(LoadModel(HOME_DIR "res/models/obj.obj", MODEL_TYPE_OBJ, 0, VK_VERTEX_INPUT_RATE_VERTEX)));
    // p_Models.push_back(std::move(LoadModel(HOME_DIR "res/models/spacecraft.obj", MODEL_TYPE_OBJ, 0, VK_VERTEX_INPUT_RATE_VERTEX)));

    for (size_t i = 0; i < p_Models.size(); ++i)
    {
        CreateUniformBuffers(sizeof(opm::mat4), p_Models[i]->m_TransformBuffers.data(), p_Models[i]->m_TransformBuffers.size());
    }
    CreateTextures(HOME_DIR "res/textures/Viking_Room.png", p_Models[0]->m_ColorTextures.data(), p_Models[0]->m_ColorTextures.size(), true, true);
    CreateTextures(HOME_DIR "res/textures/Quad.jpg", p_Models[1]->m_ColorTextures.data(), p_Models[1]->m_ColorTextures.size(), true, true);
    // opm::srgb color(100, 60, 60, 100);
    // CreateTextures(p_Models[1]->m_ColorTextures.data(), p_Models[1]->m_ColorTextures.size(), &color);
}

void VulkanExperiment::CreateDescriptorPool()
{
    VulkanRenderSystem::GetGlobalDescriptorPool() = p_RenderSystem->InitSystem(m_Settings.MaxFramesInFlight, p_Device->GetDevice())
                                                        .SetMaxSets((p_Models.size() * 2 + 1 + 2) * m_Settings.MaxFramesInFlight)
                                                        .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_Settings.MaxFramesInFlight)                           // camera uniform
                                                        .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, p_Models.size() * m_Settings.MaxFramesInFlight)         // module uniform
                                                        .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, p_Models.size() * m_Settings.MaxFramesInFlight) // module texture
                                                        .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_Settings.MaxFramesInFlight)                           // sky box uniform
                                                        .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_Settings.MaxFramesInFlight)                   // sky cube texture
                                                        .BuildDescriptorPool(0);
}

/**
 * @brief Question: Enable depth test or not?
 * @if Don't enable. We draw sky box at the first time, then draw other objects.
 * @if Enable. We set the depth compare operation as VK_COMPARE_OP_LESS_OR_EQUAL so that sky box will pass the depth test because our sky box depth is always 1.0.
 */
void VulkanExperiment::CreateGraphicsPipelines()
{
    // Sky box descriptor sets
    VkDescriptorSetLayout transform = p_RenderSystem->AddSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT)
                                          .BuildDescriptorSetLayout();
    m_SkyBoxDescriptorSetLayout.push_back(transform);
    p_SkyBox->m_DescriptorSetLayouts.push_back(transform);
    p_RenderSystem->AllocateDescriptorSets(VulkanRenderSystem::GetGlobalDescriptorPool(), transform, p_SkyBox->m_TransformSets.data(), p_SkyBox->m_TransformSets.size());
    for (size_t j = 0; j < p_SkyBox->m_TransformSets.size(); ++j)
    {
        p_RenderSystem->WriteDescriptorSets(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, p_SkyBox->m_TransformSets[j], 0, &p_SkyBox->m_TransformBuffers[j].DescriptorBufferInfo);
    }

    VkDescriptorSetLayout texture = p_RenderSystem->AddSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                                        .BuildDescriptorSetLayout();
    m_SkyBoxDescriptorSetLayout.push_back(texture);
    p_SkyBox->m_DescriptorSetLayouts.push_back(texture);
    p_RenderSystem->AllocateDescriptorSets(VulkanRenderSystem::GetGlobalDescriptorPool(), texture, p_SkyBox->m_TextureSets.data(), p_SkyBox->m_TextureSets.size());
    for (size_t j = 0; j < p_SkyBox->m_TextureSets.size(); ++j)
    {
        p_RenderSystem->WriteDescriptorSets(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, p_SkyBox->m_TextureSets[j], 0, &p_SkyBox->m_ColorTextures[j].DescriptorImageInfo);
    }
    p_RenderSystem->UpdateDescriptorSets();

    m_SkyBoxPipelineLayout = p_RenderSystem->BuildPipelineLayout(0, nullptr, p_SkyBox->m_DescriptorSetLayouts.data(), p_SkyBox->m_DescriptorSetLayouts.size());
    p_SkyBoxPipelineConfig = new PipelineConfigInfo();
    p_RenderSystem->MakeDefaultGraphicsPipelineConfigInfo(p_SkyBoxPipelineConfig,
                                                          m_SkyBoxPipelineLayout,
                                                          p_SwapChain->GetRenderPass(),
                                                          0,
                                                          VulkanModel::GetBindingDescription(),
                                                          VulkanModel::GetAttributeDescription());
    p_SkyBoxPipelineConfig->RasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    // If enable depth test, depth compare operation must be VK_COMPARE_OP_LESS_OR_EQUAL so that sky box will pass the depth test because our sky box depth is always 1.0.
    p_SkyBoxPipelineConfig->DepthStencilInfo.depthTestEnable = VK_TRUE;
    p_SkyBoxPipelineConfig->DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    p_SkyBoxPipelineConfig->DepthStencilInfo.depthWriteEnable = VK_FALSE;

    m_SkyBoxPipeline = p_RenderSystem->BuildShaderStage(SHADER_DIR "Sky_Box.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
                           .BuildShaderStage(SHADER_DIR "Sky_Box.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
                           .BuildGraphicsPipeline(p_SkyBoxPipelineConfig);

    // Camera descriptors
    p_Camera->m_CameraSetLayout = p_RenderSystem->AddSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT)
                                      .BuildDescriptorSetLayout();
    m_DescriptorSetLayouts.push_back(p_Camera->m_CameraSetLayout);
    p_RenderSystem->AllocateDescriptorSets(VulkanRenderSystem::GetGlobalDescriptorPool(), p_Camera->m_CameraSetLayout, p_Camera->m_CameraSets.data(), p_Camera->m_CameraSets.size());
    for (uint32_t i = 0; i < p_Camera->m_CameraSets.size(); ++i)
    {
        p_RenderSystem->WriteDescriptorSets(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, p_Camera->m_CameraSets[i], 0, &p_Camera->m_CameraUniformBuffers[i].DescriptorBufferInfo);
    }
    // Model transform descriptors
    VkDescriptorSetLayout modelTransformSetLayout = p_RenderSystem->AddSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT)
                                                        .BuildDescriptorSetLayout();
    m_DescriptorSetLayouts.push_back(modelTransformSetLayout);
    for (size_t i = 0; i < p_Models.size(); ++i)
    {
        p_Models[i]->m_DescriptorSetLayouts.push_back(modelTransformSetLayout);
        p_RenderSystem->AllocateDescriptorSets(VulkanRenderSystem::GetGlobalDescriptorPool(), modelTransformSetLayout, p_Models[i]->m_TransformSets.data(), p_Models[i]->m_TransformSets.size());
        for (size_t j = 0; j < p_Models[i]->m_TransformSets.size(); ++j)
        {
            p_RenderSystem->WriteDescriptorSets(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, p_Models[i]->m_TransformSets[j], 0, &p_Models[i]->m_TransformBuffers[j].DescriptorBufferInfo);
        }
    }
    // Model texture descriptors
    VkDescriptorSetLayout modelTextureSetLayout = p_RenderSystem->AddSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                                                      .BuildDescriptorSetLayout();
    m_DescriptorSetLayouts.push_back(modelTextureSetLayout);
    for (size_t i = 0; i < p_Models.size(); ++i)
    {
        p_Models[i]->m_DescriptorSetLayouts.push_back(modelTextureSetLayout);
        p_RenderSystem->AllocateDescriptorSets(VulkanRenderSystem::GetGlobalDescriptorPool(), modelTextureSetLayout, p_Models[i]->m_TextureSets.data(), p_Models[i]->m_TextureSets.size());
        for (size_t j = 0; j < p_Models[i]->m_TextureSets.size(); ++j)
        {
            p_RenderSystem->WriteDescriptorSets(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, p_Models[i]->m_TextureSets[j], 0, &p_Models[i]->m_ColorTextures[j].DescriptorImageInfo);
        }
    }
    p_RenderSystem->UpdateDescriptorSets();

    // Model and camera graphics pipeline
    m_ModelGraphicsPipelineLayout = p_RenderSystem->BuildPipelineLayout(0, nullptr, m_DescriptorSetLayouts.data(), m_DescriptorSetLayouts.size());
    p_ModelGraphcisPipelineConfig = new PipelineConfigInfo();
    p_RenderSystem->MakeDefaultGraphicsPipelineConfigInfo(p_ModelGraphcisPipelineConfig,
                                                          m_ModelGraphicsPipelineLayout,
                                                          p_SwapChain->GetRenderPass(),
                                                          0,
                                                          VulkanModel::GetBindingDescription(),
                                                          VulkanModel::GetAttributeDescription());
    m_ModelGraphicsPipeline = p_RenderSystem->BuildShaderStage(SHADER_DIR "Basic_Vert.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
                                  .BuildShaderStage(SHADER_DIR "Basic_Frag.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
                                  .BuildGraphicsPipeline(p_ModelGraphcisPipelineConfig);
}

void VulkanExperiment::Prepare()
{
    VulkanRenderer::Prepare();
    CreateRenderPasses();
    LoadModels();
    CreateDescriptorPool();
    CreateGraphicsPipelines();
    PrepareUI(p_SwapChain->GetRenderPass(), 0);
}

void VulkanExperiment::RenderUI()
{
    VulkanRenderer::RenderUI();

    ImGui::SetWindowPos(ImVec2(static_cast<float>(40 * p_UI->m_GlobalScale), static_cast<float>(20 * p_UI->m_GlobalScale)), ImGuiCond_FirstUseEver);
    ImGui::SetWindowSize(ImVec2(200 * p_UI->m_GlobalScale, 100 * p_UI->m_GlobalScale), ImGuiCond_FirstUseEver);
    ImGui::Begin("Test");
    ImGui::TextUnformatted("Hello World!");
    ImGui::End();
}

void VulkanExperiment::Render()
{
    /*============================== Begin frame ==============================*/
    VkCommandBuffer cmdBuffer = BeginFrame();
    if (cmdBuffer != VK_NULL_HANDLE)
    {
        /*============================== Update uniforms ==============================*/
        // Update current global uniform buffer
        p_Camera->UpdateViewMat();
        p_Camera->UpdatePerspectiveMat(opm::MATH_PI_4, static_cast<float>(m_Width) / static_cast<float>(m_Height), 0.1, 100.0);
        UpdateUniformBuffers(&p_Camera->m_CameraUniformBuffers[p_SwapChain->m_CurrentFrame], 1, &p_Camera->GetUniformData());

        // Update model transform uniform buffer
        // p_Models[0]->Transform({1.0, 1.0, 1.0}, {0.0, 0.0, 0.01}, {0.0, 0.0, 0.0});
        p_Models[1]->Transform({1.0, 1.0, 1.0}, {0.0, 0.0, -0.01}, {0.0, 0.0, 0.0});
        for (size_t i = 0; i < p_Models.size(); ++i)
        {
            opm::mat4 modelMat = p_Models[i]->m_UniqueModelMat.Transpose();
            UpdateUniformBuffers(&p_Models[i]->m_TransformBuffers[p_SwapChain->m_CurrentFrame], 1, &modelMat);
        }

        // Update sky box transform uniform buffer
        VulkanCamera::Matrix m = p_Camera->GetUniformData();
        m.ViewMat[3] = {0.0, 0.0, 0.0, 1.0};
        UpdateUniformBuffers(&p_SkyBox->m_TransformBuffers[p_SwapChain->m_CurrentFrame], 1, &m);

        /*============================== Begin render pass ==============================*/
        BeginRenderPass(cmdBuffer, p_SwapChain->GetRenderPass());

        /*============================== Draw objects ==============================*/

        // Sky box
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyBoxPipeline);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyBoxPipelineLayout, 0, 1, &p_SkyBox->m_TransformSets[p_SwapChain->m_CurrentFrame], 0, nullptr);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyBoxPipelineLayout, 1, 1, &p_SkyBox->m_TextureSets[p_SwapChain->m_CurrentFrame], 0, nullptr);
        p_SkyBox->Bind(cmdBuffer);
        p_SkyBox->Draw(cmdBuffer);

        // Objects
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ModelGraphicsPipeline);
        // Camera descriptor
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ModelGraphicsPipelineLayout, 0, 1, &p_Camera->m_CameraSets[p_SwapChain->m_CurrentFrame], 0, nullptr);
        for (size_t i = 0; i < p_Models.size(); ++i)
        {
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ModelGraphicsPipelineLayout, 1, 1, &p_Models[i]->m_TransformSets[p_SwapChain->m_CurrentFrame], 0, nullptr);
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ModelGraphicsPipelineLayout, 2, 1, &p_Models[i]->m_TextureSets[p_SwapChain->m_CurrentFrame], 0, nullptr);
            p_Models[i]->Bind(cmdBuffer);
            p_Models[i]->Draw(cmdBuffer);
        }

        /*============================== End render pass ==============================*/
        EndRenderPass(cmdBuffer);

        /*============================== End frame ==============================*/
        EndFrame();
    }
}

VULKAN_EXAMPLE_MAIN(VulkanExperiment)
