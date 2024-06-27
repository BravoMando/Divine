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
#include "VulkanModel.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "VulkanInitializer.hpp"

#include <unordered_map>

std::unordered_set<uint32_t> VulkanModel::s_UniqueBinding = {};
std::unordered_set<uint32_t> VulkanModel::s_UniqueLocation = {};
std::vector<VkVertexInputBindingDescription> VulkanModel::s_BindingDescriptions = {};
std::vector<VkVertexInputAttributeDescription> VulkanModel::s_AttributeDescriptions = {};

// hash combine
template <typename T, typename... Rest>
void HashCombine(std::size_t &seed, const T &v, const Rest &...rest)
{
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (HashCombine(seed, rest), ...);
}
// hash functor
namespace std
{
    template <>
    struct hash<VulkanVertex>
    {
        size_t operator()(const VulkanVertex &vertex) const
        {
            size_t seed = 0;
            HashCombine(seed, vertex.Position, vertex.Color, vertex.Normal, vertex.UV);

            return seed;
        }
    };
}

VulkanModel::VulkanModel(const std::string &modelPath, ModelTypeFlags modelType, uint32_t binding, VkVertexInputRate inputRate, VkDevice device, const VkAllocationCallbacks *pAllocator)
    : p_Allocator{pAllocator}, m_VertexBuffer{pAllocator}, m_IndexBuffer{pAllocator}
{
    if (device == VK_NULL_HANDLE)
    {
        FATAL("Device must be valid!");
    }
    m_Device = device;
    p_Allocator = pAllocator;

    switch (modelType)
    {
    case MODEL_TYPE_NONE:
    {
        FATAL("Model type must not be MODEL_TYPE_NONE!");
    }
    break;

    case MODEL_TYPE_OBJ:
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str()))
        {
            FATAL("Loading model failed!\n\twarning: %s\n\terror: %s", warn.c_str(), err.c_str());
        }

        m_Vertices.clear();
        m_Indices.clear();

        std::unordered_map<VulkanVertex, IndexType> uniqueVertices{};

        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                VulkanVertex vertex{};

                if (index.vertex_index >= 0)
                {
                    vertex.Position = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]};

                    vertex.Color = {
                        attrib.colors[3 * index.vertex_index + 0],
                        attrib.colors[3 * index.vertex_index + 1],
                        attrib.colors[3 * index.vertex_index + 2]};
                }

                if (index.normal_index >= 0)
                {
                    vertex.Normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]};
                }

                if (index.texcoord_index >= 0)
                {
                    vertex.UV = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1]};
                }

                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
                    m_Vertices.push_back(vertex);
                }
                m_Indices.push_back(uniqueVertices[vertex]);
            }
        }
        m_IndexCount = m_Indices.size();
        m_VertexCount = m_Vertices.size();
        m_HasIndexBuffer = m_IndexCount > 0;
        m_Type = MODEL_TYPE_OBJ;
    }
    break;

    case MODEL_TYPE_GLTF:
    {
        // TODO: Add gltf support
        m_Type = MODEL_TYPE_GLTF;
    }
    break;

    default:
    {
        FATAL("Invalid model type flags!");
    }
    break;
    }

    INFO("vertex count: %d, index count: %d\n", m_VertexCount, m_IndexCount);

    VulkanModel::AddVertexInputBinding(binding, sizeof(VulkanVertex), inputRate);
    VulkanModel::AddVertexInputAttribute(0, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanVertex, Position));
    VulkanModel::AddVertexInputAttribute(1, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanVertex, Color));
    VulkanModel::AddVertexInputAttribute(2, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanVertex, Normal));
    VulkanModel::AddVertexInputAttribute(3, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(VulkanVertex, UV));
}

