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

#include "VulkanRenderer.h"
#include "VulkanInitializer.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>

VulkanRenderer::VulkanRenderer(CameraTypeFlags camType, const VkAllocationCallbacks *pAllocator)
{
    p_Allocator = pAllocator;

#if defined(AUTO_VALIDATION)
#if defined(RELEASE_MODE)
    m_Settings.EnableValidationLayer = false;
#elif defined(DEBUG_MODE)
    m_Settings.EnableValidationLayer = true;
#endif
#endif

    try
    {
        p_Camera = new VulkanCamera();
    }
    catch (const std::exception &e)
    {
        if (p_Camera != nullptr)
        {
            delete p_Instance;
            p_Instance = nullptr;
        }
        FATAL(e.what());
    }
    p_Camera->m_Type = camType;

    INFO_TIME("Enter program!\n");
}

VulkanRenderer::~VulkanRenderer()
{
    for (size_t i = 0; i < m_Settings.MaxFramesInFlight; ++i)
    {
        vkDestroyFence(p_Device->GetDevice(), m_GraphicsInFlightFences[i], p_Allocator);
        vkDestroySemaphore(p_Device->GetDevice(), m_ImageAvailableSemaphores[i], p_Allocator);
        vkDestroySemaphore(p_Device->GetDevice(), m_RenderFinishedSemaphores[i], p_Allocator);
    }
    if (p_UI != nullptr)
    {
        delete p_UI;
    }
    if (p_SkyBox != nullptr)
    {
        delete p_SkyBox;
    }
    if (p_Camera != nullptr)
    {
        delete p_Camera;
    }
    if (m_DrawCmdPool != VK_NULL_HANDLE)
    {
        p_Device->DestroyCommandPool(m_DrawCmdPool);
    }
    if (p_RenderSystem != nullptr)
    {
        delete p_RenderSystem;
    }
    if (p_SwapChain != nullptr)
    {
        delete p_SwapChain;
    }
    if (p_Device != nullptr)
    {
        delete p_Device;
    }
    if (p_Instance != nullptr)
    {
        delete p_Instance;
    }
    if (p_Window != nullptr)
    {
        glfwDestroyWindow(p_Window);
        glfwTerminate();
    }

    INFO_TIME("Exit program!\n\n");
}

void VulkanRenderer::FrameBufferResizeCallback(GLFWwindow *window, int width, int height)
{
    VulkanRenderer *MyWindow = reinterpret_cast<VulkanRenderer *>(glfwGetWindowUserPointer(window));
    MyWindow->OnWindowResize(width, height);
}

void VulkanRenderer::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    VulkanRenderer *MyWindow = reinterpret_cast<VulkanRenderer *>(glfwGetWindowUserPointer(window));
    MyWindow->OnKeyState(key, scancode, action, mods);
}

void VulkanRenderer::MouseMoveCallback(GLFWwindow *window, double xpos, double ypos)
{
    VulkanRenderer *MyWindow = reinterpret_cast<VulkanRenderer *>(glfwGetWindowUserPointer(window));
    MyWindow->OnMouseMove(xpos, ypos);
}

void VulkanRenderer::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    VulkanRenderer *MyWindow = reinterpret_cast<VulkanRenderer *>(glfwGetWindowUserPointer(window));
    MyWindow->OnMouseButtonState(button, action, mods);
}

void VulkanRenderer::MouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    VulkanRenderer *MyWindow = reinterpret_cast<VulkanRenderer *>(glfwGetWindowUserPointer(window));
    MyWindow->OnMouseScroll(xoffset, yoffset);
}

void VulkanRenderer::IconifyWindowCallback(GLFWwindow *window, int inconify)
{
    if (static_cast<bool>(inconify))
    {
        INFO("Window %p is inconified!\n", window);
    }
    else
    {
        glfwRestoreWindow(window);
        INFO("Window %p is restored!\n", window);
    }
}

void VulkanRenderer::SetUpWindow(GLFWframebuffersizefun frameCallback,
                                 GLFWkeyfun keyCallback,
                                 GLFWcursorposfun mouseMoveCallback,
                                 GLFWmousebuttonfun mouseButtonCallback,
                                 GLFWscrollfun mouseScrollCallback)
{
    if (!glfwInit())
    {
        FATAL("GLFW initialization failed!");
    }

    uint32_t width = 0, height = 0;
    p_Monitor = glfwGetPrimaryMonitor();
    if (p_Monitor == nullptr)
    {
        FATAL("Failed to find monitor!");
    }
    const GLFWvidmode *viMode = glfwGetVideoMode(p_Monitor);

    glfwWindowHint(GLFW_RED_BITS, viMode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, viMode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, viMode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, viMode->refreshRate);

    if (viMode == nullptr)
    {
        FATAL("Failed to query video mode!");
    }
    width = static_cast<uint32_t>(viMode->width);
    height = static_cast<uint32_t>(viMode->height);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (m_Settings.FullScreenMode)
    {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_TRUE);

        m_Width = width;
        m_Height = height;

        p_Window = glfwCreateWindow(static_cast<int>(m_Width), static_cast<int>(m_Height), WND_TITLE, p_Monitor, nullptr);
        if (p_Window == nullptr)
        {
            FATAL("Failed to create window!");
        }
    }
    else
    {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        p_Window = glfwCreateWindow(static_cast<int>(m_Width), static_cast<int>(m_Height), WND_TITLE, nullptr, nullptr);
        if (p_Window == nullptr)
        {
            FATAL("Failed to create window!");
        }
        glfwSetWindowPos(p_Window, (width - m_Width) / 2, (height - m_Height) / 2);
    }
    glfwSetCursorPos(p_Window, static_cast<double>(m_Width) / 2.0, static_cast<double>(m_Height) / 2.0);

    glfwSetWindowUserPointer(p_Window, this);
    glfwSetFramebufferSizeCallback(p_Window, frameCallback);
    glfwSetKeyCallback(p_Window, keyCallback);
    glfwSetCursorPosCallback(p_Window, mouseMoveCallback);
    glfwSetMouseButtonCallback(p_Window, mouseButtonCallback);
    glfwSetScrollCallback(p_Window, mouseScrollCallback);
    glfwSetWindowIconifyCallback(p_Window, IconifyWindowCallback);
}

