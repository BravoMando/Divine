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

#include "VulkanSceneObject.h"
#include "VulkanLogger.h"

int VulkanScene::s_LatestObjectID = -1;

VulkanScene::VulkanScene()
{
}

VulkanScene::~VulkanScene()
{
    if (m_IsConnected)
    {
        DestroyAllBuffers();
        DestroyAllTextures();
    }
}

void VulkanScene::Connect(VulkanDevice *pDevice)
{
    if (pDevice == nullptr)
    {
        FATAL("Device must be valid!");
    }

    p_Device = pDevice;
    m_IsConnected = true;
}

void VulkanScene::ResizeAllBuffers(size_t size)
{
    SceneBuffers.resize(size);
    DirectLightsBuffer.resize(size);
    PointLightsBuffer.resize(size);
    SpheresBuffer.resize(size);
    PlanesBuffer.resize(size);
    BoxesBuffer.resize(size);
    TorusesBuffer.resize(size);
    RingsBuffer.resize(size);
    SurfacesBuffer.resize(size);
}

void VulkanScene::ResizeAllTextures(size_t size)
{
    SphereColors.resize(size);
    SphereNormals.resize(size);
    BoxesColors.resize(size);
    BoxesNormals.resize(size);
    TorusColors.resize(size);
    TorusNormals.resize(size);
    RingColors.resize(size);
    RingNormals.resize(size);
    SurfaceColors.resize(size);
    SurfaceNormals.resize(size);
}

void VulkanScene::DestroyAllBuffers()
{
    for (size_t i = 0; i < SceneBuffers.size(); ++i)
    {
        SceneBuffers[i].Destroy();
    }
    SceneBuffers.clear();
    for (size_t i = 0; i < PointLightsBuffer.size(); ++i)
    {
        PointLightsBuffer[i].Destroy();
    }
    PointLightsBuffer.clear();
    for (size_t i = 0; i < DirectLightsBuffer.size(); ++i)
    {
        DirectLightsBuffer[i].Destroy();
    }
    DirectLightsBuffer.clear();
    for (size_t i = 0; i < SpheresBuffer.size(); ++i)
    {
        SpheresBuffer[i].Destroy();
    }
    SpheresBuffer.clear();
    for (size_t i = 0; i < PlanesBuffer.size(); ++i)
    {
        PlanesBuffer[i].Destroy();
    }
    PlanesBuffer.clear();
    for (size_t i = 0; i < BoxesBuffer.size(); ++i)
    {
        BoxesBuffer[i].Destroy();
    }
    BoxesBuffer.clear();
    for (size_t i = 0; i < TorusesBuffer.size(); ++i)
    {
        TorusesBuffer[i].Destroy();
    }
    TorusesBuffer.clear();
    for (size_t i = 0; i < RingsBuffer.size(); ++i)
    {
        RingsBuffer[i].Destroy();
    }
    RingsBuffer.clear();
    for (size_t i = 0; i < SurfacesBuffer.size(); ++i)
    {
        SurfacesBuffer[i].Destroy();
    }
    SurfacesBuffer.clear();
}

void VulkanScene::DestroyAllTextures()
{
    for (size_t i = 0; i < SphereColors.size(); ++i)
    {
        SphereColors[i].Destroy();
    }
    SphereColors.clear();
    for (size_t i = 0; i < SphereNormals.size(); ++i)
    {
        SphereNormals[i].Destroy();
    }
    SphereNormals.clear();
    for (size_t i = 0; i < BoxesColors.size(); ++i)
    {
        BoxesColors[i].Destroy();
    }
    BoxesColors.clear();
    for (size_t i = 0; i < BoxesNormals.size(); ++i)
    {
        BoxesNormals[i].Destroy();
    }
    BoxesNormals.clear();
    for (size_t i = 0; i < TorusColors.size(); ++i)
    {
        TorusColors[i].Destroy();
    }
    TorusColors.clear();
    for (size_t i = 0; i < TorusNormals.size(); ++i)
    {
        TorusNormals[i].Destroy();
    }
    TorusNormals.clear();
    for (size_t i = 0; i < RingColors.size(); ++i)
    {
        RingColors[i].Destroy();
    }
    RingColors.clear();
    for (size_t i = 0; i < RingNormals.size(); ++i)
    {
        RingNormals[i].Destroy();
    }
    RingNormals.clear();
    for (size_t i = 0; i < SurfaceColors.size(); ++i)
    {
        SurfaceColors[i].Destroy();
    }
    SurfaceColors.clear();
    for (size_t i = 0; i < SurfaceNormals.size(); ++i)
    {
        SurfaceNormals[i].Destroy();
    }
    SurfaceNormals.clear();
}

