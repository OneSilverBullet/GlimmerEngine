#pragma once
#include <vector>
#include <string>
#include "vertexformat.h"

struct aiScene;
struct aiMesh;

class Mesh
{
public:
	Mesh();
	~Mesh();
	void LoadMeshData(const aiScene* scene, aiMesh* mesh);

	std::vector<GeometryVertex>& GetVertices() { return m_vertices; }
	std::vector<UINT32>& GetIndices() { return m_indices; }

private:
	std::vector<GeometryVertex> m_vertices;
	std::vector<UINT32> m_indices;
};