void VulkanRenderer::InitVulkan(uint32_t maxFramesInFilght)
{
    // Instance
    try
    {
        p_Instance = new VulkanInstance(m_Settings.EnableValidationLayer, p_Allocator);
        // p_Instance->m_EnabledEextensions.push_back("...");
        p_Instance->CreateInstance();
        p_Instance->SetUpDebugMessenger();
    }
    catch (const std::exception &e)
    {
        if (p_Instance != nullptr)
        {
            delete p_Instance;
            p_Instance = nullptr;
        }
        FATAL(e.what());
    }

    // Device
    try
    {
        p_Device = new VulkanDevice(m_Settings.EnableValidationLayer, p_Allocator);
    }
    catch (const std::exception &e)
    {
        if (p_Device != nullptr)
        {
            delete p_Device;
            p_Device = nullptr;
        }
        FATAL(e.what());
    }

    // SwapChain
    try
    {
        p_SwapChain = new VulkanSwapChain(p_Instance->GetInstance(), p_Allocator);
        p_SwapChain->CreateSurface(p_Window);
    }
    catch (const std::exception &e)
    {
        if (p_SwapChain != nullptr)
        {
            delete p_SwapChain;
            p_SwapChain = nullptr;
        }
        FATAL(e.what());
    }

    // Render system
    try
    {
        p_RenderSystem = new VulkanRenderSystem(p_Allocator);
    }
    catch (const std::exception &e)
    {
        if (p_RenderSystem != nullptr)
        {
            delete p_RenderSystem;
            p_RenderSystem = nullptr;
        }
        FATAL(e.what());
    }

    // UI
    try
    {
        if (m_Settings.EnableUI)
        {
            p_UI = new VulkanUI(m_Width,
                                m_Height,
                                maxFramesInFilght,
                                p_Device,
                                p_Allocator);
        }
    }
    catch (const std::exception &e)
    {
        if (p_UI != nullptr)
        {
            delete p_UI;
            p_UI = nullptr;
        }
        FATAL(e.what());
    }

    uint32_t *p_MaxFrames = const_cast<uint32_t *>(&m_Settings.MaxFramesInFlight);
    *p_MaxFrames = maxFramesInFilght;
    p_Camera->m_CameraUniformBuffers.resize(maxFramesInFilght);
    p_Camera->m_CameraSets.resize(maxFramesInFilght);
    if (p_Camera->m_Type == CAMERA_TYPE_FIRST_PERSON)
    {
        glfwSetInputMode(p_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    m_IsInitialized = true;
}

void VulkanRenderer::OnWindowResize(int width, int height)
{
    m_FrameBufferResizing = true;
    m_Width = width;
    m_Height = height;
    if (m_Settings.EnableUI)
    {
        p_UI->Resize(width, height);
    }
}

void VulkanRenderer::OnKeyState(int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(p_Window, true);
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        m_Paused = !m_Paused;
        INFO_TIME("Program paused!\n");
    }
    if (m_Settings.EnableUI)
    {
        ImGuiIO &io = ImGui::GetIO();
        if (io.WantCaptureKeyboard)
        {
            // TODO: Fix typing while camera dispatch events
            io.AddInputCharacter(static_cast<unsigned int>(key));
            return;
        }
    }
    p_Camera->OnKeyState(m_DeltaTime / 1000.0f, key, scancode, action, mods);
}

void VulkanRenderer::OnMouseMove(double xpos, double ypos)
{
    if (m_Settings.EnableUI)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.MousePos = ImVec2(static_cast<float>(xpos), static_cast<float>(ypos));
    }
    p_Camera->OnMouseMove(m_DeltaTime / 1000.0f, xpos, ypos);
}

void VulkanRenderer::OnMouseButtonState(int button, int action, int mods)
{
    if (m_Settings.EnableUI)
    {
        ImGuiIO &io = ImGui::GetIO();
        // Release
        if (action == GLFW_RELEASE)
        {
            // glfwSetInputMode(p_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            switch (button)
            {
            case GLFW_MOUSE_BUTTON_LEFT:
            {
                io.MouseDown[0] = false;
            }
            break;

            case GLFW_MOUSE_BUTTON_RIGHT:
            {
                io.MouseDown[1] = false;
            }
            break;

            case GLFW_MOUSE_BUTTON_MIDDLE:
            {

                io.MouseDown[2] = false;
            }
            break;

            default:
                break;
            }
        }
        // Press
        else if (action == GLFW_PRESS)
        {
            // glfwSetInputMode(p_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            switch (button)
            {
            case GLFW_MOUSE_BUTTON_LEFT:
            {
                io.MouseDown[0] = true;
            }
            break;

            case GLFW_MOUSE_BUTTON_RIGHT:
            {
                io.MouseDown[1] = true;
            }
            break;

            case GLFW_MOUSE_BUTTON_MIDDLE:
            {

                io.MouseDown[2] = true;
            }
            break;

            default:
                break;
            }
        }
    }
    p_Camera->OnMouseButtonState(m_DeltaTime / 1000.0f, button, action, mods);
}

void VulkanRenderer::OnMouseScroll(double xoffset, double yoffset)
{
    p_Camera->OnMouseScroll(m_DeltaTime / 1000.0f, xoffset, yoffset);
}

void VulkanRenderer::CreateSyncObjects()
{
    m_GraphicsInFlightFences.resize(m_Settings.MaxFramesInFlight);
    m_ImageAvailableSemaphores.resize(m_Settings.MaxFramesInFlight);
    m_RenderFinishedSemaphores.resize(m_Settings.MaxFramesInFlight);

    VkFenceCreateInfo fenceCI = vkinfo::FenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCI = vkinfo::SemaphoreInfo();

    for (uint32_t i = 0; i < m_Settings.MaxFramesInFlight; ++i)
    {
        CHECK_VK_RESULT(vkCreateFence(p_Device->GetDevice(), &fenceCI, p_Allocator, &m_GraphicsInFlightFences[i]));
        CHECK_VK_RESULT(vkCreateSemaphore(p_Device->GetDevice(), &semaphoreCI, p_Allocator, &m_ImageAvailableSemaphores[i]));
        CHECK_VK_RESULT(vkCreateSemaphore(p_Device->GetDevice(), &semaphoreCI, p_Allocator, &m_RenderFinishedSemaphores[i]));
    }
}

void VulkanRenderer::Prepare()
{
    if (!m_IsInitialized)
    {
        FATAL("VulkanRenderer must be intialized!");
    }

    p_Device->InitDevice(p_Instance->GetInstance(), p_SwapChain->GetSurface(), QUEUE_TYPE_ALL, &m_QueueFamilyIndices, m_UniqueQueueFamilyIndices, &m_Queues);
    m_DrawCmdPool = p_Device->CreateCommandPool(m_QueueFamilyIndices.Graphics);
    m_DrawCmdBuffers.resize(m_Settings.MaxFramesInFlight);
    p_Device->AllocateCommandBuffers(m_DrawCmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(m_DrawCmdBuffers.size()), m_DrawCmdBuffers.data());
    p_SwapChain->Connect(p_Device->GetGPU(), p_Device->GetDevice(), &m_QueueFamilyIndices, m_UniqueQueueFamilyIndices.data(), static_cast<uint32_t>(m_UniqueQueueFamilyIndices.size()), &m_Queues, m_Settings.MaxFramesInFlight);
    p_SwapChain->InitSwapChain(m_Settings.EnableVsync);
    p_SwapChain->CreateSwapChain(m_Width, m_Height);
    CreateSyncObjects();
    p_SwapChain->CreateDepthStencilImageBuffer();

    m_Prepared = true;
}

void VulkanRenderer::HandleKeyState()
{
    if (m_Settings.EnableUI)
    {
        ImGuiIO &io = ImGui::GetIO();
        if (io.WantCaptureKeyboard && io.WantTextInput)
        {
            return;
        }
    }
    p_Camera->HandleKeyState(m_DeltaTime / 1000.0f, p_Window);
}

void VulkanRenderer::HandleMouseMove()
{
    if (m_Settings.EnableUI)
    {
        ImGuiIO &io = ImGui::GetIO();
        if (io.WantCaptureMouse)
        {
            return;
        }
    }
    p_Camera->HandleMouseMove(m_DeltaTime / 1000.0f, p_Window);
}

void VulkanRenderer::HandleMouseButtonState()
{
    if (m_Settings.EnableUI)
    {
        ImGuiIO &io = ImGui::GetIO();
        if (io.WantCaptureMouse)
        {
            return;
        }
    }
    p_Camera->HandleMouseButtonState(m_DeltaTime / 1000.0f, p_Window);
}

void VulkanRenderer::HandleMouseScroll()
{
    if (m_Settings.EnableUI)
    {
        ImGuiIO &io = ImGui::GetIO();
        if (io.WantCaptureMouse)
        {
            return;
        }
    }
    p_Camera->HandleMouseScroll(m_DeltaTime / 1000.0f, p_Window);
}

void VulkanRenderer::WindowResize()
{
    if (m_Width == 0 || m_Height == 0)
    {
        return;
    }

    // Make sure all operations have been done
    vkDeviceWaitIdle(p_Device->GetDevice());
    p_SwapChain->RecreateSwapChainResources(m_Width, m_Height);

    m_Prepared = true;
}

VulkanModel *VulkanRenderer::LoadModel(const std::string &modelPath, ModelTypeFlags modelType, uint32_t binding, VkVertexInputRate inputRate)
{
    VulkanModel *pModel = new VulkanModel(modelPath, modelType, binding, inputRate, p_Device->GetDevice(), p_Allocator);
    for (uint32_t i = 0; i < m_Settings.MaxFramesInFlight; ++i)
    {
        pModel->m_TransformBuffers.push_back(std::move(VulkanBuffer(p_Allocator)));
        pModel->m_ColorTextures.push_back(std::move(VulkanTexture(p_Allocator)));
    }
    pModel->m_TransformSets.resize(m_Settings.MaxFramesInFlight);
    pModel->m_TextureSets.resize(m_Settings.MaxFramesInFlight);
    CreateVertexBuffer(pModel);
    CreateIndexBuffer(pModel);
    return pModel;
}