VulkanScene &VulkanScene::AddPointLight(const opm::vec4 &positionAndRadius,
                                        const opm::vec3 &color,
                                        float intensity,
                                        float linearK,
                                        float quadraticK,
                                        const opm::quat &rotate)
{
    PointLight pl{};
    pl.ID = ++VulkanScene::s_LatestObjectID;
    pl.PositionAndRadius = positionAndRadius;
    pl.Color = color;
    pl.Intensity = intensity;
    pl.QuadraticK = quadraticK;
    pl.LinearK = linearK;
    pl.Rotation = rotate;
    PointLights.push_back(pl);

    return *this;
}

void VulkanScene::BuildPointLightBuffer()
{
    VulkanBuffer staging{};
    p_Device->CreateBuffer(PointLights.size() * sizeof(PointLight),
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &staging,
                           PointLights.data());
    for (size_t i = 0; i < PointLightsBuffer.size(); ++i)
    {
        p_Device->CreateBuffer(PointLights.size() * sizeof(PointLight),
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               &PointLightsBuffer[i]);
        p_Device->CopyBuffer(&staging, &PointLightsBuffer[i]);
    }
    staging.Destroy();
}

VulkanScene &VulkanScene::AddDirectLight(const opm::vec3 &dir,
                                         const opm::vec3 &color,
                                         float intensity,
                                         const opm::quat &rotate)
{
    DirectLight dl{};
    dl.ID = ++VulkanScene::s_LatestObjectID;
    dl.Direction = dir;
    dl.Color = color;
    dl.Intensity = intensity;
    dl.Rotation = rotate;
    DirectLights.push_back(dl);

    return *this;
}

void VulkanScene::BuildDirectLightBuffer()
{
    VulkanBuffer staging{};
    p_Device->CreateBuffer(DirectLights.size() * sizeof(DirectLight),
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &staging,
                           DirectLights.data());
    for (size_t i = 0; i < DirectLightsBuffer.size(); ++i)
    {
        p_Device->CreateBuffer(DirectLights.size() * sizeof(DirectLight),
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               &DirectLightsBuffer[i]);
        p_Device->CopyBuffer(&staging, &DirectLightsBuffer[i]);
    }
    staging.Destroy();
}

VulkanScene &VulkanScene::AddSphere(const opm::vec4 &centerAndRadius,
                                    const Material &mat,
                                    bool hollow,
                                    const opm::quat &rotate)
{
    Sphere s{};
    s.ID = ++VulkanScene::s_LatestObjectID;
    s.Hollow = hollow;
    s.CenterAndRadius = centerAndRadius;
    s.Mat = mat;
    s.Rotation = rotate;
    Spheres.push_back(s);

    return *this;
}

void VulkanScene::BuildSphereBuffer()
{
    VulkanBuffer staging{};
    p_Device->CreateBuffer(Spheres.size() * sizeof(Sphere),
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &staging,
                           Spheres.data());
    for (size_t i = 0; i < SpheresBuffer.size(); ++i)
    {
        p_Device->CreateBuffer(Spheres.size() * sizeof(Sphere),
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               &SpheresBuffer[i]);
        p_Device->CopyBuffer(&staging, &SpheresBuffer[i]);
    }
    staging.Destroy();
}

