#pragma once
#include <vector>
#include <string>
#include "geometry/vertexformat.h"


class SkyBox
{
public:

	SkyBox();
	~SkyBox();
	void Initialize(std::string cubemapName);
	void Render();


private:
	std::vector<PBRVertex> m_vertices;
	std::vector<DWORD> m_indicies;
};