VulkanModel::VulkanModel(const std::vector<VulkanVertex> vertex, uint32_t binding, VkVertexInputRate inputRate, VkDevice device, const VkAllocationCallbacks *pAllocator, const std::vector<IndexType> index)
    : p_Allocator{pAllocator}, m_VertexBuffer{pAllocator}, m_IndexBuffer{pAllocator}
{
    if (device == VK_NULL_HANDLE)
    {
        FATAL("Device must be valid!");
    }
    m_Device = device;
    p_Allocator = pAllocator;

    m_Vertices = vertex;
    m_VertexCount = vertex.size();
    m_Indices = index;
    m_IndexCount = index.size();
    m_HasIndexBuffer = m_IndexCount > 0;
    m_Type = MODEL_TYPE_NONE;

    INFO("vertex count: %d, index count: %d\n", m_VertexCount, m_IndexCount);

    VulkanModel::AddVertexInputBinding(binding, sizeof(VulkanVertex), inputRate);
    VulkanModel::AddVertexInputAttribute(0, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanVertex, Position));
    VulkanModel::AddVertexInputAttribute(1, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanVertex, Color));
    VulkanModel::AddVertexInputAttribute(2, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanVertex, Normal));
    VulkanModel::AddVertexInputAttribute(3, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(VulkanVertex, UV));
}

VulkanModel::~VulkanModel()
{
    FreeBufferMemory();
    DestroyTextures();
}

void VulkanModel::AddVertexInputBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
{
    if (VulkanModel::s_UniqueBinding.count(binding) == 0)
    {
        VulkanModel::s_UniqueBinding.insert(binding);
        VulkanModel::s_BindingDescriptions.push_back({binding, stride, inputRate});
    }
}

void VulkanModel::AddVertexInputAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset)
{
    if (VulkanModel::s_UniqueLocation.count(location) == 0)
    {
        VulkanModel::s_UniqueLocation.insert(location);
        VulkanModel::s_AttributeDescriptions.push_back({location, binding, format, offset});
    }
}

const std::vector<VkVertexInputBindingDescription> &VulkanModel::GetBindingDescription()
{
    return VulkanModel::s_BindingDescriptions;
}

const std::vector<VkVertexInputAttributeDescription> &VulkanModel::GetAttributeDescription()
{
    return VulkanModel::s_AttributeDescriptions;
}

void VulkanModel::Transform(const opm::vec3 &scale, const opm::vec3 &rotation, const opm::vec3 &translation)
{
    m_UniqueModelMat = opm::Transform(m_UniqueModelMat, scale, rotation, translation);
}

void VulkanModel::Rotate(const opm::vec3 &axis, opm::T radians)
{
    m_UniqueModelMat = opm::Rotate(m_UniqueModelMat, radians, axis);
}

void VulkanModel::Scale(const opm::vec3 &scale)
{
    m_UniqueModelMat = opm::Scale(m_UniqueModelMat, scale);
}

void VulkanModel::Translate(const opm::vec3 &offset)
{
    m_UniqueModelMat = opm::Translate(m_UniqueModelMat, offset);
}

void VulkanModel::Bind(VkCommandBuffer cmdBuffer)
{
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_VertexBuffer.Buffer, &offset);
    if (m_HasIndexBuffer)
    {
        vkCmdBindIndexBuffer(cmdBuffer, m_IndexBuffer.Buffer, 0, INDEX_TYPE_FLAG);
    }
}

void VulkanModel::Draw(VkCommandBuffer cmdBuffer)
{
    if (m_HasIndexBuffer)
    {
        vkCmdDrawIndexed(cmdBuffer, m_IndexCount, 1, 0, 0, 0);
    }
    else
    {
        vkCmdDraw(cmdBuffer, m_VertexCount, 1, 0, 0);
    }
}

void VulkanModel::FreeBufferMemory()
{
    for (size_t i = 0; i < m_TransformBuffers.size(); ++i)
    {
        if (m_TransformBuffers[i].Device != VK_NULL_HANDLE)
        {
            m_TransformBuffers[i].Destroy();
        }
    }
    m_IndexBuffer.Destroy();
    m_VertexBuffer.Destroy();
}

void VulkanModel::DestroyTextures()
{
    for (size_t i = 0; i < m_ColorTextures.size(); ++i)
    {
        if (m_ColorTextures[i].Device != VK_NULL_HANDLE)
        {
            m_ColorTextures[i].Destroy();
        }
    }
}
