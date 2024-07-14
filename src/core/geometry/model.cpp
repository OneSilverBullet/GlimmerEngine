#include "model.h"
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "resources/uploadbuffer.h"
#include "types/commontypes.h"
#include "types/uuid.h"

Model::Model() {
	m_meshes.reserve(100);
}

Model::~Model() {
	m_meshes.clear();
	m_batchVertices.clear();
	m_batchIndices.clear();
}

void Model::Initialize(const std::string& path)
{
	Assimp::Importer localImporter;

	const aiScene* pLocalScene = localImporter.ReadFile(
		path,
		// Triangulates all faces of all meshes
		aiProcess_Triangulate |
		// Supersedes the aiProcess_MakeLeftHanded and aiProcess_FlipUVs and aiProcess_FlipWindingOrder flags
		aiProcess_ConvertToLeftHanded |
		// This preset enables almost every optimization step to achieve perfectly optimized data. In D3D, need combine with aiProcess_ConvertToLeftHanded
		aiProcessPreset_TargetRealtime_MaxQuality |
		// Calculates the tangents and bitangents for the imported meshes
		aiProcess_CalcTangentSpace |
		// Splits large meshes into smaller sub-meshes
		// This is quite useful for real-time rendering, 
		// where the number of triangles which can be maximally processed in a single draw - call is limited by the video driver / hardware
		aiProcess_SplitLargeMeshes |
		// A postprocessing step to reduce the number of meshes
		aiProcess_OptimizeMeshes |
		// A postprocessing step to optimize the scene hierarchy
		aiProcess_OptimizeGraph);

	// "localScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE" is used to check whether value data returned is incomplete
	if (pLocalScene == nullptr || pLocalScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || pLocalScene->mRootNode == nullptr)
	{
		std::cout << "ERROR::ASSIMP::" << localImporter.GetErrorString() << std::endl;
	}

	//directory = path.substr(0, path.find_last_of('/'));

	TraverseNode(pLocalScene, pLocalScene->mRootNode);


	//generate batch vertices and indices
	GenerateBatchVertices();
	GenerateBatchIndices();

	//generate model information
	GenerateBatchModelInfor();
}

ModelInfor Model::GetModelInfor() {
	return m_modelInfo;
}

void Model::TraverseNode(const aiScene* scene, aiNode* node)
{
	// load mesh
	for (UINT i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* pLocalMesh = scene->mMeshes[node->mMeshes[i]];
		
		m_meshes.push_back(Mesh());
		m_meshes.back().LoadMeshData(scene, pLocalMesh);
	}

	// traverse child node
	for (UINT i = 0; i < node->mNumChildren; ++i)
	{
		TraverseNode(scene, node->mChildren[i]);
	}
}


void Model::GenerateBatchVertices() {
	m_batchVertices.reserve(100);

	for (auto& m : m_meshes)
	{
		for (auto& v : m.GetVertices())
		{
			m_batchVertices.push_back(v);
		}
	}
}

void Model::GenerateBatchIndices() {
	m_batchIndices.reserve(100);

	for (auto& m : m_meshes)
	{
		for (auto& i : m.GetIndices())
		{
			m_batchIndices.push_back(i);
		}
	}
}

void Model::GenerateBatchModelInfor()
{
	//todo: model with the same material should be in same batch

	UINT32 verticesSize = m_batchVertices.size() * sizeof(GeometryVertex);
	UINT32 indicesSize = m_batchIndices.size() * sizeof(UINT32);

	m_modelInfo.verticesSize = verticesSize;
	m_modelInfo.indicesSize = indicesSize;

	for (int i = 0; i < m_meshes.size(); ++i) {
		MeshInfo meshInfor = m_meshes[i].GetMeshInfor();
		m_modelInfo.meshesInfor.push_back(meshInfor);
	}
}

std::vector<GeometryVertex>& Model::GetBatchVertices() {
	return m_batchVertices;
}

std::vector<UINT32>& Model::GetBatchIndices() {
	return m_batchIndices;
}

