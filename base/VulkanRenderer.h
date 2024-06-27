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

// Do this
// #include "VulkanCore.h"
// #include "VulkanRenderer.h"
// VulkanRenderer *pBase = nullptr;
// In your entry point(main function)
// try
// {
//     pBase = new YourExample();
//     pBase->InitVulkan();
//     pBase->SetUpWindow(...);
//     ```                                                                                                                                   ``` |
//     p_Device->InitDevice(p_Instance->GetInstance(), p_SwapChain->GetSurface(), QUEUE_TYPE_ALL, &m_QueueFamilyIndices, &m_Queues);             |
//     m_DrawCmdPool = p_Device->CreateCommandPool(m_QueueFamilyIndices.Graphics);                                                               |
//     p_SwapChain->Connect(p_Device->GetGPU(), p_Device->GetDevice(), &m_QueueFamilyIndices, &m_Queues, m_Settings.MaxFramesInFlight);          |
//     p_SwapChain->InitSwapChain(m_Settings.EnableVsync);                                                                                       |
//     p_SwapChain->CreateSwapChain(&m_Width, &m_Height);                                                                                        | pBase->Prepare();
//     AllocateCommandBuffers();                                                                                                                 |
//     p_SwapChain->CreateDepthStencilImageBuffer();                                                                                             |
//     ```                                                                                                                                   ``` |
//     **pBase->Model();**
//     **pBase->Descriptors();**
//     **pBase->Pipeline();**
//     pBase->MainLoop();
// }
// catch (std::exception &e)
// {
//     std::cerr << e.what() << std::endl;
// }
// You may override these functions, and NOTE that Render function is an interface.

#ifndef VULKAN_RENDERER_HEADER
#define VULKAN_RENDERER_HEADER

#pragma once

#include "VulkanCore.h"
#include "VulkanTools.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanModel.h"
#include "VulkanRenderSystem.h"
#include "VulkanCamera.h"
#include "VulkanUI.h"

#include <string>
#include <array>
#include <chrono>

/**
 * @brief The Vulkan base class. This contains VulkanInstance, VulkanDevice and VulkanSwapChain class.
 * @note Render interface is reserved to be implemented by user's Render method.
 */
class DVAPI_ATTR VulkanRenderer
{
public:
    struct Settings
    {
        // Enable validation layer
        bool EnableValidationLayer = true;
        // Full screen mode
        bool FullScreenMode = false;
        // Enable V-Sync
        bool EnableVsync = true;
        // Enable ImGui UI
        bool EnableUI = true;
        // Show Dear ImGui Demo Window
        bool ShowDemoWindow = false;
        /**
         * @brief Frames-in-flight. This controls how many frames should be processed concurrently.
         * @warning This can only be used after initialization(after calling InitVulkan function).
         */
        const uint32_t MaxFramesInFlight = 2;
    } m_Settings;

public:
    explicit VulkanRenderer(CameraTypeFlags camType = CAMERA_TYPE_LOOK_AT, const VkAllocationCallbacks *pAllocator = nullptr);
    virtual ~VulkanRenderer();
    VulkanRenderer(const VulkanRenderer &) = delete;
    VulkanRenderer(VulkanRenderer &&) = delete;
    VulkanRenderer &operator=(const VulkanRenderer &) = delete;
    VulkanRenderer &operator=(VulkanRenderer &&) = delete;

    // frame buffer resize flag
    bool m_FrameBufferResizing = false;
    // Swap chain image index
    uint32_t m_CurrentImageIndex = 0;
    // Synchronization objects for each frame
    std::vector<VkFence> m_GraphicsInFlightFences = {};
    // Synchronization objects for each frame
    std::vector<VkSemaphore> m_ImageAvailableSemaphores = {};
    // Synchronization objects for each frame
    std::vector<VkSemaphore> m_RenderFinishedSemaphores = {};

public:
    // GLFW frame buffer resize call back
    static void FrameBufferResizeCallback(GLFWwindow *window, int width, int height);
    // GLFW key call back
    static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    // GLFW mouse move call back
    static void MouseMoveCallback(GLFWwindow *window, double xpos, double ypos);
    // GLFW mouse call back
    static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    // GLFW mouse scroll call back
    static void MouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
    // GLFW iconify window
    static void IconifyWindowCallback(GLFWwindow *window, int inconify);

