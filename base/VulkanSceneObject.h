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

#ifndef VULKAN_SCENE_OBJECT_HEADER
#define VULKAN_SCENE_OBJECT_HEADER

#pragma once

#include "VulkanCore.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "VulkanDevice.h"
#include "opm.hpp"

#include <vector>

struct DVAPI_ATTR CameraProperties
{
    opm::vec3 Position;
    float fov;
    opm::vec3 Up;
    float _pad0;
    opm::vec3 Front;
    float _pad1;
};

struct DVAPI_ATTR SceneDefinition
{
    uint32_t PointLightCount;
    uint32_t DirectLightCount;
    uint32_t SphereCount;
    uint32_t PlaneCount;

    uint32_t BoxCount;
    uint32_t TorusCount;
    uint32_t RingCount;
    uint32_t SurfaceCount;
};

struct DVAPI_ATTR SceneProperties
{
    uint32_t CanvasWidth;
    uint32_t CanvasHeight;
    uint32_t ReflectDepth = 2;
    float GlobalRefract = 1.0;
    opm::vec3 AmbientColor;
    uint32_t RefractDepth = 2;
    CameraProperties Camera;
    opm::mat4 ViewMat{1.0};
    opm::mat4 InverseViewMat{1.0};
    opm::mat4 ProjectionMat{1.0};
    opm::mat4 InverseProjectionMat{1.0};
    SceneDefinition Definitions;
};

struct DVAPI_ATTR PointLight
{
    opm::vec4 PositionAndRadius;

    opm::vec3 Color;
    float Intensity;

    int ID;
    float LinearK;
    float QuadraticK;
    float _pad0;

    opm::quat Rotation;
};

struct DVAPI_ATTR DirectLight
{
    int ID;
    opm::vec3 Direction;
    opm::vec3 Color;
    float Intensity;
    opm::quat Rotation;
};

struct DVAPI_ATTR Material
{
    int ColorTexture;
    int NormalTexture;
    float Reflect;
    float Refract;

    opm::vec3 Color;
    float Specular;
};

struct DVAPI_ATTR Sphere
{
    int ID;
    bool Hollow;
    opm::vec2 _pad0;

    opm::vec4 CenterAndRadius;
    Material Mat;
    opm::quat Rotation;
};

struct DVAPI_ATTR Plane
{
    int ID;
    opm::vec3 Position;

    opm::vec3 Normal;
    float _pad0;

    Material Mat;
    opm::quat Rotation;
};

struct DVAPI_ATTR Box
{
    int ID;
    opm::vec3 Position;

    opm::vec3 Form; // long, width, height
    float _pad0;

    Material Mat;
    opm::quat Rotation;
};

struct DVAPI_ATTR Torus
{
    int ID;
    opm::vec3 Position;

    opm::vec2 Form; // r1, r2
    opm::vec2 _pad0;

    Material Mat;
    opm::quat Rotation;
};

struct DVAPI_ATTR Ring
{
    int ID;
    opm::vec3 Position;

    opm::vec2 Form; // r1 * r1, r2 * r2
    opm::vec2 _pad0;

    Material Mat;
    opm::quat Rotation;
};

struct DVAPI_ATTR Surface
{
    int ID;
    opm::vec3 Min = {-opm::MATH_FLT_INFINITY};

    opm::vec3 Max = {opm::MATH_FLT_INFINITY};
    float _pad0;

    opm::vec3 Position;
    float a; // x2 coefficient

    float b; // y2 coefficient
    float c; // z2 coefficient
    float d; // z coefficient
    float e; // y coefficient

    float f; // constance
    opm::vec3 _pad1;

    Material Mat;
    opm::quat Rotation;
};

class DVAPI_ATTR VulkanScene final
{
private:
    bool m_IsConnected = false;
    VulkanDevice *p_Device = nullptr;

public:
    SceneProperties SceneProperty;
    std::vector<VulkanBuffer> SceneBuffers; // Scene properties and scene definitions

    std::vector<PointLight> PointLights;
    std::vector<VulkanBuffer> PointLightsBuffer; // Frames in flight
    std::vector<DirectLight> DirectLights;
    std::vector<VulkanBuffer> DirectLightsBuffer; // Frames in flight

    std::vector<Sphere> Spheres;
    std::vector<VulkanBuffer> SpheresBuffer;  // Frames in flight
    std::vector<VulkanTexture> SphereColors;  // Frames in flight
    std::vector<VulkanTexture> SphereNormals; // Frames in flight

    std::vector<Plane> Planes;
    std::vector<VulkanBuffer> PlanesBuffer; // Frames in flight

    std::vector<Box> Boxes;
    std::vector<VulkanBuffer> BoxesBuffer;   // Frames in flight
    std::vector<VulkanTexture> BoxesColors;  // Frames in flight
    std::vector<VulkanTexture> BoxesNormals; // Frames in flight

