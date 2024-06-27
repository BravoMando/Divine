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

#ifndef VULKAN_TOOLS_HEADER
#define VULKAN_TOOLS_HEADER

#pragma once

#ifdef _MSC_VER
#pragma warning(disable : 4715) // return value
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wreturn-type"
#pragma clang diagnostic ignored "-Wformat-security"
#pragma clang diagnostic ignored "-Wdefaulted-function-deleted"
#endif

#include "VulkanCore.h"
#include "VulkanConfig.h"
#include "VulkanLogger.h"
#include "vulkan/vulkan.h"

/**
 * @brief The size of array.
 * @warning C-Style static array ONLY! Can NOT be used with pointers!
 */
#define STATIC_ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

// Convert the VkResult code into C-Style string.
DVAPI_ATTR const char *DVAPI_CALL _ErrorToString_(VkResult code);

// Check if VkResult is equal to VK_SUCCESS
#define CHECK_VK_RESULT(f)                                                      \
    do                                                                          \
    {                                                                           \
        VkResult res = (f);                                                     \
        if (res != VK_SUCCESS)                                                  \
        {                                                                       \
            FATAL("Vulkan Error: %s\n\tfile: %s\n\tfunction: %s\n\tline: %d\n", \
                  _ErrorToString_(res), __FILE__, #f, __LINE__);                \
        }                                                                       \
    } while (0)

// VK_INDEX_TYPE_UINT16 = 0,
// VK_INDEX_TYPE_UINT32 = 1,
// VK_INDEX_TYPE_NONE_KHR = 1000165000,
// VK_INDEX_TYPE_UINT8_EXT = 1000265000,
// VK_INDEX_TYPE_NONE_NV = VK_INDEX_TYPE_NONE_KHR,
// VK_INDEX_TYPE_MAX_ENUM = 0x7FFFFFFF
/**
 * @brief The index buffer type, must be unsigned integer, macro to record the type
 */
#if defined(INDEX_TYPE_UINT32)
#include <cstdint>
typedef uint32_t IndexType;
#define INDEX_TYPE_FLAG VK_INDEX_TYPE_UINT32
#elif defined(INDEX_TYPE_UINT16)
#include <cstdint>
typedef uint16_t IndexType;
#define INDEX_TYPE_FLAG VK_INDEX_TYPE_UINT16
#elif defined(INDEX_TYPE_UINT8)
#include <cstdint>
typedef uint8_t IndexType;
#define INDEX_TYPE_FLAG VK_INDEX_TYPE_UINT8_EXT
#endif

// Helper functions

// Check if format support stencil
DVAPI_ATTR bool DVAPI_CALL HasStencilFormat(VkFormat format);
// Find memory type index
DVAPI_ATTR uint32_t DVAPI_CALL FindMemoryTypeIndex(VkPhysicalDevice gpu, uint32_t typeFilter, VkMemoryPropertyFlags properties);
// Transition image layout
DVAPI_ATTR void DVAPI_CALL TransitionImageLayout(VkCommandBuffer cmdbuffer,
                                                 VkImage image,
                                                 VkImageLayout oldLayout,
                                                 VkImageLayout newLayout,
                                                 VkImageSubresourceRange subresourceRange,
                                                 VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                 VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

#endif