    // (Virtual) Set up window and create surface
    virtual void SetUpWindow(GLFWframebuffersizefun frameCallback = VulkanRenderer::FrameBufferResizeCallback,
                             GLFWkeyfun keyCallback = VulkanRenderer::KeyCallback,
                             GLFWcursorposfun mouseMoveCallback = VulkanRenderer::MouseMoveCallback,
                             GLFWmousebuttonfun mouseButtonCallback = VulkanRenderer::MouseButtonCallback,
                             GLFWscrollfun mouseScrollCallback = VulkanRenderer::MouseScrollCallback);
    // (Virtual Final) Initialize class resources, set max frames-in-flight and create VulkanInstance, VulkanDevice, VulkanSwapChain entities
    virtual void InitVulkan(uint32_t maxFramesInFilght = 2) final;

    //============ This never lose input event! ============//
    // (Virtual) Window resizing
    virtual void OnWindowResize(int width, int height);
    // (Virtual) Handle keyboard
    virtual void OnKeyState(int key, int scancode, int action, int mods);
    // (Virtual) Handle mouse move
    virtual void OnMouseMove(double xpos, double ypos);
    // (Virtual) Handle mouse button
    virtual void OnMouseButtonState(int button, int action, int mods);
    // (Virtual) Handle mouse scroll
    virtual void OnMouseScroll(double xoffset, double yoffset);
    //============ This never lose input event! ============//

    // (Virtual) Synchronization objects for graphics
    virtual void CreateSyncObjects();
    // Prepare render resources
    virtual void Prepare();

    //============ This might lose the input event, use carefully! ============//
    // (Virtual) Handle key state in game loop instead of polling events
    virtual void HandleKeyState();
    // (Virtual) Handle mouse move in game loop instead of polling events
    virtual void HandleMouseMove();
    // (Virtual) Handle mouse button state in game loop instead of polling events
    virtual void HandleMouseButtonState();
    // (Virtual) Handle mouse scroll in game loop instead of polling events
    virtual void HandleMouseScroll();
    //============ This might lose the input event, use carefully! ============//

    // (Virtual) Next frame, this will call the Render function and caculate frame time
    virtual void NextFrame();
    // (Virtual) The main game loop
    virtual void MainLoop();
    // (Pure Virtual)Render interface
    virtual void Render() = 0;

protected:
    const VkAllocationCallbacks *p_Allocator = nullptr;
    VulkanInstance *p_Instance = nullptr;
    VulkanDevice *p_Device = nullptr;
    VulkanSwapChain *p_SwapChain = nullptr;
    VulkanRenderSystem *p_RenderSystem = nullptr;
    VulkanUI *p_UI = nullptr;
    bool m_IsInitialized = false;

    // Delta time/Frame time
    float m_DeltaTime = 0.01f;
    // Time stamp
    std::chrono::time_point<std::chrono::high_resolution_clock> m_LastTime{};
    // Frame times for performance analysis
    std::array<float, 50> m_FrameTimes = {};
    // Minimum frame time
    float m_MinFrameTime = 1000.0f;
    // Maximum frame time
    float m_MaxFrameTime = 0.0f;
    // Frame rate recorder
    float m_FrameTime = 0.0f;
    // Frame count
    uint32_t m_FrameCount = 0;
    // FPS
    uint32_t m_FPS = 0;

    // Window extent
    uint32_t m_Width = 1600;
    uint32_t m_Height = 900;

