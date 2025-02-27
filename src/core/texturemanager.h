#pragma once
#include <d3d12.h>
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <unordered_map>

#include "resources/gpuresource.h"
#include "resources/texture.h"

class ManagedTexture : public Texture
{
	friend class TextureRef;
public:
	ManagedTexture(const std::string& fileName);

	void WaitForLoad(void) const;
	void CreateFromFile(std::wstring filePath, DefaultTextureType fallback, bool sRGB);
	void CreateFromHDRFile(std::string filePath);

private:
	bool IsValid(void) const { return m_isValid; }
	void Unload();

	std::string m_mapKey;
	bool m_isValid;
	bool m_isLoading;
	size_t m_referenceCount;
};


//the encapsulation of the managed texture
//implement the function of texture ptr
class TextureRef
{
public:
	TextureRef(const TextureRef& ref);
	TextureRef(ManagedTexture* tex = nullptr);
	~TextureRef();

	//assign the nullptr to the texture ptr
	void operator=(std::nullptr_t); 
	void operator=(TextureRef& rhs);

	bool IsValid() const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const;

	Texture* Get(void) const;
	Texture* operator->(void) const;

private:
	ManagedTexture* m_ref = nullptr;
};

class TextureManager
{
public:

	void Initialize(const std::string& rootPath);
	void Release();


	TextureRef LoadDDSFromFile(const std::string& filePath, DefaultTextureType defaultTex, bool sRGB = false);
	void DestoryTexture(const std::string& key);

	D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultTexture(DefaultTextureType texID);

private:
	ManagedTexture* FindOrLoadTexture(const std::string& filename, DefaultTextureType texType, bool forceSRGB);

private:
	std::mutex m_mutex;
	std::string m_rootPath = "";
	Texture defaultTextures[DefaultTextureType::DefaultTextureNum];
	std::unordered_map<std::string, std::unique_ptr<ManagedTexture>> m_texturesCache;
};






