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

#ifndef VULKAN_MODEL_HEADER
#define VULKAN_MODEL_HEADER

#pragma once

#include "VulkanCore.h"
#include "VulkanConfig.h"
#define OPM_ENABLE_HASH
#include "opm.hpp"
#include "VulkanMedium.hpp"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"

#include <string>
#include <vector>
#include <unordered_set>
#include <utility>

struct DVAPI_ATTR VulkanVertex
{
    opm::vec3 Position{};
    opm::vec3 Color{};
    opm::vec3 Normal{};
    opm::vec2 UV{};

    inline bool operator==(const VulkanVertex &other) const
    {
        return Position == other.Position && Color == other.Color && Normal == other.Normal && UV == other.UV;
    }
};

class DVAPI_ATTR VulkanModel final
{
private:
    ModelTypeFlags m_Type = MODEL_TYPE_NONE;
    std::vector<VulkanVertex> m_Vertices = {};
    size_t m_VertexCount = 0;
    std::vector<IndexType> m_Indices = {};
    size_t m_IndexCount = 0;
    bool m_HasIndexBuffer = false;
    opm::vec3 m_Rotation{0.0};
    opm::vec3 m_Scale{1.0};
    opm::vec3 m_Translation{0.0};

    static std::unordered_set<uint32_t> s_UniqueBinding;
    static std::unordered_set<uint32_t> s_UniqueLocation;
    static std::vector<VkVertexInputBindingDescription> s_BindingDescriptions;
    static std::vector<VkVertexInputAttributeDescription> s_AttributeDescriptions;

    static void AddVertexInputBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate);
    static void AddVertexInputAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);

public:
    VkDevice m_Device = VK_NULL_HANDLE;
    const VkAllocationCallbacks *p_Allocator = nullptr;
    opm::mat4 m_UniqueModelMat{1.0};
    VulkanBuffer m_VertexBuffer{};
    VulkanBuffer m_IndexBuffer{};

    std::vector<VulkanBuffer> m_TransformBuffers = {};
    std::vector<VulkanTexture> m_ColorTextures = {};
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts = {};
    std::vector<VkDescriptorSet> m_TransformSets = {};
    std::vector<VkDescriptorSet> m_TextureSets = {};

public:
    explicit VulkanModel(const std::string &modelPath,
                         ModelTypeFlags modelType,
                         uint32_t binding,
                         VkVertexInputRate inputRate,
                         VkDevice device,
                         const VkAllocationCallbacks *pAllocator);
    explicit VulkanModel(const std::vector<VulkanVertex> vertex,
                         uint32_t binding,
                         VkVertexInputRate inputRate,
                         VkDevice device,
                         const VkAllocationCallbacks *pAllocator,
                         const std::vector<IndexType> index = {});
    ~VulkanModel();
    // Not copyable but moveable
    VulkanModel(const VulkanModel &) = delete;
    VulkanModel &operator=(const VulkanModel &) = delete;
    VulkanModel(VulkanModel &&) = default;
    VulkanModel &operator=(VulkanModel &&) = default;

    // This gives more efficiency compared with Rotation
    void Transform(const opm::vec3 &scale, const opm::vec3 &rotation, const opm::vec3 &translation);
    // Rotate angles based on given axis
    void Rotate(const opm::vec3 &axis, opm::T radians);
    // Scale the model size
    void Scale(const opm::vec3 &scale);
    // Translate model position
    void Translate(const opm::vec3 &offset);

    void Bind(VkCommandBuffer cmdBuffer);
    void Draw(VkCommandBuffer cmdBuffer);

    static const std::vector<VkVertexInputBindingDescription> &GetBindingDescription();
    static const std::vector<VkVertexInputAttributeDescription> &GetAttributeDescription();

    inline const size_t GetVertexCount() const { return m_VertexCount; }
    inline size_t GetVertexCount() { return m_VertexCount; }
    inline const size_t GetIndexCount() const { return m_IndexCount; }
    inline size_t GetIndexCount() { return m_IndexCount; }
    inline const VulkanVertex *GetVertexData() const { return m_Vertices.data(); }
    inline const IndexType *GetIndexData() const { return m_Indices.data(); }
    inline void ClearVertexData() { m_Vertices.clear(); }
    inline void ClearIndexData() { m_Indices.clear(); }
    void FreeBufferMemory();
    void DestroyTextures();
};

#endif