    // Monitor handle
    GLFWmonitor *p_Monitor = nullptr;
    // Window handle
    GLFWwindow *p_Window = nullptr;

    // Queue family indices including compute, graphics, transfer and present queue family index
    QueueFamilyIndices m_QueueFamilyIndices{};
    std::vector<uint32_t> m_UniqueQueueFamilyIndices = {};
    // Queue handles including compute, graphics, transfer and present queue
    Queues m_Queues{};
    // Render command pool
    VkCommandPool m_DrawCmdPool = VK_NULL_HANDLE;
    // Render command buffers
    std::vector<VkCommandBuffer> m_DrawCmdBuffers = {};

    // Prepare flag
    bool m_Prepared = false;
    // Pause flag
    bool m_Paused = false;
    // Frame in flight flag
    bool m_BeginFrame = false;
    // Render pass in flight flag
    bool m_BeginRenderPass = false;

    // Camera
    VulkanCamera *p_Camera = nullptr;

    // Sky box
    VulkanModel *p_SkyBox = nullptr;

    /**
     * @brief (Virtual) Recreate swap chain resources
     */
    virtual void WindowResize();
    /**
     * @brief (Virtual) Load model from model file.
     * @warning Memory leek! The memory needs to be deleted manually! (delete pModel;)
     * @note This returns a pointer that memory is allocated from heap memory, needs to be deleted manually!
     */
    virtual VulkanModel *LoadModel(const std::string &modelPath, ModelTypeFlags modelType, uint32_t binding, VkVertexInputRate inputRate);
    /**
     * @brief (Virtual) Load model from buffer.
     * @warning Memory leek! The memory needs to be deleted manually! (delete pModel;)
     * @note This returns a pointer that memory is allocated from heap memory, needs to be deleted manually!
     */
    virtual VulkanModel *LoadModel(const std::vector<VulkanVertex> vertex, uint32_t binding, VkVertexInputRate inputRate, const std::vector<IndexType> index = {});
    /**
     * @brief (Virtual) Create vertex buffers.
     * @param pModels The address of the model for buffer creation.
     */
    virtual void CreateVertexBuffer(VulkanModel *pModel);
    /**
     * @brief (Virtual) Create index buffers.
     * @param pModels The address of the model for buffer creation.
     */
    virtual void CreateIndexBuffer(VulkanModel *pModel);
    /**
     * @brief (Virtual) Create uniform buffers.
     * @param bufferSize The size of buffer memory for each buffer object.
     * @note This will map and bind buffer memory, while do not copy data to memory.
     */
    virtual void CreateUniformBuffers(VkDeviceSize bufferSize, VulkanBuffer *pBuffers, size_t bufferCount);
    /**
     * @brief (Virtual) Load sky box textures.
     * @param pTexture The address of the sky box texture object.
     * @param filePathes The cube map image files' path corresponding to +X/-X/+Y/-Y/+Z/-Z.
     * @note Notice your cube UVW.
     */
    virtual void LoadSkyBoxTextures(VulkanTexture *pTextures,
                                    size_t textureCount,
                                    const std::array<const char *, 6> &filePathes,
                                    bool generateMipmap,
                                    bool flipVerticallyOnLoad);
    /**
     * @brief (Virtual) Create sky box from 6 images with the same extent.
     * @param filePathes The image files' path.
     */
    virtual void CreateSkyBox(const std::vector<VulkanVertex> vertex,
                              const std::array<const char *, 6> &filePathes,
                              bool generateMipmap,
                              bool flipVerticallyOnLoad,
                              const std::vector<IndexType> index = {});
    /**
     * @brief (Virtual) Create sky box from 6 images with the same extent.
     * @param filePathes The image files' path.
     */
    virtual void CreateSkyBox(const char *modelPath, const std::array<const char *, 6> &filePathes, bool generateMipmap, bool flipVerticallyOnLoad);
    /**
     * @brief (Virtual) Create default texture object.
     * @param pTexture The address of the texture.
     * @param textureCount The texture count.
     * @param pColor The color with srgb format with color value ranging from between 0-255 and alpha value ranging from 0-100.
     */
    virtual void CreateTextures(VulkanTexture *pTexture,
                                size_t textureCount,
                                opm::srgb *pColor);
    /**
     * @brief (Virtual) Create texture object.
     * @param pTexture The address of the texture.
     * @param textureCount The texture count.
     */
    virtual void CreateTextures(const std::string &filePath,
                                VulkanTexture *pTexture,
                                size_t textureCount,
                                bool generateMipmap,
                                bool flipVerticallyOnLoad);
    /**
     * @brief (Virtual) Create texture array with same extent.
     * @param pTexture The address of the texture.
     * @param textureCount The texture count.
     */
    virtual void CreateTextureArray(const std::vector<const char *> &filePathes,
                                    VulkanTexture *pTextures,
                                    size_t textureCount,
                                    bool generateMipmap,
                                    bool flipVerticallyOnLoad);
    /**
     * @brief (Virtual) Update uniform buffers.
     * @param data The uniform buffer data source.
     */
    virtual void UpdateUniformBuffers(VulkanBuffer *pBuffers, uint32_t bufferCount, const void *data);

