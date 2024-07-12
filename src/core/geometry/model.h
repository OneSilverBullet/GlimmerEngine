#pragma once

#include <string>
#include "mesh.h"

struct aiScene;
struct aiNode;

class Model
{
public:
	Model();
	~Model();
	void Initialize(const std::string& path);
	void TraverseNode(const aiScene* scene, aiNode* node);

	void GenerateBatchVertices();
	void GenerateBatchIndices();

	std::vector<GeometryVertex>& GetBatchVertices();
	std::vector<UINT32>& GetBatchIndices();

private:
	std::vector<Mesh> m_meshes;
	std::vector<GeometryVertex> m_batchVertices;
	std::vector<UINT32> m_batchIndices;
};