VulkanScene &VulkanScene::AddPlane(const opm::vec3 &normal,
                                   const opm::vec3 &position,
                                   const Material &mat,
                                   const opm::quat &rotate)
{
    Plane p{};
    p.ID = ++VulkanScene::s_LatestObjectID;
    p.Position = position;
    p.Normal = normal;
    p.Mat = mat;
    p.Rotation = rotate;
    Planes.push_back(p);

    return *this;
}

void VulkanScene::BuildPlaneBuffer()
{
    VulkanBuffer staging{};
    p_Device->CreateBuffer(Planes.size() * sizeof(Plane),
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &staging,
                           Planes.data());
    for (size_t i = 0; i < PlanesBuffer.size(); ++i)
    {
        p_Device->CreateBuffer(Planes.size() * sizeof(Plane),
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               &PlanesBuffer[i]);
        p_Device->CopyBuffer(&staging, &PlanesBuffer[i]);
    }
    staging.Destroy();
}

VulkanScene &VulkanScene::AddBox(const opm::vec3 &position,
                                 const opm::vec3 &form,
                                 const Material &mat,
                                 const opm::quat &rotate)
{
    Box b{};
    b.ID = ++VulkanScene::s_LatestObjectID;
    b.Position = position;
    b.Form = form;
    b.Mat = mat;
    b.Rotation = rotate;
    Boxes.push_back(b);

    return *this;
}

void VulkanScene::BuildBoxBuffer()
{
    VulkanBuffer staging{};
    p_Device->CreateBuffer(Boxes.size() * sizeof(Box),
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &staging,
                           Boxes.data());
    for (size_t i = 0; i < BoxesBuffer.size(); ++i)
    {
        p_Device->CreateBuffer(Boxes.size() * sizeof(Box),
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               &BoxesBuffer[i]);
        p_Device->CopyBuffer(&staging, &BoxesBuffer[i]);
    }
    staging.Destroy();
}

VulkanScene &VulkanScene::AddTorus(const opm::vec3 &position,
                                   const opm::vec2 &form,
                                   const Material &mat,
                                   const opm::quat &rotate)
{
    Torus t{};
    t.ID = ++VulkanScene::s_LatestObjectID;
    t.Position = position;
    t.Form = form;
    t.Mat = mat;
    t.Rotation = rotate;
    Toruses.push_back(t);

    return *this;
}

void VulkanScene::BuildTorusBuffer()
{
    VulkanBuffer staging{};
    p_Device->CreateBuffer(Toruses.size() * sizeof(Torus),
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &staging,
                           Toruses.data());
    for (size_t i = 0; i < TorusesBuffer.size(); ++i)
    {
        p_Device->CreateBuffer(Toruses.size() * sizeof(Torus),
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               &TorusesBuffer[i]);
        p_Device->CopyBuffer(&staging, &TorusesBuffer[i]);
    }
    staging.Destroy();
}

VulkanScene &VulkanScene::AddRing(const opm::vec3 &position,
                                  const opm::vec2 &form,
                                  const Material &mat,
                                  const opm::quat &rotate)
{
    Ring r{};
    r.ID = ++VulkanScene::s_LatestObjectID;
    r.Position = position;
    r.Form = form;
    r.Mat = mat;
    r.Rotation = rotate;
    Rings.push_back(r);

    return *this;
}

void VulkanScene::BuildRingBuffer()
{
    VulkanBuffer staging{};
    p_Device->CreateBuffer(Rings.size() * sizeof(Ring),
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &staging,
                           Rings.data());
    for (size_t i = 0; i < RingsBuffer.size(); ++i)
    {
        p_Device->CreateBuffer(Rings.size() * sizeof(Ring),
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               &RingsBuffer[i]);
        p_Device->CopyBuffer(&staging, &RingsBuffer[i]);
    }
    staging.Destroy();
}

VulkanScene &VulkanScene::AddSurface(Surface &s, const opm::quat &rotate)
{
    // TODO: fix surface id generation
    s.ID = ++VulkanScene::s_LatestObjectID;
    s.Rotation = rotate;
    Surfaces.push_back(s);

    return *this;
}

