#pragma once
#include <d3d12.h>
#include <vector>
#include <memory>
#include <queue>
#include <mutex>

#include "resources/gpuresource.h"
#include "resources/texture.h"


class TextureRef
{
public:
	TextureRef(const TextureRef& ref);
	
};


class TextureManager
{
public:

	void Initialize(const std::wstring& rootPath);
	void Release();
	

	TextureRef LoadDDSFromFile(const std::wstring& filePath);




private:
	Texture defaultTextures[6];
};







