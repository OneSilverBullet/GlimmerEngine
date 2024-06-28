#include "ModelLoader.h"



void ModelLoader::LoadModel(string path)
{
	ifstream file;
	file.open(path.c_str()); //加载model文件

	string line, type, x, y, z;
	float tempU, tempV, intpart;

	while (!file.eof()) {
		getline(file, line);
		istringstream loader(line);
		//先读取当前文件的type
		loader >> type;
		if (type == "v") { //position
			loader >> x >> y >> z;
			//注意：obj模型是左手坐标系，DX是右手坐标系
			XMFLOAT3 temp_pos = XMFLOAT3(stof(x), stof(y), -stof(z));
			modelloader_positions.push_back(temp_pos);
		}
		else if (type == "vn") { //normal
			loader >> x >> y >> z;
			//注意：obj模型是左手坐标系，DX是右手坐标系
			XMFLOAT3 temp_normal = XMFLOAT3(stof(x), stof(y), -stof(z));
			modelloader_normals.push_back(temp_normal);
		}
		else if (type == "vt") //uv
		{
			loader >> x >> y;
			XMFLOAT2 temp_uv = XMFLOAT2(stof(x), 1.0 - stof(y));
			modelloader_uvs.push_back(temp_uv);
		}
		else if (type == "f") {//面
			loader >> x >> y >> z;
			PBRVertex first_vertex = PBRVertexBuilder(x);
			PBRVertex second_vertex = PBRVertexBuilder(y);
			PBRVertex third_vertex = PBRVertexBuilder(z);
			//加载第一个顶点
			modelloader_vertices.push_back(first_vertex);
			UINT current_index = modelloader_vertices.size() - 1;
			modelloader_indices.push_back(current_index);
			//加载第二个顶点
			modelloader_vertices.push_back(second_vertex);
			current_index = modelloader_vertices.size() - 1;
			modelloader_indices.push_back(current_index);
			//加载第三个顶点
			modelloader_vertices.push_back(third_vertex);
			current_index = modelloader_vertices.size() - 1;
			modelloader_indices.push_back(current_index);
		}
	}
	file.clear();
	file.seekg(0, file.beg); //归位
}

PBRVertex ModelLoader::PBRVertexBuilder(string face_frag)
{
	vector<string> vertex_index_str_vec;
	for (int i = 0; i < 2; ++i) {
		int split_point = face_frag.find('/');
		string index_value = face_frag.substr(0, split_point);
		face_frag = face_frag.substr(split_point + 1);
		vertex_index_str_vec.push_back(index_value);
	}
	vertex_index_str_vec.push_back(face_frag);
	//
	vector<int> vertex_index_int_vec;
	for (int i = 0; i < 3; ++i) {
		vertex_index_int_vec.push_back(string_to_int(vertex_index_str_vec[i]));
	}
	PBRVertex res_vertex(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	//注意:索引从1开始
	res_vertex.position = modelloader_positions[vertex_index_int_vec[0] - 1];
	res_vertex.uv = modelloader_uvs[vertex_index_int_vec[1] - 1];
	res_vertex.normal = modelloader_normals[vertex_index_int_vec[2] - 1];
	return res_vertex;
}

BaseMesh<PBRVertex> ModelLoader::MeshBuilder(string path)
{
	LoadModel(path);
	BaseMesh<PBRVertex> res;
	res.indexArray = modelloader_indices;
	res.vertexArray = modelloader_vertices;
	return res;
}

int string_to_int(string value)
{
	int res = 0;
	for (int i = 0; i < value.size(); ++i) {
		res = res * 10 + int(value[i] - '0');
	}
	return res;
}