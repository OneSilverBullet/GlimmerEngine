#include "mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Mesh::Mesh() {
	//optimization for vector reallocation
	m_vertices.reserve(100);
	m_indices.reserve(100);
}

Mesh::~Mesh() {
	m_vertices.clear();
	m_indices.clear();
}

void Mesh::LoadMeshData(const aiScene* scene, aiMesh* mesh)
{
	// process vertex position, normal, tangent, texture coordinates
	for (UINT i = 0; i < mesh->mNumVertices; ++i)
	{
		GeometryVertex localVertex;

		localVertex.position.x = mesh->mVertices[i].x;
		localVertex.position.y = mesh->mVertices[i].y;
		localVertex.position.z = mesh->mVertices[i].z;

		localVertex.normal.x = mesh->mNormals[i].x;
		localVertex.normal.y = mesh->mNormals[i].y;
		localVertex.normal.z = mesh->mNormals[i].z;

		localVertex.tangent.x = mesh->mTangents[i].x;
		localVertex.tangent.y = mesh->mTangents[i].y;
		localVertex.tangent.z = mesh->mTangents[i].z;

		// assimp allow one model have 8 different texture coordinates in one vertex, but we just care first texture coordinates because we will not use so many
		if (mesh->mTextureCoords[0])
		{
			localVertex.uv.x = mesh->mTextureCoords[0][i].x;
			localVertex.uv.y = mesh->mTextureCoords[0][i].y;
		}
		else
		{
			localVertex.uv = DirectX::XMFLOAT2(0.0f, 0.0f);
		}

		m_vertices.push_back(localVertex);
	}

	for (UINT i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace localFace = mesh->mFaces[i];
		for (UINT j = 0; j < localFace.mNumIndices; ++j)
		{
			m_indices.push_back(localFace.mIndices[j]);
		}
	}
}