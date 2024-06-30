#pragma once

#include <string>
#include <wtypes.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include "vertexformat.h"

class ObjModelLoader
{
public:
	static void LoadModel(std::string path, 
		std::vector<PBRVertex>& outputVertices,
		std::vector<DWORD>& outputIndices);

	static PBRVertex OBJVertexBuilder(std::string faceFrag,
		std::vector<DirectX::XMFLOAT3>& position,
		std::vector<DirectX::XMFLOAT2>& uvs,
		std::vector<DirectX::XMFLOAT3>& normals);
};