VulkanModel *VulkanRenderer::LoadModel(const std::vector<VulkanVertex> vertex, uint32_t binding, VkVertexInputRate inputRate, const std::vector<IndexType> index)
{
    VulkanModel *pModel = new VulkanModel(vertex, binding, inputRate, p_Device->GetDevice(), p_Allocator, index);
    for (uint32_t i = 0; i < m_Settings.MaxFramesInFlight; ++i)
    {
        pModel->m_TransformBuffers.push_back(std::move(VulkanBuffer(p_Allocator)));
        pModel->m_ColorTextures.push_back(std::move(VulkanTexture(p_Allocator)));
    }
    pModel->m_TransformSets.resize(m_Settings.MaxFramesInFlight);
    pModel->m_TextureSets.resize(m_Settings.MaxFramesInFlight);
    CreateVertexBuffer(pModel);
    CreateIndexBuffer(pModel);
    return pModel;
}

void VulkanRenderer::CreateVertexBuffer(VulkanModel *pModel)
{

    VkDeviceSize vertexSize = sizeof(VulkanVertex) * (pModel->GetVertexCount());
    VulkanBuffer vertexStaging{p_Allocator};
    p_Device->CreateBuffer(vertexSize,
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                           &vertexStaging,

                           pModel->GetVertexData());
    pModel->ClearVertexData();
    p_Device->CreateBuffer(vertexSize,
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           &(pModel->m_VertexBuffer),
                           p_Allocator);
    p_Device->CopyBuffer(&vertexStaging, &(pModel->m_VertexBuffer));
    vertexStaging.Destroy();
}

void VulkanRenderer::CreateIndexBuffer(VulkanModel *pModel)
{
    VkDeviceSize indexSize = sizeof(IndexType) * (pModel->GetIndexCount());
    VulkanBuffer indexStaging{p_Allocator};
    p_Device->CreateBuffer(indexSize,
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                           &indexStaging,

                           (pModel->GetIndexData()));
    pModel->ClearIndexData();
    p_Device->CreateBuffer(indexSize,
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           &(pModel->m_IndexBuffer),
                           p_Allocator);
    p_Device->CopyBuffer(&indexStaging, &(pModel->m_IndexBuffer));
    indexStaging.Destroy();
}

void VulkanRenderer::CreateUniformBuffers(VkDeviceSize bufferSize, VulkanBuffer *pBuffers, size_t bufferCount)
{
    for (size_t i = 0; i < bufferCount; ++i)
    {
        p_Device->CreateBuffer(bufferSize,
                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               (pBuffers + i),
                               p_Allocator);
        (pBuffers + i)->Map();
    }
}