void ModelManager::Initialize()
{
	const std::string modelFilePath = "resource\\models";
	
	std::vector<std::string> objNames = ListFilesInDirectory(modelFilePath);

	UINT32 verticesOffset = 0;
	UINT32 indicesOffset = 0;
	for (int i = 0; i < objNames.size(); ++i) {
		std::string modelPath = modelFilePath + "\\" + objNames[i];
		UUID uuid(modelPath);
		//build up the connection between uuid and name 
		m_nameUUIDMapping[modelPath] = uuid.toString();

		//build up the models loading
		Model* model = new Model();
		model->Initialize(modelPath);
		m_models[uuid.toString()] = model;

		//build up the model infor
		ModelInfor modelInfor =  model->GetModelInfor();
		modelInfor.verticesOffset = verticesOffset;
		modelInfor.indicesOffset = indicesOffset;
		m_modelInfors[uuid.toString()] = modelInfor;

		verticesOffset += modelInfor.verticesSize;
		indicesOffset += modelInfor.indicesSize;
	}
	m_verticesSize = verticesOffset;
	m_indicesSize = indicesOffset;

	//update the geometry data to GPU buffer
	BuildupGeometryBuffer();

}

void ModelManager::BuildupGeometryBuffer() {

	//collect the vertices and indices
	std::vector<GeometryVertex> vertices;
	std::vector<UINT32> indices;
	for (auto item : m_models) {
		std::vector<GeometryVertex> modelVBatch = item.second->GetBatchVertices();
		std::vector<UINT32> modelIBatch = item.second->GetBatchIndices();
		for (int i = 0; i < (size_t)modelVBatch.size(); ++i) {
			vertices.push_back(modelVBatch[i]);
		}
		for (int i = 0; i < (size_t)modelIBatch.size(); ++i) {
			indices.push_back(modelIBatch[i]);
		}
	}

	assert(m_verticesSize == vertices.size() * sizeof(GeometryVertex));
	assert(m_indicesSize == indices.size() * sizeof(UINT32));

	uint32_t uploadBufferSize = m_verticesSize + m_indicesSize;

	UploadBuffer uploadBuffer;
	uploadBuffer.Create(L"Upload Buffer", uploadBufferSize);

	uint8_t* uploadMem = (uint8_t*)uploadBuffer.Map();

	memcpy(uploadMem, vertices.data(), m_verticesSize);
	memcpy(uploadMem + m_verticesSize, indices.data(), m_indicesSize);

	m_geometryBuffer.Create(L"Static Geometry Buffer", uploadBufferSize, 1, uploadBuffer);

	//generate the vertex and index view
	for (auto item : m_modelInfors) {
		std::string modelUUID = item.first;
		ModelInfor modelInfo = item.second;
		UINT32 modelVerticesStart = modelInfo.verticesOffset;
		UINT32 modelIndicesStart = modelInfo.indicesOffset;
		for (int i = 0; i < modelInfo.meshesInfor.size(); ++i) {
			MeshInfo meshInfo = modelInfo.meshesInfor[i];
			UINT32 meshVerticesSize = meshInfo.m_verticesSize;
			UINT32 meshIndicesSize = meshInfo.m_indicesSize;

			D3D12_VERTEX_BUFFER_VIEW meshVBV = m_geometryBuffer.VertexBufferView(modelVerticesStart, meshVerticesSize, sizeof(GeometryVertex));
			D3D12_INDEX_BUFFER_VIEW meshIBV = m_geometryBuffer.IndexBufferView(modelIndicesStart, meshIndicesSize, true);

			m_modelVBV[modelUUID].push_back(meshVBV);
			m_modelIBV[modelUUID].push_back(meshIBV);

			modelVerticesStart += meshVerticesSize;
			modelIndicesStart += meshIndicesSize;
		}
	}
}

ModelRef ModelManager::GetModelRef(const std::string& modelPath) {
	std::string UUID = m_nameUUIDMapping[modelPath];
	std::vector<D3D12_VERTEX_BUFFER_VIEW> vertices = m_modelVBV[UUID];
	std::vector<D3D12_INDEX_BUFFER_VIEW> indices = m_modelIBV[UUID];
	return ModelRef(vertices, indices);
}


