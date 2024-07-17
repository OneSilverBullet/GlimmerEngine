#pragma once

#include <string>
#include <map>
#include "mesh.h"
#include "resources/byteaddressbuffer.h"
#include "material.h"
struct aiScene;
struct aiNode;


//describe the model infor in geometry buffer
struct ModelInfor
{
	UINT32 verticesOffset;
	UINT32 indicesOffset;
	UINT32 verticesSize;
	UINT32 indicesSize;
	std::vector<MeshInfo> meshesInfor;
	//todo: material material refs
};

class Model
{
public:
	Model();
	~Model();
	void Initialize(const std::string& path);
	ModelInfor GetModelInfor();
	std::vector<GeometryVertex>& GetBatchVertices();
	std::vector<UINT32>& GetBatchIndices();

private:
	void GenerateBatchVertices();
	void GenerateBatchIndices();
	void GenerateBatchModelInfor();
	void TraverseNode(const aiScene* scene, aiNode* node);


private:
	std::vector<Mesh> m_meshes;
	std::vector<Material*> m_materials; //the number of the materials is same as meshes
	std::vector<GeometryVertex> m_batchVertices;
	std::vector<UINT32> m_batchIndices;

	ModelInfor m_modelInfo;

	//todo: material vector
};


//todo: add object instance
class ModelRef
{
public:
	ModelRef(){}

	ModelRef(std::vector<D3D12_VERTEX_BUFFER_VIEW>& vbv, std::vector<D3D12_INDEX_BUFFER_VIEW>& ibv, std::vector<UINT32>& indicesSizes)
		: m_vertices(vbv), m_indices(ibv), m_indicesSizes(indicesSizes)
	{}

	ModelRef(const ModelRef& v) {
		this->operator=(v);
	}

	std::vector<D3D12_VERTEX_BUFFER_VIEW>& GetMeshVertexBufferView() { return m_vertices; }
	std::vector<D3D12_INDEX_BUFFER_VIEW>& GetIndicesVertexBufferView() { return m_indices; }
	std::vector<UINT32>& GetIndicesSizes() { return m_indicesSizes; }

	ModelRef& operator=(const ModelRef& modelInstance) {
		m_vertices.reserve(modelInstance.m_vertices.size());
		for (int i = 0; i < modelInstance.m_vertices.size(); ++i) {
			m_vertices.push_back(modelInstance.m_vertices[i]);
		}
		m_indices.reserve(modelInstance.m_indices.size());
		for (int i = 0; i < modelInstance.m_indices.size(); ++i) {
			m_indices.push_back(modelInstance.m_indices[i]);
		}
		m_indicesSizes.reserve(modelInstance.m_indicesSizes.size());
		for (int i = 0; i < modelInstance.m_indicesSizes.size(); ++i) {
			m_indicesSizes.push_back(modelInstance.m_indicesSizes[i]);
		}
		return *this;
	}

private:
	std::vector<D3D12_VERTEX_BUFFER_VIEW> m_vertices;
	std::vector<D3D12_INDEX_BUFFER_VIEW> m_indices;
	std::vector<UINT32> m_indicesSizes;

};

//model manager
class ModelManager
{
public:
	//only load models once
	void Initialize();
	
	ModelRef GetModelRef(const std::string& modelPath);



private:
	void BuildupGeometryBuffer();
	
private:
	std::map<std::string, Model*> m_models; //UUID mapping model instance
	std::map<std::string, ModelInfor> m_modelInfors; //UUID mapping model infor
	std::map<std::string, std::string> m_nameUUIDMapping; //name mapping UUID
	std::map<std::string, std::vector<D3D12_VERTEX_BUFFER_VIEW>> m_modelVBV;
	std::map<std::string, std::vector<D3D12_INDEX_BUFFER_VIEW>> m_modelIBV;
	std::map<std::string, std::vector<UINT32>> m_modelIndicesSize;

	UINT32 m_verticesSize;
	UINT32 m_indicesSize;

	ByteAddressBuffer m_geometryBuffer;
};