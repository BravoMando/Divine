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

#ifndef VULKAN_CAMERA_HEADER
#define VULKAN_CAMERA_HEADER

#pragma once

#include "VulkanCore.h"
#include "VulkanMedium.hpp"
#include "VulkanBuffer.h"
#include "opm.hpp"
#include "GLFW/glfw3.h"

class DVAPI_ATTR VulkanCamera final
{
public:
    struct Matrix
    {
        opm::mat4 ViewMat{1.0};
        opm::mat4 InverseView{1.0};
        opm::mat4 ProjectionMat{1.0};
        opm::mat4 InverseProjectionMat{1.0};
    };

private:
    opm::T m_Roll = 0.0;
    opm::T m_Yaw = 0.0;
    opm::T m_Pitch = 0.0;
    opm::T m_Fov;
    opm::vec3 m_Position = {0.0, 0.0, -1.0};
    opm::vec3 m_Front = {0.0, 0.0, 1.0};
    opm::vec3 m_GlobalUp = {0.0, -1.0, 0.0};
    // Camera uniform data
    Matrix m_CameraUniformData;

public:
    CameraTypeFlags m_Type = CAMERA_TYPE_NONE;
    opm::T m_RotationSpeed = 3.0;
    opm::T m_MoveSpeed = 1.5;
    // Only one mouse button to be dealt once
    bool m_LeftButtonPressed = false;
    // Only one mouse button to be dealt once
    bool m_MiddleButtonPressed = false;
    // Only one mouse button to be dealt once
    bool m_RightButtonPressed = false;
    // Camera uniform buffer
    std::vector<VulkanBuffer> m_CameraUniformBuffers = {};
    VkDescriptorPool m_CameraDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_CameraSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_CameraSets = {};

    // GLFW input event functions
    void (*KeyEvent)(float frameTime, int key, int scancode, int action, int mods) = nullptr;
    void (*MouseMoveEvent)(float frameTime, double xpos, double ypos) = nullptr;
    void (*MouseButtonEvent)(float frameTime, int button, int action, int mods) = nullptr;
    void (*MouseScrollEvent)(float frameTime, double xoffset, double yoffset) = nullptr;

    // Game loop input functions
    void (*KeyHandler)(float frameTime, GLFWwindow *window) = nullptr;
    void (*MouseMoveHandler)(float frameTime, GLFWwindow *window) = nullptr;
    void (*MouseButtonHandler)(float frameTime, GLFWwindow *window) = nullptr;
    void (*MouseScrollHandler)(float frameTime, GLFWwindow *window) = nullptr;

    explicit VulkanCamera();
    VulkanCamera(const VulkanCamera &) = delete;
    ~VulkanCamera();
    VulkanCamera &operator=(const VulkanCamera &) = delete;
    VulkanCamera(VulkanCamera &&) = default;
    VulkanCamera &operator=(VulkanCamera &&) = default;

public:
    inline opm::vec3 GetPosition() { return m_Position; }
    inline opm::vec3 GetFront() { return m_Front; }
    inline opm::vec3 GetUp() { return m_GlobalUp; }

    inline void SetPosition(const opm::vec3 &pos) { m_Position = pos; }
    inline void SetDirection(const opm::vec3 &dir)
    {
        m_Front = dir;
        m_Pitch = opm::asin(m_Front.y) * 180.0 / opm::MATH_PI;
        m_Yaw = opm::asin(m_Front.x / opm::cos(opm::radians(m_Pitch))) * 180.0 / opm::MATH_PI;
    }
    inline void SetUp(const opm::vec3 &globalUp) { m_GlobalUp = globalUp; }
    inline void SetRoll(const opm::T roll_degree) { m_Roll = roll_degree; }
    inline void SetYaw(const opm::T yaw_degree) { m_Yaw = yaw_degree; }
    inline void SetPitch(const opm::T pitch_degree) { m_Pitch = pitch_degree; }

    inline opm::vec3 GetPosition() const { return m_Position; }
    inline opm::vec3 GetDirection() const { return m_Front; }
    inline opm::vec3 GetUp() const { return m_GlobalUp; }
    inline opm::T GetFov() const { return m_Fov; }

    // Handle key state, this will call the GLFW input event function if it's not nullptr
    void OnKeyState(float frameTime, int key, int scancode, int action, int mods);
    // Handle mouse move, this will call the GLFW input event function if it's not nullptr
    void OnMouseMove(float frameTime, double xpos, double ypos);
    // Handle mouse button state, this will call the GLFW input event function if it's not nullptr
    void OnMouseButtonState(float frameTime, int button, int action, int mods);
    // Handle mouse scroll, this will call the GLFW input event function if it's not nullptr
    void OnMouseScroll(float frameTime, double xoffset, double yoffset);

    //====================================================================================//
    //============ Mouse button event state will be handled by polling events ============//
    //============ Mouse scroll event state will be handled by polling events ============//
    //====================================================================================//

    // Handle key state in game loop instead of polling events
    void HandleKeyState(float frameTime, GLFWwindow *window);
    // Handle mouse move in game loop instead of polling events
    void HandleMouseMove(float frameTime, GLFWwindow *window);
    // Handle mouse button state in game loop instead of polling events
    void HandleMouseButtonState(float frameTime, GLFWwindow *window);
    // Handle mouse scroll in game loop instead of polling events
    void HandleMouseScroll(float frameTime, GLFWwindow *window);

    // Update camera view matrix and inverse view matrix
    void UpdateViewMat();
    // Update camera orthographic projection matrix and its inverse matrix
    void UpdateOrthographicMat(opm::T left, opm::T right, opm::T bottom, opm::T top, opm::T _near, opm::T _far);
    // Update camera perspective projection matrix and its inverse matrix
    void UpdatePerspectiveMat(opm::T fovy, opm::T aspect, opm::T _near, opm::T _far);

    // Uniform data
    inline const Matrix &GetUniformData() { return m_CameraUniformData; }
    // View matrix (model to world)
    inline opm::mat4 GetViewMat(bool transpose = true) { return transpose ? m_CameraUniformData.ViewMat.Transpose() : m_CameraUniformData.ViewMat; }
    // Inverse view matrix (world to model)
    inline opm::mat4 GetInverseViewMat(bool transpose = true) { return transpose ? m_CameraUniformData.InverseView.Transpose() : m_CameraUniformData.InverseView; }
    // Projection matrix
    inline opm::mat4 GetProjectionMat(bool transpose = true) { return transpose ? m_CameraUniformData.ProjectionMat.Transpose() : m_CameraUniformData.ProjectionMat; }
    // Inverse projection matrix
    inline opm::mat4 GetInverseProjectionMat(bool transpose = true) { return transpose ? m_CameraUniformData.InverseProjectionMat.Transpose() : m_CameraUniformData.InverseProjectionMat; }
};

#endif
