#include "material.h"
#include "texturemanager.h"
#include "graphicscore.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>


PBRMaterial::PBRMaterial(std::string materialPath)
	: m_materialPath(materialPath)
{
    ResourceLoading();
    ResourceInitialize();
}


void PBRMaterial::ParseMaterial()
{
    std::ifstream file(m_materialPath);
    nlohmann::json j;
    file >> j;

    //vector to hold material
    std::vector<Material> materials;

    // Parse and populate the materials
    for (const auto& item : j["materials"]) {
        Material material;
        material = item["name"];
        std::copy(item["ambient"].begin(), item["ambient"].end(), );
        std::copy(item["diffuse"].begin(), item["diffuse"].end(), material.diffuse);
        std::copy(item["specular"].begin(), item["specular"].end(), material.specular);
        material.shininess = item["shininess"];
        materials.push_back(material);
    }

    // Output the material properties for verification
    for (const auto& material : materials) {
        std::cout << "Material: " << material.name << std::endl;
        std::cout << "Ambient: " << material.ambient[0] << ", " << material.ambient[1] << ", " << material.ambient[2] << std::endl;
        std::cout << "Diffuse: " << material.diffuse[0] << ", " << material.diffuse[1] << ", " << material.diffuse[2] << std::endl;
        std::cout << "Specular: " << material.specular[0] << ", " << material.specular[1] << ", " << material.specular[2] << std::endl;
        std::cout << "Shininess: " << material.shininess << std::endl << std::endl;
    }
}

void PBRMaterial::ResourceLoading() {
    std::string albedoTexturePath = m_materialPath + "\\" + GRAPHICS_CORE::g_pbrmaterialTextureName[0] + ".dds";
    std::string normalTexturePath = m_materialPath + "\\" + GRAPHICS_CORE::g_pbrmaterialTextureName[1] + ".dds";
    std::string roughnessTexturePath = m_materialPath + "\\" + GRAPHICS_CORE::g_pbrmaterialTextureName[2] + ".dds";
    std::string metalnessTexturePath = m_materialPath + "\\" + GRAPHICS_CORE::g_pbrmaterialTextureName[3] + ".dds";
    std::string aoTexturePath = m_materialPath + "\\" + GRAPHICS_CORE::g_pbrmaterialTextureName[4] + ".dds";

    m_albedoTexture = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile(albedoTexturePath, BlackOpaque2D, true);
    m_normalTexture = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile(normalTexturePath, BlackOpaque2D, true);
    m_roughnessTexture = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile(roughnessTexturePath, BlackOpaque2D, true);
    m_metalnessTexture = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile(metalnessTexturePath, BlackOpaque2D, true);
    m_aoTexture = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile(aoTexturePath, BlackOpaque2D, true);
}

void PBRMaterial::ResourceInitialize()
{
    //allocate descriptor handle
    m_materialHandle = GRAPHICS_CORE::g_texturesDescriptorHeap.Alloc(5);
    m_samplerHandle = GRAPHICS_CORE::g_samplersDescriptorHeap.Alloc(1);

    //texture loading process
    D3D12_CPU_DESCRIPTOR_HANDLE textures[] = {
        m_albedoTexture.GetSRV(),
        m_normalTexture.GetSRV(),
        m_roughnessTexture.GetSRV(),
        m_metalnessTexture.GetSRV(),
        m_aoTexture.GetSRV()
    };

    UINT texturesDestNum = 5;
    UINT texturesSrcNums[] = { 1, 1, 1, 1, 1 };

    GRAPHICS_CORE::g_device->CopyDescriptors(1, &m_materialHandle, &texturesDestNum, texturesDestNum, 
        textures, texturesSrcNums, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    UINT samplerDestNum = 1;
    UINT samplerSrcNums[] = { 1 };

    //sampler loading process
    D3D12_CPU_DESCRIPTOR_HANDLE samplers[] = {
        GRAPHICS_CORE::g_samplerLinearWrap
    };

    GRAPHICS_CORE::g_device->CopyDescriptors(1, &m_samplerHandle, &samplerDestNum, samplerDestNum,
        samplers, samplerSrcNums, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}