    // (Virtual) Prepare UI resources
    virtual void PrepareUI(VkRenderPass renderPass,
                           uint32_t subpass,
                           VkFormat colorFormat = VK_FORMAT_R32G32B32_SFLOAT,
                           VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT,
                           VkFormat stencilFormat = VK_FORMAT_D32_SFLOAT_S8_UINT,
                           const std::string &vertFilePath = HOME_DIR "bin/VulkanUI/VulkanUI.vert.spv",
                           const std::string &fragFilePath = HOME_DIR "bin/VulkanUI/VulkanUI.frag.spv");
    // (Virtual) Draw UI
    virtual void DrawUI(VkCommandBuffer cmdBuffer, uint32_t currentFrame);
    // (Virtual) Acquire swap chain image and set graphics frame synchronization, return the vkAcquireNextImageKHR result
    virtual VkResult AcquireSwapChainImage();
    // (Virtual) Begin a draw command buffer to record commands and acquire swap chain image index
    virtual VkCommandBuffer BeginFrame();
    // (Virtual) Begin render pass, set dynamic states
    virtual void BeginRenderPass(VkCommandBuffer cmdBuffer, VkRenderPass renderPass);
    // (Virtual) Render UI and update UI data
    virtual void ShowUI();
    // (Virtual) Render UI
    virtual void RenderUI();
    // (Virtual) Update UI data
    virtual void UpdateUI();
    // (Virtual) End render pass
    virtual void EndRenderPass(VkCommandBuffer cmdBuffer);
    // (Virtual) Commit all updated submits
    virtual void CommitAllSubmits();
    // (Virtual) Present rendered image and update swap chain current frame index, return vkQueuePresentKHR result
    virtual VkResult PresentImage();
    // (Virtual) End commands recording and submit command buffer to present
    virtual void EndFrame();
};

// Example entry point
#define VULKAN_EXAMPLE_MAIN(USER_DERIVED_CLASS) \
    VulkanRenderer *pBase = nullptr;            \
    int main(int argc, const char *argv[])      \
    {                                           \
        try                                     \
        {                                       \
            pBase = new USER_DERIVED_CLASS();   \
            pBase->SetUpWindow();               \
            pBase->InitVulkan();                \
            pBase->Prepare();                   \
            pBase->MainLoop();                  \
        }                                       \
        catch (const std::exception &e)         \
        {                                       \
            if (pBase != nullptr)               \
            {                                   \
                delete pBase;                   \
                pBase = nullptr;                \
            }                                   \
            ABORT(e.what());                    \
        }                                       \
        delete pBase;                           \
        pBase = nullptr;                        \
        return 0;                               \
    }

#endif
