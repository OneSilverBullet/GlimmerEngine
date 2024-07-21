#include "material.h"
#include "texturemanager.h"
#include "graphicscore.h"
#include "types/uuid.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>


PBRMaterial::PBRMaterial(const std::string& albedo, const std::string& normal, 
    const std::string& metalness, const std::string& roughness, const std::string& ao)
	: m_albedoPath(albedo), m_normalPath(normal), m_metalnessPath(metalness), 
    m_roughnessPath(roughness), m_aoPath(ao)
{
    m_matType = MATERIAL_TYPE::PBR;
    ResourceLoading();
    ResourceInitialize();
}


void PBRMaterial::ResourceLoading() {
    m_albedoTexture = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile(m_albedoPath, BlackOpaque2D, true);
    m_normalTexture = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile(m_normalPath, BlackOpaque2D, true);
    m_roughnessTexture = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile(m_metalnessPath, BlackOpaque2D, true);
    m_metalnessTexture = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile(m_roughnessPath, BlackOpaque2D, true);
    m_aoTexture = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile(m_aoPath, BlackOpaque2D, true);
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

MaterialManager::~MaterialManager() {
    for (auto item : m_materialContainer) {
        delete item.second;
    }
}

void MaterialManager::Initialize()
{
    const std::string materialFilePath = "resource\\materials";

    std::vector<std::string> objNames = TypeUtiles::ListFilesInDirectory(materialFilePath);

    UINT32 verticesOffset = 0;
    UINT32 indicesOffset = 0;
    for (int i = 0; i < objNames.size(); ++i) {
        std::string modelPath = materialFilePath + "\\" + objNames[i];
        std::ifstream file(modelPath);
        nlohmann::json j;
        file >> j;

        for (const auto& item : j["materials"]) {
            std::string submaterialName = item["name"];
            std::string materialType = item["type"];
            
            //based on different material type, generate different material
            if (materialType == "PBR") {
                std::string albedoMatPath = item["albedo"];
                std::string normalMatPath = item["normal"];
                std::string metalnessMatPath = item["metalness"];
                std::string roughnessMatPath = item["roughness"];
                std::string aoMatPath = item["ao"];

                Material* newMat = new PBRMaterial(albedoMatPath, normalMatPath, 
                    metalnessMatPath, roughnessMatPath, aoMatPath);

                char delimiter = ',';
                size_t pos = objNames[i].find(delimiter);
                if (pos != std::string::npos) {
                    // Split the string into two parts
                    std::string purModelName = objNames[i].substr(0, pos);
                    std::string subMatName = purModelName + "-" + submaterialName;
                    m_materialContainer[subMatName] = newMat;
                }
            }
        }
    }
}

Material* MaterialManager::GetMaterial(const std::string& modelName, const std::string& meshName)
{
    std::string matName = modelName + "-" + meshName;
    return m_materialContainer[matName];
}

UINT32 MaterialManager::GetMaterialTypeDescriptorNum(MATERIAL_TYPE matType) {
    if (matType == MATERIAL_TYPE::PBR)
        return 5;
    return 0;
}
