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

#include "VulkanCamera.h"
#include "VulkanTools.h"

VulkanCamera::VulkanCamera()
{
}

VulkanCamera::~VulkanCamera()
{
    for (size_t i = 0; i < m_CameraUniformBuffers.size(); ++i)
    {
        m_CameraUniformBuffers[i].Destroy();
    }
}

void VulkanCamera::OnKeyState(float frameTime, int key, int scancode, int action, int mods)
{
    if (KeyEvent != nullptr)
    {
        return KeyEvent(frameTime, key, scancode, action, mods);
    }
}

void VulkanCamera::OnMouseMove(float frameTime, double xpos, double ypos)
{
    if (MouseMoveEvent != nullptr)
    {
        return MouseMoveEvent(frameTime, xpos, ypos);
    }
}

void VulkanCamera::OnMouseButtonState(float frameTime, int button, int action, int mods)
{
    if (MouseButtonEvent != nullptr)
    {
        return MouseButtonEvent(frameTime, button, action, mods);
    }

    if (m_Type == CAMERA_TYPE_LOOK_AT)
    {
        // Release
        if (action == GLFW_RELEASE)
        {
            switch (button)
            {
            case GLFW_MOUSE_BUTTON_RIGHT:
            {
                m_RightButtonPressed = false;
            }
            break;

            case GLFW_MOUSE_BUTTON_MIDDLE:
            {
                m_MiddleButtonPressed = false;
            }
            break;

            case GLFW_MOUSE_BUTTON_LEFT:
            {
                m_LeftButtonPressed = false;
            }
            break;

            default:
                break;
            }
        }
        // Press
        else
        {
            switch (button)
            {
            case GLFW_MOUSE_BUTTON_RIGHT:
            {
                m_RightButtonPressed = true;
            }
            break;

            case GLFW_MOUSE_BUTTON_MIDDLE:
            {
                m_MiddleButtonPressed = true;
            }
            break;

            case GLFW_MOUSE_BUTTON_LEFT:
            {
                m_LeftButtonPressed = true;
            }
            break;

            default:
                break;
            }
        }
    }
    else if (m_Type == CAMERA_TYPE_FIRST_PERSON)
    {
    }
}

void VulkanCamera::OnMouseScroll(float frameTime, double xoffset, double yoffset)
{
    if (MouseScrollEvent != nullptr)
    {
        return MouseScrollEvent(frameTime, xoffset, yoffset);
    }

    m_Position = m_Position + m_Front * yoffset * frameTime * m_MoveSpeed;
}

