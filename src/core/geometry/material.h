#pragma once

#include <string>
#include "texturemanager.h"
#include "descriptortypes.h"

class Material
{
public:


protected:
	virtual void ParseMaterial() = 0;
	virtual void ResourceLoading() = 0;
	virtual void ResourceInitialize() = 0;

};

//material structure for PBR rendering
class PBRMaterial : public Material
{
public:
	PBRMaterial(std::string materialPath);

	UINT64 GetTexturesGPUPtr() { return m_materialHandle.GetGPUPtr(); }
	UINT64 GetSamplersGPUPtr() { return m_samplerHandle.GetGPUPtr(); }
protected:
	void ParseMaterial() override;
	void ResourceLoading() override;
	void ResourceInitialize() override;


private:
	std::string m_materialPath;
	TextureRef m_albedoTexture;
	TextureRef m_normalTexture;
	TextureRef m_roughnessTexture;
	TextureRef m_metalnessTexture;
	TextureRef m_aoTexture;
	DescriptorHandle m_materialHandle;
	DescriptorHandle m_samplerHandle;
};

class MaterialManager
{
public:


	void Initialize();




private:
	std::map<std::string, Material*> m_materials; //UUID mapping material
	std::map<std::string, std::string> m_pathMappingUUID; //path mapping UUID
};

