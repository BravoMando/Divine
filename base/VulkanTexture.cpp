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

#include "VulkanTexture.h"
#include "VulkanTools.h"

void VulkanTexture::SetDescriptorImage()
{
    DescriptorImageInfo.imageLayout = Layout;
    DescriptorImageInfo.imageView = View;
    DescriptorImageInfo.sampler = Sampler;
}

void VulkanTexture::Destroy()
{
    if (IsInitialized == false)
    {
        return;
    }
    if (Device == VK_NULL_HANDLE)
    {
        FATAL("No valid device for texture operation!");
    }

    if (Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(Device, Sampler, Allocator);
    }
    if (View != VK_NULL_HANDLE)
    {
        vkDestroyImageView(Device, View, Allocator);
    }
    if (Memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(Device, Memory, Allocator);
    }
    if (Image != VK_NULL_HANDLE)
    {
        vkDestroyImage(Device, Image, Allocator);
    }
}
