#pragma once

#include <string>

//value���������ֵ�string
int string_to_int(string value);

class ObjModelLoader
{
public:
	BaseMesh<PBRVertex> MeshBuilder(string path);


	void LoadModel(string path);
	//��ʽ���� x/x/x
	PBRVertex PBRVertexBuilder(string face_frag);

private:
	vector<XMFLOAT3> modelloader_positions;
	vector<XMFLOAT2> modelloader_uvs;
	vector<XMFLOAT3> modelloader_normals;
	vector<PBRVertex> modelloader_vertices;
	vector<UINT> modelloader_indices;
};

