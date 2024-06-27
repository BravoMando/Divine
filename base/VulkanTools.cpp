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

#include "VulkanTools.h"

const char *_ErrorToString_(VkResult code)
{
    switch (code)
    {

#define _STR_(r) \
    case VK_##r: \
        return #r

        _STR_(NOT_READY);
        _STR_(TIMEOUT);
        _STR_(EVENT_SET);
        _STR_(EVENT_RESET);
        _STR_(INCOMPLETE);
        _STR_(ERROR_OUT_OF_HOST_MEMORY);
        _STR_(ERROR_OUT_OF_DEVICE_MEMORY);
        _STR_(ERROR_INITIALIZATION_FAILED);
        _STR_(ERROR_DEVICE_LOST);
        _STR_(ERROR_MEMORY_MAP_FAILED);
        _STR_(ERROR_LAYER_NOT_PRESENT);
        _STR_(ERROR_EXTENSION_NOT_PRESENT);
        _STR_(ERROR_FEATURE_NOT_PRESENT);
        _STR_(ERROR_INCOMPATIBLE_DRIVER);
        _STR_(ERROR_TOO_MANY_OBJECTS);
        _STR_(ERROR_FORMAT_NOT_SUPPORTED);
        _STR_(ERROR_SURFACE_LOST_KHR);
        _STR_(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        _STR_(SUBOPTIMAL_KHR);
        _STR_(ERROR_OUT_OF_DATE_KHR);
        _STR_(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        _STR_(ERROR_VALIDATION_FAILED_EXT);
        _STR_(ERROR_INVALID_SHADER_NV);
        _STR_(ERROR_INCOMPATIBLE_SHADER_BINARY_EXT);
#undef _STR_
    default:
        return "UNKNOWN_ERROR";
    }
}

bool HasStencilFormat(VkFormat format)
{
    return format >= VK_FORMAT_S8_UINT && format <= VK_FORMAT_D32_SFLOAT_S8_UINT;
}

uint32_t FindMemoryTypeIndex(VkPhysicalDevice gpu, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties GPUMemProps{};
    vkGetPhysicalDeviceMemoryProperties(gpu, &GPUMemProps);
    for (uint32_t i = 0; i < GPUMemProps.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (GPUMemProps.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    FATAL("No matching memory type index found!");
}

void TransitionImageLayout(VkCommandBuffer cmdbuffer,
                           VkImage image,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout,
                           VkImageSubresourceRange subresourceRange,
                           VkPipelineStageFlags srcStageMask,
                           VkPipelineStageFlags dstStageMask)
{
    // Create an image barrier object
    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    switch (oldLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
    {
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE;
    }
    break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
    {
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    }
    break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    {
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    {
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    {
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }
    break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    {
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    {
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    break;

    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    {
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    {
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }
    break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    {
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    {
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    {
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (imageMemoryBarrier.srcAccessMask == VK_ACCESS_NONE)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    break;

    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Put barrier inside setup command buffer
    vkCmdPipelineBarrier(cmdbuffer,
                         srcStageMask,
                         dstStageMask,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &imageMemoryBarrier);
}