    std::vector<Torus> Toruses;
    std::vector<VulkanBuffer> TorusesBuffer; // Frames in flight
    std::vector<VulkanTexture> TorusColors;  // Frames in flight
    std::vector<VulkanTexture> TorusNormals; // Frames in flight

    std::vector<Ring> Rings;
    std::vector<VulkanBuffer> RingsBuffer;  // Frames in flight
    std::vector<VulkanTexture> RingColors;  // Frames in flight
    std::vector<VulkanTexture> RingNormals; // Frames in flight

    std::vector<Surface> Surfaces;
    std::vector<VulkanBuffer> SurfacesBuffer;  // Frames in flight
    std::vector<VulkanTexture> SurfaceColors;  // Frames in flight
    std::vector<VulkanTexture> SurfaceNormals; // Frames in flight

public:
    VulkanScene();
    ~VulkanScene();
    VulkanScene(const VulkanScene &) = delete;
    VulkanScene &operator=(const VulkanScene &) = delete;
    VulkanScene(VulkanScene &&) = delete;
    VulkanScene &operator=(VulkanScene &&) = delete;

    void Connect(VulkanDevice *pDevice);
    void ResizeAllBuffers(size_t size);
    void ResizeAllTextures(size_t size);
    void DestroyAllBuffers();
    void DestroyAllTextures();

    VulkanScene &AddPointLight(const opm::vec4 &positionAndRadius,
                               const opm::vec3 &color,
                               float intensity,
                               float linearK = 0.22,
                               float quadraticK = 0.2,
                               const opm::quat &rotate = opm::quat{0.0, 0.0, 0.0, 1.0});
    void BuildPointLightBuffer();
    VulkanScene &AddDirectLight(const opm::vec3 &dir,
                                const opm::vec3 &color,
                                float intensity,
                                const opm::quat &rotate = opm::quat{0.0, 0.0, 0.0, 1.0});
    void BuildDirectLightBuffer();
    VulkanScene &AddSphere(const opm::vec4 &centerAndRadius,
                           const Material &mat,
                           bool hollow = false,
                           const opm::quat &rotate = opm::quat{0.0, 0.0, 0.0, 1.0});
    void BuildSphereBuffer();
    VulkanScene &AddPlane(const opm::vec3 &normal,
                          const opm::vec3 &position,
                          const Material &mat,
                          const opm::quat &rotate = opm::quat{0.0, 0.0, 0.0, 1.0});
    void BuildPlaneBuffer();
    VulkanScene &AddBox(const opm::vec3 &position,
                        const opm::vec3 &form,
                        const Material &mat,
                        const opm::quat &rotate = opm::quat{0.0, 0.0, 0.0, 1.0});
    void BuildBoxBuffer();
    VulkanScene &AddTorus(const opm::vec3 &position,
                          const opm::vec2 &form,
                          const Material &mat,
                          const opm::quat &rotate = opm::quat{0.0, 0.0, 0.0, 1.0});
    void BuildTorusBuffer();
    VulkanScene &AddRing(const opm::vec3 &position,
                         const opm::vec2 &form,
                         const Material &mat,
                         const opm::quat &rotate = opm::quat{0.0, 0.0, 0.0, 1.0});
    void BuildRingBuffer();
    VulkanScene &AddSurface(Surface &s, const opm::quat &rotate = opm::quat{0.0, 0.0, 0.0, 1.0});
    void BuildSurfaceBuffer();

public:
    static int s_LatestObjectID;

    static Material CreateMaterial(const opm::vec3 &color,
                                   float specular,
                                   float reflect = 1.0,
                                   float refract = 0.0,
                                   int colorTex = -1,
                                   int normalTex = -1);
};

DVAPI_ATTR SceneDefinition GetSceneDefinition(const VulkanScene &s);

DVAPI_ATTR Surface GetEllipsoid(float a, float b, float c, const Material &material);
DVAPI_ATTR Surface GetEllipticParaboloid(float a, float b, const Material &material);
DVAPI_ATTR Surface GetHyperbolicParaboloid(float a, float b, const Material &material);
DVAPI_ATTR Surface GetEllipticHyperboloidOneSheet(float a, float b, float c, const Material &material);
DVAPI_ATTR Surface GetEllipticHyperboloidTwoSheets(float a, float b, float c, const Material &material);
DVAPI_ATTR Surface GetEllipticCone(float a, float b, float c, const Material &material);
DVAPI_ATTR Surface GetEllipticCylinder(float a, float b, const Material &material);
DVAPI_ATTR Surface GetHyperbolicCylinder(float a, float b, const Material &material);
DVAPI_ATTR Surface GetParabolicCylinder(float a, const Material &material);

#endif