void VulkanRenderer::LoadSkyBoxTextures(VulkanTexture *pTextures,
                                        size_t textureCount,
                                        const std::array<const char *, 6> &filePathes,
                                        bool generateMipmap,
                                        bool flipVerticallyOnLoad)
{
    std::vector<stbi_uc> pixels(2048 * 2048 * 4 * 6);
    VkDeviceSize size = 0;
    VkDeviceSize offsets[7] = {0};
    int width, height, channel;
    stbi_set_flip_vertically_on_load(flipVerticallyOnLoad);

    VkDeviceSize size_one = 0;
    for (size_t i = 0; i < 6; ++i)
    {
        stbi_uc *pixelData = stbi_load(filePathes[i], &width, &height, &channel, 4);
        if (pixelData == nullptr)
        {
            FATAL("Failed to load image at %s!", filePathes[i]);
        }
        size_one = width * height * 4;
        offsets[i + 1] = offsets[i] + size_one;
        size += size_one;
        if (size != (i + 1) * offsets[1])
        {
            FATAL("The images' extents are not the same!");
        }
        if (size > pixels.capacity())
        {
            pixels.reserve(size + size_one);
        }
        memcpy(pixels.data() + offsets[i], pixelData, size_one);
        stbi_image_free(pixelData);
    }

    VulkanBuffer staging{p_Allocator};
    p_Device->CreateBuffer(size,
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &staging,
                           pixels.data());

    for (size_t i = 0; i < textureCount; ++i)
    {
        // Create image and generate mipmaps
        if (m_QueueFamilyIndices.TransferHasValue || m_QueueFamilyIndices.GraphicsHasValue)
        {
            VkImageCreateInfo imageCI = vkinfo::ImageInfo();
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            imageCI.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageCI.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            if (generateMipmap)
            {
                imageCI.mipLevels = static_cast<uint32_t>(std::floor(std::log2((std::max)(width, height))) + 1);
            }
            else
            {
                imageCI.mipLevels = 1;
            }
            imageCI.arrayLayers = 6;
            imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCI.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            if (m_UniqueQueueFamilyIndices.size() > 1)
            {
                imageCI.queueFamilyIndexCount = static_cast<uint32_t>(m_UniqueQueueFamilyIndices.size());
                imageCI.pQueueFamilyIndices = m_UniqueQueueFamilyIndices.data();
                imageCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
            }
            else
            {
                imageCI.queueFamilyIndexCount = 0;
                imageCI.pQueueFamilyIndices = nullptr;
                imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }
            // This flag is required for cube map
            imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(p_Device->GetGPU(), imageCI.format, &formatProperties);
            if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            {
                FATAL("Texture image format does not support linear blitting!");
            }

            (pTextures + i)->Device = p_Device->GetDevice();
            (pTextures + i)->IsInitialized = true;
            (pTextures + i)->Allocator = p_Allocator;
            (pTextures + i)->Width = static_cast<uint32_t>(width);
            (pTextures + i)->Height = static_cast<uint32_t>(height);
            (pTextures + i)->Layout = VK_IMAGE_LAYOUT_UNDEFINED;
            (pTextures + i)->MipMapLevelCount = imageCI.mipLevels;
            (pTextures + i)->ArrayLayerCount = imageCI.arrayLayers;
            p_SwapChain->CreateImageWithInfo(&imageCI, &(pTextures + i)->Image, &(pTextures + i)->Memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);

            VkCommandBuffer cmdBuffer = p_Device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
            // Initialize resource
            VkImageSubresourceRange subresource{};
            subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresource.baseArrayLayer = 0;
            subresource.layerCount = (pTextures + i)->ArrayLayerCount;
            subresource.baseMipLevel = 0;
            subresource.levelCount = 1;
            VkImageMemoryBarrier barrier = vkinfo::ImageMemoryBarrier();
            barrier.image = (pTextures + i)->Image;

            // Transfer mipmap level 0 image layout
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_NONE;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.subresourceRange = subresource;
            vkCmdPipelineBarrier(cmdBuffer,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            // Copy buffer to image memory
            std::vector<VkBufferImageCopy> copyRegins = {};
            for (uint32_t j = 0; j < (pTextures + i)->ArrayLayerCount; ++j)
            {
                VkBufferImageCopy copyRegin{};
                copyRegin.imageExtent = {static_cast<uint32_t>((pTextures + i)->Width), static_cast<uint32_t>((pTextures + i)->Height), 1};
                copyRegin.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegin.imageSubresource.baseArrayLayer = j;
                copyRegin.imageSubresource.layerCount = 1;
                copyRegin.imageSubresource.mipLevel = 0;
                copyRegin.bufferOffset = offsets[j];
                copyRegins.push_back(copyRegin);
            }
            vkCmdCopyBufferToImage(cmdBuffer,
                                   staging.Buffer,
                                   (pTextures + i)->Image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   copyRegins.size(),
                                   copyRegins.data());

            // Transfer mipmap level 0 image layout
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.subresourceRange = subresource;
            vkCmdPipelineBarrier(cmdBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            if (m_QueueFamilyIndices.TransferHasValue)
            {
                p_Device->FlushCommandBuffer(cmdBuffer, m_Queues.Transfer);
            }
            else
            {
                p_Device->FlushCommandBuffer(cmdBuffer, m_Queues.Graphics);
            }

            // Generate mipmap images
            VkCommandBuffer blitCmd = p_Device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
            int32_t texWidth = static_cast<int32_t>((pTextures + i)->Width);
            int32_t texHeight = static_cast<int32_t>((pTextures + i)->Height);
            for (uint32_t j = 1; j < (pTextures + i)->MipMapLevelCount; ++j)
            {
                subresource.baseMipLevel = j;
                // Transfer "mipmap level image" layout to transfer dstination optimal from undefined
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_NONE;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.subresourceRange = subresource;
                vkCmdPipelineBarrier(blitCmd,
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);

                // Blit image
                VkImageBlit imageBlit{};
                imageBlit.srcOffsets[0] = {0, 0, 0};
                imageBlit.srcOffsets[1] = {texWidth, texHeight, 1};
                imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.srcSubresource.baseArrayLayer = 0;
                imageBlit.srcSubresource.layerCount = (pTextures + i)->ArrayLayerCount;
                imageBlit.srcSubresource.mipLevel = j - 1;
                imageBlit.dstOffsets[0] = {0, 0, 0};
                imageBlit.dstOffsets[1] = {(texWidth > 1 ? (texWidth /= 2, texWidth) : 1), (texHeight > 1 ? (texHeight /= 2, texHeight) : 1), 1};
                imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.dstSubresource.baseArrayLayer = 0;
                imageBlit.dstSubresource.layerCount = (pTextures + i)->ArrayLayerCount;
                imageBlit.dstSubresource.mipLevel = j;
                vkCmdBlitImage(blitCmd,
                               (pTextures + i)->Image,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               (pTextures + i)->Image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &imageBlit,
                               VK_FILTER_LINEAR);

                // Transfer "mipmap level image" layout to transfer dstination optimal from transfer source optimal
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.subresourceRange = subresource;
                vkCmdPipelineBarrier(blitCmd,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);
            }

            // Lastly, change the whole image layout to shader read only optimal so that can be sampled from
            subresource.baseMipLevel = 0;
            subresource.levelCount = (pTextures + i)->MipMapLevelCount;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.subresourceRange = subresource;
            vkCmdPipelineBarrier(blitCmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
            (pTextures + i)->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            if (m_QueueFamilyIndices.TransferHasValue)
            {
                p_Device->FlushCommandBuffer(blitCmd, m_Queues.Transfer);
            }
            else
            {
                p_Device->FlushCommandBuffer(blitCmd, m_Queues.Graphics);
            }
        }
        else
        {
            FATAL("It seems neither the transfer queue nor the graphics queue are enabled when initializing device!");
        }

        // Create image sampler
        VkSamplerCreateInfo samplerInfo = vkinfo::SamplerInfo();
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>((pTextures + i)->MipMapLevelCount);
        samplerInfo.anisotropyEnable = p_Device->m_GPUFeatures.samplerAnisotropy;
        if (p_Device->m_GPUFeatures.samplerAnisotropy != VK_TRUE)
        {
            WARNING("Device feature: sampler anisotrophy not support! Request at %s line: %d\n!", __FILE__, __LINE__);
        }
        samplerInfo.maxAnisotropy = 8.0f;
        CHECK_VK_RESULT(vkCreateSampler(p_Device->GetDevice(), &samplerInfo, p_Allocator, &(pTextures + i)->Sampler));

        VkImageViewCreateInfo viewInfo = vkinfo::ImageViewInfo();
        viewInfo.image = (pTextures + i)->Image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = (pTextures + i)->ArrayLayerCount;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = (pTextures + i)->MipMapLevelCount;
        viewInfo.components = {VK_COMPONENT_SWIZZLE_R,
                               VK_COMPONENT_SWIZZLE_G,
                               VK_COMPONENT_SWIZZLE_B,
                               VK_COMPONENT_SWIZZLE_A};
        CHECK_VK_RESULT(vkCreateImageView(p_Device->GetDevice(), &viewInfo, p_Allocator, &(pTextures + i)->View));
        (pTextures + i)->SetDescriptorImage();
    }
    staging.Destroy();
}

void VulkanRenderer::CreateSkyBox(const std::vector<VulkanVertex> vertex,
                                  const std::array<const char *, 6> &filePathes,
                                  bool generateMipmap,
                                  bool flipVerticallyOnLoad,
                                  const std::vector<IndexType> index)
{
    p_SkyBox = std::move(LoadModel(vertex, 0, VK_VERTEX_INPUT_RATE_VERTEX, index));
    LoadSkyBoxTextures(p_SkyBox->m_ColorTextures.data(), p_SkyBox->m_ColorTextures.size(), filePathes, generateMipmap, flipVerticallyOnLoad);
}

void VulkanRenderer::CreateSkyBox(const char *modelPath, const std::array<const char *, 6> &filePathes, bool generateMipmap, bool flipVerticallyOnLoad)
{
    p_SkyBox = std::move(LoadModel(modelPath, MODEL_TYPE_OBJ, 0, VK_VERTEX_INPUT_RATE_VERTEX));
    LoadSkyBoxTextures(p_SkyBox->m_ColorTextures.data(), p_SkyBox->m_ColorTextures.size(), filePathes, generateMipmap, flipVerticallyOnLoad);
}

void VulkanRenderer::CreateTextures(VulkanTexture *pTexture,
                                    size_t textureCount,
                                    opm::srgb *pColor)
{
    opm::srgb color(255, 255, 255, 100);
    if (pColor == nullptr)
    {
        pColor = &color;
    }
    else
    {
        if (pColor->r > 255)
        {
            pColor->r = 255;
        }
        if (pColor->g > 255)
        {
            pColor->g = 255;
        }
        if (pColor->b > 255)
        {
            pColor->b = 255;
        }
        if (pColor->a > 100)
        {
            pColor->a = 100;
        }
    }

    VulkanBuffer staging{p_Allocator};
    p_Device->CreateBuffer(sizeof(opm::srgb),
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &staging,
                           pColor);

    for (size_t i = 0; i < textureCount; ++i)
    {
        // Create image and generate mipmaps
        if (m_QueueFamilyIndices.TransferHasValue || m_QueueFamilyIndices.GraphicsHasValue)
        {
            VkImageCreateInfo imageCI = vkinfo::ImageInfo();
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            imageCI.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageCI.extent = {1, 1, 1};
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCI.mipLevels = 1;
            imageCI.arrayLayers = (pTexture + i)->ArrayLayerCount;
            imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCI.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            if (m_UniqueQueueFamilyIndices.size() > 1)
            {
                imageCI.queueFamilyIndexCount = static_cast<uint32_t>(m_UniqueQueueFamilyIndices.size());
                imageCI.pQueueFamilyIndices = m_UniqueQueueFamilyIndices.data();
                imageCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
            }
            else
            {
                imageCI.queueFamilyIndexCount = 0;
                imageCI.pQueueFamilyIndices = nullptr;
                imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(p_Device->GetGPU(), imageCI.format, &formatProperties);
            if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            {
                FATAL("Texture image format does not support linear blitting!");
            }

            (pTexture + i)->Device = p_Device->GetDevice();
            (pTexture + i)->IsInitialized = true;
            (pTexture + i)->Allocator = p_Allocator;
            (pTexture + i)->Width = 1;
            (pTexture + i)->Height = 1;
            (pTexture + i)->Layout = VK_IMAGE_LAYOUT_UNDEFINED;
            (pTexture + i)->MipMapLevelCount = imageCI.mipLevels;
            (pTexture + i)->ArrayLayerCount = 1;
            p_SwapChain->CreateImageWithInfo(&imageCI, &(pTexture + i)->Image, &(pTexture + i)->Memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);

            VkCommandBuffer cmdBuffer = p_Device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
            // Initialize resource
            VkImageSubresourceRange subresource{};
            subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresource.baseArrayLayer = 0;
            subresource.layerCount = (pTexture + i)->ArrayLayerCount;
            subresource.baseMipLevel = 0;
            subresource.levelCount = 1;
            VkImageMemoryBarrier barrier = vkinfo::ImageMemoryBarrier();
            barrier.image = (pTexture + i)->Image;

            // Transfer image layout
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_NONE;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.subresourceRange = subresource;
            vkCmdPipelineBarrier(cmdBuffer,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            // Copy buffer to image memory
            VkBufferImageCopy copyRegin{};
            copyRegin.imageExtent = {(pTexture + i)->Width, (pTexture + i)->Height, 1};
            copyRegin.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegin.imageSubresource.baseArrayLayer = 0;
            copyRegin.imageSubresource.layerCount = (pTexture + i)->ArrayLayerCount;
            copyRegin.imageSubresource.mipLevel = 0;
            vkCmdCopyBufferToImage(cmdBuffer,
                                   staging.Buffer,
                                   (pTexture + i)->Image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1,
                                   &copyRegin);

            // Transfer image layout
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.subresourceRange = subresource;
            vkCmdPipelineBarrier(cmdBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            if (m_QueueFamilyIndices.TransferHasValue)
            {
                p_Device->FlushCommandBuffer(cmdBuffer, m_Queues.Transfer);
            }
            else
            {
                p_Device->FlushCommandBuffer(cmdBuffer, m_Queues.Graphics);
            }

            (pTexture + i)->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
        else
        {
            FATAL("It seems neither the transfer queue nor the graphics queue are enabled when initializing device!");
        }

        // Create image sampler
        VkSamplerCreateInfo samplerInfo = vkinfo::SamplerInfo();
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>((pTexture + i)->MipMapLevelCount);
        samplerInfo.anisotropyEnable = p_Device->m_GPUFeatures.samplerAnisotropy;
        if (p_Device->m_GPUFeatures.samplerAnisotropy != VK_TRUE)
        {
            WARNING("Device feature: sampler anisotrophy not support! Request at %s line: %d\n!", __FILE__, __LINE__);
        }
        samplerInfo.maxAnisotropy = 8.0f;
        CHECK_VK_RESULT(vkCreateSampler(p_Device->GetDevice(), &samplerInfo, p_Allocator, &(pTexture + i)->Sampler));

        VkImageViewCreateInfo viewInfo = vkinfo::ImageViewInfo();
        viewInfo.image = (pTexture + i)->Image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = (pTexture + i)->ArrayLayerCount;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = (pTexture + i)->MipMapLevelCount;
        viewInfo.components = {VK_COMPONENT_SWIZZLE_R,
                               VK_COMPONENT_SWIZZLE_G,
                               VK_COMPONENT_SWIZZLE_B,
                               VK_COMPONENT_SWIZZLE_A};
        CHECK_VK_RESULT(vkCreateImageView(p_Device->GetDevice(), &viewInfo, p_Allocator, &(pTexture + i)->View));
        (pTexture + i)->SetDescriptorImage();
    }

    staging.Destroy();
}

void VulkanRenderer::CreateTextures(const std::string &filePath,
                                    VulkanTexture *pTexture,
                                    size_t textureCount,
                                    bool generateMipmap,
                                    bool flipVerticallyOnLoad)
{
    int width, height, channel;
    stbi_set_flip_vertically_on_load(flipVerticallyOnLoad);
    stbi_uc *pixels = stbi_load(filePath.c_str(), &width, &height, &channel, 4);
    if (pixels == nullptr)
    {
        FATAL("Failed to load texture at %s!", filePath.c_str());
    }
    VkDeviceSize pixelSize = width * height * 4;

    VulkanBuffer staging{p_Allocator};
    p_Device->CreateBuffer(pixelSize,
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &staging,
                           pixels);
    stbi_image_free(pixels);

    for (size_t i = 0; i < textureCount; ++i)
    {
        // Create image and generate mipmaps
        if (m_QueueFamilyIndices.TransferHasValue || m_QueueFamilyIndices.GraphicsHasValue)
        {
            VkImageCreateInfo imageCI = vkinfo::ImageInfo();
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            imageCI.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageCI.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            if (generateMipmap)
            {
                imageCI.mipLevels = static_cast<uint32_t>(std::floor(std::log2((std::max)(width, height))) + 1);
            }
            else
            {
                imageCI.mipLevels = 1;
            }
            imageCI.arrayLayers = (pTexture + i)->ArrayLayerCount;
            imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCI.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            if (m_UniqueQueueFamilyIndices.size() > 1)
            {
                imageCI.queueFamilyIndexCount = static_cast<uint32_t>(m_UniqueQueueFamilyIndices.size());
                imageCI.pQueueFamilyIndices = m_UniqueQueueFamilyIndices.data();
                imageCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
            }
            else
            {
                imageCI.queueFamilyIndexCount = 0;
                imageCI.pQueueFamilyIndices = nullptr;
                imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(p_Device->GetGPU(), imageCI.format, &formatProperties);
            if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            {
                FATAL("Texture image format does not support linear blitting!");
            }

            (pTexture + i)->Device = p_Device->GetDevice();
            (pTexture + i)->IsInitialized = true;
            (pTexture + i)->Allocator = p_Allocator;
            (pTexture + i)->Width = static_cast<uint32_t>(width);
            (pTexture + i)->Height = static_cast<uint32_t>(height);
            (pTexture + i)->Layout = VK_IMAGE_LAYOUT_UNDEFINED;
            (pTexture + i)->MipMapLevelCount = imageCI.mipLevels;
            (pTexture + i)->ArrayLayerCount = imageCI.arrayLayers;
            p_SwapChain->CreateImageWithInfo(&imageCI, &(pTexture + i)->Image, &(pTexture + i)->Memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);

            VkCommandBuffer cmdBuffer = p_Device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
            // Initialize resource
            VkImageSubresourceRange subresource{};
            subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresource.baseArrayLayer = 0;
            subresource.layerCount = (pTexture + i)->ArrayLayerCount;
            subresource.baseMipLevel = 0;
            subresource.levelCount = 1;
            VkImageMemoryBarrier barrier = vkinfo::ImageMemoryBarrier();
            barrier.image = (pTexture + i)->Image;

            // Transfer mipmap level 0 image layout
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_NONE;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.subresourceRange = subresource;
            vkCmdPipelineBarrier(cmdBuffer,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            // Copy buffer to image memory
            VkBufferImageCopy copyRegin{};
            copyRegin.imageExtent = {(pTexture + i)->Width, (pTexture + i)->Height, 1};
            copyRegin.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegin.imageSubresource.baseArrayLayer = 0;
            copyRegin.imageSubresource.layerCount = (pTexture + i)->ArrayLayerCount;
            copyRegin.imageSubresource.mipLevel = 0;
            vkCmdCopyBufferToImage(cmdBuffer,
                                   staging.Buffer,
                                   (pTexture + i)->Image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1,
                                   &copyRegin);

            // Transfer mipmap level 0 image layout
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.subresourceRange = subresource;
            vkCmdPipelineBarrier(cmdBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            if (m_QueueFamilyIndices.TransferHasValue)
            {
                p_Device->FlushCommandBuffer(cmdBuffer, m_Queues.Transfer);
            }
            else
            {
                p_Device->FlushCommandBuffer(cmdBuffer, m_Queues.Graphics);
            }

            // Generate mipmap images
            VkCommandBuffer blitCmd = p_Device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
            int32_t texWidth = static_cast<int32_t>((pTexture + i)->Width);
            int32_t texHeight = static_cast<int32_t>((pTexture + i)->Height);
            for (uint32_t j = 1; j < (pTexture + i)->MipMapLevelCount; ++j)
            {
                subresource.baseMipLevel = j;
                // Transfer "mipmap level image" layout to transfer dstination optimal from undefined
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_NONE;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.subresourceRange = subresource;
                vkCmdPipelineBarrier(blitCmd,
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);

                // Blit image
                VkImageBlit imageBlit{};
                imageBlit.srcOffsets[0] = {0, 0, 0};
                imageBlit.srcOffsets[1] = {texWidth, texHeight, 1};
                imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.srcSubresource.baseArrayLayer = 0;
                imageBlit.srcSubresource.layerCount = (pTexture + i)->ArrayLayerCount;
                imageBlit.srcSubresource.mipLevel = j - 1;
                imageBlit.dstOffsets[0] = {0, 0, 0};
                imageBlit.dstOffsets[1] = {(texWidth > 1 ? (texWidth /= 2, texWidth) : 1), (texHeight > 1 ? (texHeight /= 2, texHeight) : 1), 1};
                imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.dstSubresource.baseArrayLayer = 0;
                imageBlit.dstSubresource.layerCount = (pTexture + i)->ArrayLayerCount;
                imageBlit.dstSubresource.mipLevel = j;
                vkCmdBlitImage(blitCmd,
                               (pTexture + i)->Image,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               (pTexture + i)->Image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &imageBlit,
                               VK_FILTER_LINEAR);

                // Transfer "mipmap level image" layout to transfer dstination optimal from transfer source optimal
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.subresourceRange = subresource;
                vkCmdPipelineBarrier(blitCmd,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);
            }

            // Lastly, change the whole image layout to shader read only optimal so that can be sampled from
            subresource.baseMipLevel = 0;
            subresource.levelCount = (pTexture + i)->MipMapLevelCount;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.subresourceRange = subresource;
            vkCmdPipelineBarrier(blitCmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
            (pTexture + i)->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            if (m_QueueFamilyIndices.TransferHasValue)
            {
                p_Device->FlushCommandBuffer(blitCmd, m_Queues.Transfer);
            }
            else
            {
                p_Device->FlushCommandBuffer(blitCmd, m_Queues.Graphics);
            }
        }
        else
        {
            FATAL("It seems neither the transfer queue nor the graphics queue are enabled when initializing device!");
        }

        // Create image sampler
        VkSamplerCreateInfo samplerInfo = vkinfo::SamplerInfo();
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>((pTexture + i)->MipMapLevelCount);
        samplerInfo.anisotropyEnable = p_Device->m_GPUFeatures.samplerAnisotropy;
        if (p_Device->m_GPUFeatures.samplerAnisotropy != VK_TRUE)
        {
            WARNING("Device feature: sampler anisotrophy not support! Request at %s line: %d\n!", __FILE__, __LINE__);
        }
        samplerInfo.maxAnisotropy = 8.0f;
        CHECK_VK_RESULT(vkCreateSampler(p_Device->GetDevice(), &samplerInfo, p_Allocator, &(pTexture + i)->Sampler));

        VkImageViewCreateInfo viewInfo = vkinfo::ImageViewInfo();
        viewInfo.image = (pTexture + i)->Image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = (pTexture + i)->ArrayLayerCount;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = (pTexture + i)->MipMapLevelCount;
        viewInfo.components = {VK_COMPONENT_SWIZZLE_R,
                               VK_COMPONENT_SWIZZLE_G,
                               VK_COMPONENT_SWIZZLE_B,
                               VK_COMPONENT_SWIZZLE_A};
        CHECK_VK_RESULT(vkCreateImageView(p_Device->GetDevice(), &viewInfo, p_Allocator, &(pTexture + i)->View));
        (pTexture + i)->SetDescriptorImage();
    }

    staging.Destroy();
}

void VulkanRenderer::CreateTextureArray(const std::vector<const char *> &filePathes,
                                        VulkanTexture *pTextures,
                                        size_t textureCount,
                                        bool generateMipmap,
                                        bool flipVerticallyOnLoad)
{
    std::vector<stbi_uc> pixels(2048 * 2048 * 4 * 6);
    VkDeviceSize size = 0;
    std::vector<VkDeviceSize> offsets(filePathes.size() + 1, 0);
    int width, height, channel;
    stbi_set_flip_vertically_on_load(flipVerticallyOnLoad);

    VkDeviceSize size_one = 0;
    for (size_t i = 0; i < filePathes.size(); ++i)
    {
        stbi_uc *pixelData = stbi_load(filePathes[i], &width, &height, &channel, 4);
        if (pixelData == nullptr)
        {
            FATAL("Failed to load image at %s!", filePathes[i]);
        }
        size_one = width * height * 4;
        offsets[i + 1] = offsets[i] + size_one;
        size += size_one;
        if (size != (i + 1) * offsets[1])
        {
            FATAL("The images' extents are not the same!");
        }
        if (size > pixels.capacity())
        {
            pixels.reserve(size + size_one);
        }
        memcpy(pixels.data() + offsets[i], pixelData, size_one);
        stbi_image_free(pixelData);
    }

    VulkanBuffer staging{p_Allocator};
    p_Device->CreateBuffer(size,
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &staging,
                           pixels.data());

    for (size_t i = 0; i < textureCount; ++i)
    {
        // Create image and generate mipmaps
        if (m_QueueFamilyIndices.TransferHasValue || m_QueueFamilyIndices.GraphicsHasValue)
        {
            VkImageCreateInfo imageCI = vkinfo::ImageInfo();
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            imageCI.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageCI.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            if (generateMipmap)
            {
                imageCI.mipLevels = static_cast<uint32_t>(std::floor(std::log2((std::max)(width, height))) + 1);
            }
            else
            {
                imageCI.mipLevels = 1;
            }
            imageCI.arrayLayers = filePathes.size();
            imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCI.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            if (m_UniqueQueueFamilyIndices.size() > 1)
            {
                imageCI.queueFamilyIndexCount = static_cast<uint32_t>(m_UniqueQueueFamilyIndices.size());
                imageCI.pQueueFamilyIndices = m_UniqueQueueFamilyIndices.data();
                imageCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
            }
            else
            {
                imageCI.queueFamilyIndexCount = 0;
                imageCI.pQueueFamilyIndices = nullptr;
                imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(p_Device->GetGPU(), imageCI.format, &formatProperties);
            if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            {
                FATAL("Texture image format does not support linear blitting!");
            }

            (pTextures + i)->Device = p_Device->GetDevice();
            (pTextures + i)->IsInitialized = true;
            (pTextures + i)->Allocator = p_Allocator;
            (pTextures + i)->Width = static_cast<uint32_t>(width);
            (pTextures + i)->Height = static_cast<uint32_t>(height);
            (pTextures + i)->Layout = VK_IMAGE_LAYOUT_UNDEFINED;
            (pTextures + i)->MipMapLevelCount = imageCI.mipLevels;
            (pTextures + i)->ArrayLayerCount = imageCI.arrayLayers;
            p_SwapChain->CreateImageWithInfo(&imageCI, &(pTextures + i)->Image, &(pTextures + i)->Memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);

            VkCommandBuffer cmdBuffer = p_Device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
            // Initialize resource
            VkImageSubresourceRange subresource{};
            subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresource.baseArrayLayer = 0;
            subresource.layerCount = (pTextures + i)->ArrayLayerCount;
            subresource.baseMipLevel = 0;
            subresource.levelCount = 1;
            VkImageMemoryBarrier barrier = vkinfo::ImageMemoryBarrier();
            barrier.image = (pTextures + i)->Image;

            // Transfer mipmap level 0 image layout
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_NONE;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.subresourceRange = subresource;
            vkCmdPipelineBarrier(cmdBuffer,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            // Copy buffer to image memory
            std::vector<VkBufferImageCopy> copyRegins = {};
            for (uint32_t j = 0; j < (pTextures + i)->ArrayLayerCount; ++j)
            {
                VkBufferImageCopy copyRegin{};
                copyRegin.imageExtent = {static_cast<uint32_t>((pTextures + i)->Width), static_cast<uint32_t>((pTextures + i)->Height), 1};
                copyRegin.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegin.imageSubresource.baseArrayLayer = j;
                copyRegin.imageSubresource.layerCount = 1;
                copyRegin.imageSubresource.mipLevel = 0;
                copyRegin.bufferOffset = offsets[j];
                copyRegins.push_back(copyRegin);
            }
            vkCmdCopyBufferToImage(cmdBuffer,
                                   staging.Buffer,
                                   (pTextures + i)->Image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   copyRegins.size(),
                                   copyRegins.data());

            // Transfer mipmap level 0 image layout
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.subresourceRange = subresource;
            vkCmdPipelineBarrier(cmdBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            if (m_QueueFamilyIndices.TransferHasValue)
            {
                p_Device->FlushCommandBuffer(cmdBuffer, m_Queues.Transfer);
            }
            else
            {
                p_Device->FlushCommandBuffer(cmdBuffer, m_Queues.Graphics);
            }

            // Generate mipmap images
            VkCommandBuffer blitCmd = p_Device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
            int32_t texWidth = static_cast<int32_t>((pTextures + i)->Width);
            int32_t texHeight = static_cast<int32_t>((pTextures + i)->Height);
            for (uint32_t j = 1; j < (pTextures + i)->MipMapLevelCount; ++j)
            {
                subresource.baseMipLevel = j;
                // Transfer "mipmap level image" layout to transfer dstination optimal from undefined
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_NONE;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.subresourceRange = subresource;
                vkCmdPipelineBarrier(blitCmd,
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);

                // Blit image
                VkImageBlit imageBlit{};
                imageBlit.srcOffsets[0] = {0, 0, 0};
                imageBlit.srcOffsets[1] = {texWidth, texHeight, 1};
                imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.srcSubresource.baseArrayLayer = 0;
                imageBlit.srcSubresource.layerCount = (pTextures + i)->ArrayLayerCount;
                imageBlit.srcSubresource.mipLevel = j - 1;
                imageBlit.dstOffsets[0] = {0, 0, 0};
                imageBlit.dstOffsets[1] = {(texWidth > 1 ? (texWidth /= 2, texWidth) : 1), (texHeight > 1 ? (texHeight /= 2, texHeight) : 1), 1};
                imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.dstSubresource.baseArrayLayer = 0;
                imageBlit.dstSubresource.layerCount = (pTextures + i)->ArrayLayerCount;
                imageBlit.dstSubresource.mipLevel = j;
                vkCmdBlitImage(blitCmd,
                               (pTextures + i)->Image,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               (pTextures + i)->Image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &imageBlit,
                               VK_FILTER_LINEAR);

                // Transfer "mipmap level image" layout to transfer dstination optimal from transfer source optimal
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.subresourceRange = subresource;
                vkCmdPipelineBarrier(blitCmd,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);
            }

            // Lastly, change the whole image layout to shader read only optimal so that can be sampled from
            subresource.baseMipLevel = 0;
            subresource.levelCount = (pTextures + i)->MipMapLevelCount;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.subresourceRange = subresource;
            vkCmdPipelineBarrier(blitCmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
            (pTextures + i)->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            if (m_QueueFamilyIndices.TransferHasValue)
            {
                p_Device->FlushCommandBuffer(blitCmd, m_Queues.Transfer);
            }
            else
            {
                p_Device->FlushCommandBuffer(blitCmd, m_Queues.Graphics);
            }
        }
        else
        {
            FATAL("It seems neither the transfer queue nor the graphics queue are enabled when initializing device!");
        }

        // Create image sampler
        VkSamplerCreateInfo samplerInfo = vkinfo::SamplerInfo();
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>((pTextures + i)->MipMapLevelCount);
        samplerInfo.anisotropyEnable = p_Device->m_GPUFeatures.samplerAnisotropy;
        if (p_Device->m_GPUFeatures.samplerAnisotropy != VK_TRUE)
        {
            WARNING("Device feature: sampler anisotrophy not support! Request at %s line: %d\n!", __FILE__, __LINE__);
        }
        samplerInfo.maxAnisotropy = 8.0f;
        CHECK_VK_RESULT(vkCreateSampler(p_Device->GetDevice(), &samplerInfo, p_Allocator, &(pTextures + i)->Sampler));

        VkImageViewCreateInfo viewInfo = vkinfo::ImageViewInfo();
        viewInfo.image = (pTextures + i)->Image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = (pTextures + i)->ArrayLayerCount;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = (pTextures + i)->MipMapLevelCount;
        viewInfo.components = {VK_COMPONENT_SWIZZLE_R,
                               VK_COMPONENT_SWIZZLE_G,
                               VK_COMPONENT_SWIZZLE_B,
                               VK_COMPONENT_SWIZZLE_A};
        CHECK_VK_RESULT(vkCreateImageView(p_Device->GetDevice(), &viewInfo, p_Allocator, &(pTextures + i)->View));
        (pTextures + i)->SetDescriptorImage();
    }
    staging.Destroy();
}

void VulkanRenderer::UpdateUniformBuffers(VulkanBuffer *pBuffers, uint32_t bufferCount, const void *data)
{
    for (uint32_t i = 0; i < bufferCount; ++i)
    {
        if ((pBuffers + i)->Mapped == nullptr)
        {
            FATAL("Buffer does not be mapped yet!");
        }
        (pBuffers + i)->CopyData(data, (pBuffers + i)->Size);
        (pBuffers + i)->Flush();
    }
}

void VulkanRenderer::PrepareUI(VkRenderPass renderPass,
                               uint32_t subpass,
                               VkFormat colorFormat,
                               VkFormat depthFormat,
                               VkFormat stencilFormat,
                               const std::string &vertFilePath,
                               const std::string &fragFilePath)
{
    if (m_Settings.EnableUI)
    {
        p_UI->PrepareDescriptors();
        p_UI->PreparePipeline(vertFilePath,
                              fragFilePath,
                              renderPass,
                              subpass,
                              colorFormat,
                              depthFormat,
                              stencilFormat);
    }
}

void VulkanRenderer::DrawUI(VkCommandBuffer cmdBuffer, uint32_t currentFrame)
{
    if (m_Settings.EnableUI)
    {
        p_UI->Draw(cmdBuffer, currentFrame);
    }
}

VkResult VulkanRenderer::AcquireSwapChainImage()
{
    CHECK_VK_RESULT(vkWaitForFences(p_Device->GetDevice(), 1, &m_GraphicsInFlightFences[p_SwapChain->m_CurrentFrame], VK_TRUE, DEFAULT_FENCE_TIMEOUT));
    CHECK_VK_RESULT(vkResetFences(p_Device->GetDevice(), 1, &m_GraphicsInFlightFences[p_SwapChain->m_CurrentFrame]));

    VkResult result = vkAcquireNextImageKHR(p_Device->GetDevice(),
                                            p_SwapChain->GetSwapChain(),
                                            DEFAULT_FENCE_TIMEOUT,
                                            m_ImageAvailableSemaphores[p_SwapChain->m_CurrentFrame],
                                            VK_NULL_HANDLE,
                                            &m_CurrentImageIndex);

    return result;
}

VkCommandBuffer VulkanRenderer::BeginFrame()
{
    if (m_BeginFrame)
    {
        FATAL("Can not begin frame without ending exist frame!");
    }

    VkResult result = AcquireSwapChainImage();
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        WindowResize();
        m_FrameBufferResizing = false;
        return VK_NULL_HANDLE;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        FATAL("Failed to acquire image!");
    }

    m_BeginFrame = true;

    VkCommandBuffer cmdBuffer = m_DrawCmdBuffers[p_SwapChain->m_CurrentFrame];
    VkCommandBufferBeginInfo beginInfo = vkinfo::CommandBufferBeginInfo();
    CHECK_VK_RESULT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

    if (m_Settings.EnableUI)
    {
        ImGui::NewFrame();
    }

    return cmdBuffer;
}

void VulkanRenderer::BeginRenderPass(VkCommandBuffer cmdBuffer, VkRenderPass renderPass)
{
    if (!m_BeginFrame)
    {
        FATAL("Can not begin render pass without starting a frame!");
    }
    if (m_BeginRenderPass)
    {
        FATAL("Can not begin render pass without ending exist render pass");
    }
    if (cmdBuffer != m_DrawCmdBuffers[p_SwapChain->m_CurrentFrame])
    {
        FATAL("Can not begin render pass for command buffer from different frame!");
    }

    VkExtent2D swapChainExtent = p_SwapChain->GetImageExtent();
    VkClearValue clearValues[2];
    clearValues[0].color = {0.f, 0.f, 0.f, 0.f};
    clearValues[1].depthStencil = {1.f, 0};
    VkRenderPassBeginInfo beginInfo = vkinfo::RenderPassBeginInfo(renderPass, p_SwapChain->GetFrameBuffer(m_CurrentImageIndex));
    beginInfo.renderArea.offset = {0, 0};
    beginInfo.renderArea.extent = swapChainExtent;
    beginInfo.clearValueCount = 2;
    beginInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = static_cast<uint32_t>(swapChainExtent.width);
    viewport.height = static_cast<uint32_t>(swapChainExtent.height);
    viewport.maxDepth = 1.f;
    viewport.minDepth = 0.f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    m_BeginRenderPass = true;
}

void VulkanRenderer::ShowUI()
{
    if (m_Settings.EnableUI)
    {
        RenderUI();
        UpdateUI();
    }
}

void VulkanRenderer::RenderUI()
{
    ImGui::Begin("Device Info");
    ImGui::SetWindowPos(ImVec2(static_cast<float>(400 * p_UI->m_GlobalScale),
                               static_cast<float>(20 * p_UI->m_GlobalScale)),
                        ImGuiCond_FirstUseEver);
    ImGui::SetWindowSize(ImVec2(600 * p_UI->m_GlobalScale, 600 * p_UI->m_GlobalScale), ImGuiCond_FirstUseEver);
    ImGui::TextUnformatted(glfwGetWindowTitle(p_Window));
    ImGui::TextUnformatted(p_Device->m_GPUProperties.deviceName);
    ImGui::Text("Vulkan API %i.%i.%i.%i",
                VERSION_VARIANT(p_Device->m_GPUProperties.apiVersion),
                VERSION_MAJOR(p_Device->m_GPUProperties.apiVersion),
                VERSION_MINOR(p_Device->m_GPUProperties.apiVersion),
                VERSION_PATCH(p_Device->m_GPUProperties.apiVersion));
    if (p_Device->ExtensionSupport(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME) && API_VERSION > VK_API_VERSION_1_0)
    {
        ImGui::Text("Driver information %s %s", p_Device->m_GPUDriverProperties.driverName, p_Device->m_GPUDriverProperties.driverInfo);
    }
    ImGui::Text("%u FPS", m_FPS);
    if (m_FrameCount == 0 && m_FrameTimes.size() != 0)
    {
        if (m_FrameTimes.front() < m_MinFrameTime)
        {
            m_MinFrameTime = m_FrameTimes.front();
        }
        if (m_FrameTimes.front() > m_MaxFrameTime)
        {
            m_MaxFrameTime = m_FrameTimes.front();
        }
    }
    ImGui::PlotLines("Average ms/frame", m_FrameTimes.data(), static_cast<int>(m_FrameTimes.size()), 0, "", m_MinFrameTime, m_MaxFrameTime, ImVec2(0.0f, 80.0f));

    ImGui::Checkbox("Show Demo Window", &m_Settings.ShowDemoWindow);
    if (m_Settings.ShowDemoWindow)
    {
        ImGui::ShowDemoWindow();
    }

    char buffer[1025] = {};
    ImGui::InputText("Input", buffer, sizeof(buffer));
    ImGui::Text("Output: %s", buffer);
    memset(buffer, 0, sizeof(buffer));

    ImGui::End();
}

void VulkanRenderer::UpdateUI()
{
    ImGui::Render();
    p_UI->Update(p_SwapChain->m_CurrentFrame);
}

void VulkanRenderer::EndRenderPass(VkCommandBuffer cmdBuffer)
{
    if (!m_BeginFrame)
    {
        FATAL("Can not end render pass while no frame is started!");
    }
    if (!m_BeginRenderPass)
    {
        FATAL("Can not end render pass while no render pass is started!");
    }
    if (cmdBuffer != m_DrawCmdBuffers[p_SwapChain->m_CurrentFrame])
    {
        FATAL("Can not end render pass for command buffer from different frame!");
    }

    ShowUI();
    DrawUI(cmdBuffer, p_SwapChain->m_CurrentFrame);

    vkCmdEndRenderPass(cmdBuffer);

    m_BeginRenderPass = false;
}

void VulkanRenderer::CommitAllSubmits()
{
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit = vkinfo::SubmitInfo();
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &m_DrawCmdBuffers[p_SwapChain->m_CurrentFrame];
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &m_ImageAvailableSemaphores[p_SwapChain->m_CurrentFrame];
    submit.pWaitDstStageMask = &waitStage;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &m_RenderFinishedSemaphores[p_SwapChain->m_CurrentFrame];

    CHECK_VK_RESULT(vkQueueSubmit(m_Queues.Graphics, 1, &submit, m_GraphicsInFlightFences[p_SwapChain->m_CurrentFrame]));
}

VkResult VulkanRenderer::PresentImage()
{
    VkPresentInfoKHR presentInfo = vkinfo::PresentInfo();
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[p_SwapChain->m_CurrentFrame];

    VkSwapchainKHR swapChain = p_SwapChain->GetSwapChain();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;

    presentInfo.pImageIndices = &m_CurrentImageIndex;

    VkResult result = vkQueuePresentKHR(m_Queues.Present, &presentInfo);

    p_SwapChain->m_CurrentFrame = (p_SwapChain->m_CurrentFrame + 1) % m_Settings.MaxFramesInFlight;

    return result;
}

void VulkanRenderer::EndFrame()
{
    if (!m_BeginFrame)
    {
        FATAL("Can not end frame while no frame is started!");
    }

    VkCommandBuffer cmdBuffer = m_DrawCmdBuffers[p_SwapChain->m_CurrentFrame];
    CHECK_VK_RESULT(vkEndCommandBuffer(cmdBuffer));

    CommitAllSubmits();
    VkResult result = PresentImage();
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FrameBufferResizing)
    {
        WindowResize();
        m_FrameBufferResizing = false;
    }
    else if (result != VK_SUCCESS)
    {
        FATAL("Failed to submit command buffer!");
    }

    if (m_Settings.EnableUI)
    {
        ImGui::EndFrame();
    }

    m_BeginFrame = false;
}

void VulkanRenderer::NextFrame()
{
    if (m_Paused || !m_Prepared)
    {
        return;
    }

    if (m_Settings.EnableUI)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.DeltaTime = m_DeltaTime;
    }

    m_LastTime = std::chrono::high_resolution_clock::now();
    Render();
    m_DeltaTime = std::chrono::duration<float, std::chrono::milliseconds::period>(std::chrono::high_resolution_clock::now() - m_LastTime).count();
    m_FrameTime += m_DeltaTime;
    ++m_FrameCount;
    if (m_FrameTime - 1000.0f > opm::MATH_FLT_EPSILON)
    {
        m_FrameTimes.front() = m_FrameTime / static_cast<float>(m_FrameCount);
        std::rotate(m_FrameTimes.begin(), m_FrameTimes.begin() + 1, m_FrameTimes.end());
        INFO("%u FPS\n", m_FrameCount);
        m_FPS = m_FrameCount;
        m_FrameCount = 0;
        m_FrameTime = 0.0f;
    }
}

void VulkanRenderer::MainLoop()
{
    while (!glfwWindowShouldClose(p_Window))
    {
        glfwPollEvents();

        HandleKeyState();
        HandleMouseMove();
        HandleMouseButtonState();
        HandleMouseScroll();
        if (!glfwGetWindowAttrib(p_Window, GLFW_ICONIFIED))
        {
            NextFrame();
        }
    }
    vkDeviceWaitIdle(p_Device->GetDevice());
}
