#pragma once
#include <string>
#include "geometry/material.h"
#include "geometry/model.h"


class RenderItem
{
public:
	RenderItem(){}
	void Initialize(const std::string& objName);
	std::vector<D3D12_VERTEX_BUFFER_VIEW>& GetMeshVertexBufferView() { return m_modelRef.GetMeshVertexBufferView(); }
	std::vector<D3D12_INDEX_BUFFER_VIEW>& GetIndicesVertexBufferView() { return m_modelRef.GetIndicesVertexBufferView(); }
	std::vector<UINT32>& GetIndicesSizes() { return m_modelRef.GetIndicesSizes(); }
	std::vector<Material*>& GetMaterials() { return m_materials; }
	

private:
	std::vector<Material*> m_materials;
	ModelRef m_modelRef;
};

