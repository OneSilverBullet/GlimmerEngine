#pragma once

#include <string>
#include "texturemanager.h"
#include "descriptortypes.h"
#include <map>


enum class MATERIAL_TYPE {
	PBR = 0,
	MAT_TYPE_NUM = 1,
};


class Material
{
public:
	MATERIAL_TYPE GetMatType() { return m_matType; }
	virtual std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetTextureSRVArray() = 0;
	virtual std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetSamplerSRVArray() = 0;

protected:
	virtual void ResourceLoading() = 0;
	virtual void ResourceInitialize() = 0;


protected:
	MATERIAL_TYPE m_matType;
};

//material structure for PBR rendering
class PBRMaterial : public Material
{
public:
	PBRMaterial(const std::string& albedo, const std::string& normal, const std::string& metalness, const std::string& roughness, const std::string& ao);

	UINT64 GetTexturesGPUPtr() { return m_materialHandle.GetGPUPtr(); }
	UINT64 GetSamplersGPUPtr() { return m_samplerHandle.GetGPUPtr(); }

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetTextureSRVArray() override;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetSamplerSRVArray() override;

protected:
	void ResourceLoading() override;
	void ResourceInitialize() override;

private:
	//texture path
	std::string m_albedoPath;
	std::string m_normalPath;
	std::string m_roughnessPath;
	std::string m_metalnessPath;
	std::string m_aoPath;

	//texture ref
	TextureRef m_albedoTexture;
	TextureRef m_normalTexture;
	TextureRef m_roughnessTexture;
	TextureRef m_metalnessTexture;
	TextureRef m_aoTexture;

	//the descriptor handle 
	DescriptorHandle m_materialHandle;
	DescriptorHandle m_samplerHandle;
};

class MaterialManager
{
public:
	MaterialManager(){}
	~MaterialManager();
	void Initialize();
	Material* GetMaterial(const std::string& modelName, const std::string& meshName);
	UINT32 GetMaterialTypeDescriptorNum(MATERIAL_TYPE matType);

private:
	std::map<std::string, Material*> m_materialContainer; //name mapping material
};