void VulkanCamera::HandleKeyState(float frameTime, GLFWwindow *window)
{
    if (KeyHandler != nullptr)
    {
        return KeyHandler(frameTime, window);
    }

    if (m_Type == CAMERA_TYPE_LOOK_AT)
    {
        if (m_LeftButtonPressed || m_MiddleButtonPressed || m_RightButtonPressed)
        {
            opm::vec3 dir{};
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            {
                dir = dir + m_Front;
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            {
                dir = dir - m_Front;
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            {
                dir = dir - m_Front.cross(m_GlobalUp);
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            {
                dir = dir + m_Front.cross(m_GlobalUp);
            }
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            {
                dir = dir + m_GlobalUp;
            }
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            {
                dir = dir - m_GlobalUp;
            }

            if (dir * dir > opm::epsilon<opm::T>())
            {
                m_Position = m_Position + opm::normalize(dir) * m_MoveSpeed * frameTime;
            }
        }
    }
    else if (m_Type == CAMERA_TYPE_FIRST_PERSON)
    {
        opm::vec3 dir{};
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            dir = dir + m_Front;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            dir = dir - m_Front;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            dir = dir - m_Front.cross(m_GlobalUp);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            dir = dir + m_Front.cross(m_GlobalUp);
        }

        if (dir * dir > opm::epsilon<opm::T>())
        {
            m_Position = m_Position + opm::normalize(dir) * m_MoveSpeed * frameTime;
        }
    }
}

void VulkanCamera::HandleMouseMove(float frameTime, GLFWwindow *window)
{
    if (MouseMoveHandler != nullptr)
    {
        return MouseMoveHandler(frameTime, window);
    }

    static bool firstTime = true;
    static double lastX, lastY;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    if (firstTime)
    {
        lastX = x;
        lastY = y;
        firstTime = false;
    }

    double dx = x - lastX;
    double dy = y - lastY;

    lastX = x;
    lastY = y;

    if (m_Type == CAMERA_TYPE_LOOK_AT)
    {
        if (m_LeftButtonPressed || m_MiddleButtonPressed || m_RightButtonPressed)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        if (m_LeftButtonPressed)
        {
        }
        else if (m_MiddleButtonPressed)
        {
            opm::vec3 dir{};
            dir = dir + m_Front.cross(m_GlobalUp) * dx;
            dir = dir + m_GlobalUp * dy;
            if (dir * dir > opm::epsilon<opm::T>())
            {
                m_Position = m_Position - opm::normalize(dir) * m_MoveSpeed * frameTime;
            }
        }
        else if (m_RightButtonPressed)
        {
            m_Pitch += dy * m_RotationSpeed * frameTime;
            m_Yaw += dx * m_RotationSpeed * frameTime;
            if (m_Pitch > 89.0)
            {
                m_Pitch = 89.0;
            }
            if (m_Pitch < -89.0)
            {
                m_Pitch = -89.0;
            }

            m_Front.x = opm::cos(opm::radians(m_Pitch)) * opm::sin(opm::radians(m_Yaw));
            m_Front.y = -opm::sin(opm::radians(m_Pitch));
            m_Front.z = opm::cos(opm::radians(m_Pitch)) * opm::cos(opm::radians(m_Yaw));
        }
    }
    else if (m_Type == CAMERA_TYPE_FIRST_PERSON)
    {
        m_Pitch += dy * m_RotationSpeed * frameTime;
        m_Yaw += dx * m_RotationSpeed * frameTime;
        if (m_Pitch > 89.0)
        {
            m_Pitch = 89.0;
        }
        if (m_Pitch < -89.0)
        {
            m_Pitch = -89.0;
        }

        m_Front.x = opm::cos(opm::radians(m_Pitch)) * opm::sin(opm::radians(m_Yaw));
        m_Front.y = opm::sin(opm::radians(m_Pitch));
        m_Front.z = opm::cos(opm::radians(m_Pitch)) * opm::cos(opm::radians(m_Yaw));
    }
}

void VulkanCamera::HandleMouseButtonState(float frameTime, GLFWwindow *window)
{
    if (MouseButtonHandler != nullptr)
    {
        return MouseButtonHandler(frameTime, window);
    }
}

void VulkanCamera::HandleMouseScroll(float frameTime, GLFWwindow *window)
{
    if (MouseScrollHandler != nullptr)
    {
        return MouseScrollHandler(frameTime, window);
    }
}

// Update camera view matrix and inverse view matrix
void VulkanCamera::UpdateViewMat()
{
    m_CameraUniformData.ViewMat = opm::LookAt(m_Position, m_Position + m_Front, m_GlobalUp).Transpose();
    m_CameraUniformData.InverseView = opm::InverseLookAt(m_Position, m_Position + m_Front, m_GlobalUp).Transpose();
}
// Update camera orthographic projection matrix and inverse matrix
void VulkanCamera::UpdateOrthographicMat(opm::T left, opm::T right, opm::T bottom, opm::T top, opm::T _near, opm::T _far)
{
    m_CameraUniformData.ProjectionMat = opm::OrthographicProjection(left, right, bottom, top, _near, _far).Transpose();
    m_CameraUniformData.InverseProjectionMat = opm::InverseOrthographic(left, right, bottom, top, _near, _far).Transpose();
}
// Update camera perspective projection matrix and inverse matrix
void VulkanCamera::UpdatePerspectiveMat(opm::T fovy, opm::T aspect, opm::T _near, opm::T _far)
{
    m_Fov = fovy;
    m_CameraUniformData.ProjectionMat = opm::PerspectiveProjection(fovy, aspect, _near, _far).Transpose();
    m_CameraUniformData.InverseProjectionMat = opm::InversePerspective(fovy, aspect, _near, _far).Transpose();
}
