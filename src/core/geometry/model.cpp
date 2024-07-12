#include "model.h"
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

std::vector<GeometryVertex>& Model::GetBatchVertices() {
	return m_batchVertices;
}

std::vector<UINT32>& Model::GetBatchIndices() {
	return m_batchIndices;
}