void VulkanScene::BuildSurfaceBuffer()
{
    VulkanBuffer staging{};
    p_Device->CreateBuffer(Surfaces.size() * sizeof(Surface),
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &staging,
                           Surfaces.data());
    for (size_t i = 0; i < SurfacesBuffer.size(); ++i)
    {
        p_Device->CreateBuffer(Surfaces.size() * sizeof(Surface),
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               &SurfacesBuffer[i]);
        p_Device->CopyBuffer(&staging, &SurfacesBuffer[i]);
    }
    staging.Destroy();
}

Material VulkanScene::CreateMaterial(const opm::vec3 &color,
                                     float specular,
                                     float reflect,
                                     float refract,
                                     int colorTex,
                                     int normalTex)
{
    Material mat{};
    mat.ColorTexture = colorTex;
    mat.NormalTexture = normalTex;
    mat.Reflect = reflect;
    mat.Refract = refract;
    mat.Specular = specular;
    mat.Color = color;

    return mat;
}

Surface GetEllipsoid(float a, float b, float c, const Material &material)
{
    Surface surface{};
    surface.a = powf(a, -2);
    surface.b = powf(b, -2);
    surface.c = powf(c, -2);
    surface.f = -1;
    surface.Mat = material;
    return surface;
}

Surface GetEllipticParaboloid(float a, float b, const Material &material)
{
    Surface surface{};
    surface.a = powf(a, -2);
    surface.b = powf(b, -2);
    surface.d = -1;
    surface.Mat = material;
    return surface;
}

Surface GetHyperbolicParaboloid(float a, float b, const Material &material)
{
    Surface surface{};
    surface.a = powf(a, -2);
    surface.b = -powf(b, -2);
    surface.d = -1;
    surface.Mat = material;
    return surface;
}

Surface GetEllipticHyperboloidOneSheet(float a, float b, float c, const Material &material)
{
    Surface surface{};
    surface.a = powf(a, -2);
    surface.b = powf(b, -2);
    surface.c = -powf(c, -2);
    surface.f = -1;
    surface.Mat = material;
    return surface;
}

Surface GetEllipticHyperboloidTwoSheets(float a, float b, float c, const Material &material)
{
    Surface surface{};
    surface.a = powf(a, -2);
    surface.b = powf(b, -2);
    surface.c = -powf(c, -2);
    surface.f = 1;
    surface.Mat = material;
    return surface;
}

Surface GetEllipticCone(float a, float b, float c, const Material &material)
{
    Surface surface{};
    surface.a = powf(a, -2);
    surface.b = powf(b, -2);
    surface.c = -powf(c, -2);
    surface.Mat = material;
    return surface;
}

Surface GetEllipticCylinder(float a, float b, const Material &material)
{
    Surface surface{};
    surface.a = powf(a, -2);
    surface.b = powf(b, -2);
    surface.f = -1;
    surface.Mat = material;
    return surface;
}

Surface GetHyperbolicCylinder(float a, float b, const Material &material)
{
    Surface surface{};
    surface.a = powf(a, -2);
    surface.b = -powf(b, -2);
    surface.f = -1;
    surface.Mat = material;
    return surface;
}

Surface GetParabolicCylinder(float a, const Material &material)
{
    Surface surface{};
    surface.a = 1;
    surface.e = 2 * a;
    surface.Mat = material;
    return surface;
}

SceneDefinition GetSceneDefinition(const VulkanScene &s)
{
    SceneDefinition defines{};
    defines.SphereCount = static_cast<uint32_t>(s.Spheres.size());
    defines.PlaneCount = static_cast<uint32_t>(s.Planes.size());
    defines.BoxCount = static_cast<uint32_t>(s.Boxes.size());
    defines.TorusCount = static_cast<uint32_t>(s.Toruses.size());
    defines.RingCount = static_cast<uint32_t>(s.Rings.size());
    defines.SurfaceCount = static_cast<uint32_t>(s.Surfaces.size());
    defines.DirectLightCount = static_cast<uint32_t>(s.DirectLights.size());
    defines.PointLightCount = static_cast<uint32_t>(s.PointLights.size());

    return defines;
}
