#pragma once
#include "texturemanager.h"
#include "context.h"
#include "graphicscore.h"
#include "rootsignature.h"
#include "resources/DDSTextureLoader.h"
#include <thread>

class ManagedTexture : public Texture
{
	friend class TextureRef;
public:
	ManagedTexture(const std::string& fileName);

	void WaitForLoad(void) const;
	void CreateFromMemory(std::string memory, DefaultTextureType fallback, bool sRGB);

private:
	bool IsValid(void) const { return m_isValid; }
	void Unload();

	std::string m_mapKey;
	bool m_isValid;
	bool m_isLoading;
	size_t m_referenceCount;
};

ManagedTexture::ManagedTexture(const std::string& fileName)
	: m_mapKey(fileName), m_isValid(false), m_isLoading(true), m_referenceCount(0)
{
	m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

void ManagedTexture::WaitForLoad(void) const
{
	while ((volatile bool&)m_isLoading)
		std::this_thread::yield();
}

void ManagedTexture::CreateFromMemory(std::string memory, DefaultTextureType fallback, bool sRGB)
{
	if (memory.size() == 0)
	{
		m_hCpuDescriptorHandle = GRAPHICS_CORE::g_textureManager.GetDefaultTexture(fallback);
	}
	else {
		m_hCpuDescriptorHandle = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		if (CreateDDSTextureFromMemory(GRAPHICS_CORE::g_device, 
			(const uint8_t*)memory.data(), memory.size(),
			0, sRGB, &m_resource, m_hCpuDescriptorHandle))
		{
			m_isValid = true;
			D3D12_RESOURCE_DESC desc = GetResource()->GetDesc();
			m_Width = desc.Width;
			m_Height = desc.Height;
			m_Depth = desc.DepthOrArraySize;
		}
		else {
			GRAPHICS_CORE::g_device->CopyDescriptorsSimple(1, m_hCpuDescriptorHandle,
				GRAPHICS_CORE::g_textureManager.GetDefaultTexture(fallback), 
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}
	m_isLoading = true;
}

void ManagedTexture::Unload()
{
	GRAPHICS_CORE::g_textureManager.DestoryTexture(m_mapKey);
}


void TextureManager::Initialize(const std::string& rootPath)
{
	//initialize default textures
	uint32_t magentaPixel = 0xFFFF00FF;
	defaultTextures[DefaultTextureType::Magenta2D].Create2D(4, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &magentaPixel);
	uint32_t blackOpaqueTexel = 0xFF000000;
	defaultTextures[DefaultTextureType::BlackOpaque2D].Create2D(4, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &blackOpaqueTexel);
	uint32_t blackTransparentTexel = 0x00000000;
	defaultTextures[DefaultTextureType::BlackTransparent2D].Create2D(4, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &blackTransparentTexel);
	uint32_t whiteOpaqueTexel = 0xFFFFFFFF;
	defaultTextures[DefaultTextureType::WhiteOpaque2D].Create2D(4, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &whiteOpaqueTexel);
	uint32_t whiteTransparentTexel = 0x00FFFFFF;
	defaultTextures[DefaultTextureType::WhiteTransparent2D].Create2D(4, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &whiteTransparentTexel);
	uint32_t flatNormalTexel = 0x00FF8080;
	defaultTextures[DefaultTextureType::DefaultNormalMap2D].Create2D(4, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &flatNormalTexel);
	uint32_t blackCubeTexels[6] = {};
	defaultTextures[DefaultTextureType::BlackCubeMap].CreateCube(4, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, blackCubeTexels);

	//setup root path
	m_rootPath = rootPath;
}

void TextureManager::Release()
{
	m_texturesCache.clear();
}

TextureRef TextureManager::LoadDDSFromFile(const std::string& filePath, DefaultTextureType defaultTex, bool sRGB)
{
	return FindOrLoadTexture(filePath, defaultTex, sRGB);
}

void TextureManager::DestoryTexture(const std::string& key) {
	std::lock_guard<std::mutex> guard(m_mutex);
	//erase the texture instance by the key
	auto iter = m_texturesCache.find(key);
	if (iter != m_texturesCache.end())
		m_texturesCache.erase(iter);
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::GetDefaultTexture(DefaultTextureType defaultID)
{
	return defaultTextures[defaultID].GetSRV();
}

ManagedTexture* TextureManager::FindOrLoadTexture(const std::string& filename, 
	DefaultTextureType texType, bool forceSRGB) {

	ManagedTexture* tex = nullptr;

	{
		std::lock_guard<std::mutex> guard(m_mutex);
		std::string key = filename;
		if (forceSRGB)
			key += "_sRGB";

		auto iter = m_texturesCache.find(key);
		if (iter != m_texturesCache.end()) {
			tex = iter->second.get();
			tex->WaitForLoad(); //if texture has loaded
			return tex;
		}
		else {
			tex = new ManagedTexture(key);
			m_texturesCache[key].reset(tex);
		}
	}

	std::string filenameLoaded = m_rootPath + filename + ".dds";
	tex->CreateFromMemory(filenameLoaded.c_str(), texType, forceSRGB);

	return tex;
}

/*
* TextureRef
*/
TextureRef::TextureRef(const TextureRef& ref) : m_ref(ref.m_ref) {
	if (m_ref != nullptr)
		++m_ref->m_referenceCount;
}

TextureRef::TextureRef(ManagedTexture* tex) {
	if (m_ref != nullptr)
		++m_ref->m_referenceCount;
}

TextureRef::~TextureRef() {
	if (m_ref != nullptr && --m_ref->m_referenceCount == 0)
		m_ref->Unload();
}

void TextureRef::operator=(std::nullptr_t) {
	if (m_ref != nullptr && --m_ref->m_referenceCount == 0)
		m_ref->Unload();
}

void TextureRef::operator=(TextureRef& rhs) {
	if (m_ref != nullptr)
		--m_ref->m_referenceCount;

	m_ref = rhs.m_ref;

	if (m_ref != nullptr)
		++m_ref->m_referenceCount;
}

bool TextureRef::IsValid() const {
	return m_ref && m_ref->IsValid();
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureRef::GetSRV() const {
	if (m_ref != nullptr)
		return m_ref->GetSRV();
	return GRAPHICS_CORE::g_textureManager.GetDefaultTexture(DefaultTextureType::WhiteOpaque2D);
}

const Texture* TextureRef::Get(void) const {
	return m_ref;
}

const Texture* TextureRef::operator->(void) const {
	return m_ref;
}
