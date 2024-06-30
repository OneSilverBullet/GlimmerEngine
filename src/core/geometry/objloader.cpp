#include "objloader.h"


int StringToInt(std::string value)
{
	int res = 0;
	for (int i = 0; i < value.size(); ++i) {
		res = res * 10 + int(value[i] - '0');
	}
	return res;
}

PBRVertex ObjModelLoader::OBJVertexBuilder(std::string faceFrag,
	std::vector<DirectX::XMFLOAT3>& position, 
	std::vector<DirectX::XMFLOAT2>& uvs,
	std::vector<DirectX::XMFLOAT3>& normals)
{
	std::vector<std::string> vertexIndexStrVec;
	for (int i = 0; i < 2; ++i) {
		int splitPoint = faceFrag.find('/');
		std::string indexValue = faceFrag.substr(0, splitPoint);
		faceFrag = faceFrag.substr(splitPoint + 1);
		vertexIndexStrVec.push_back(indexValue);
	}
	vertexIndexStrVec.push_back(faceFrag);

	std::vector<int> vertexIndexIntVec;
	for (int i = 0; i < 3; ++i) {
		vertexIndexIntVec.push_back(StringToInt(vertexIndexStrVec[i]));
	}

	PBRVertex vertexInstance(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

	vertexInstance.position = position[vertexIndexIntVec[0] - 1];
	vertexInstance.uv = uvs[vertexIndexIntVec[1] - 1];
	vertexInstance.normal = normals[vertexIndexIntVec[1] - 1];
	return vertexInstance;
}

void ObjModelLoader::LoadModel(std::string path, 
	std::vector<PBRVertex>& outputVertices,
	std::vector<DWORD>& outputIndices)
{
	std::ifstream file;
	file.open(path.c_str()); //loading the model in obj type

	std::string line, type, x, y, z;
	float tempU, tempV, intpart;

	std::vector<DirectX::XMFLOAT3> positions;
	std::vector<DirectX::XMFLOAT3> normals;
	std::vector<DirectX::XMFLOAT2> uvs;

	while (!file.eof()) {
		getline(file, line);
		std::istringstream loader(line);
		loader >> type;
		if (type == "v") { //position
			loader >> x >> y >> z;
			//Attention: the obj model has the left-hand coordination system
			//dx is the right-hand coordination system
			DirectX::XMFLOAT3 tempPos = DirectX::XMFLOAT3(stof(x), stof(y), -stof(z));
			positions.push_back(tempPos);
		}
		else if (type == "vn") { //normal
			loader >> x >> y >> z;
			//Attention: the obj model has the left-hand coordination system
			//dx is the right-hand coordination system
			DirectX::XMFLOAT3 tempNormal = DirectX::XMFLOAT3(stof(x), stof(y), -stof(z));
			normals.push_back(tempNormal);
		}
		else if (type == "vt") //uv
		{
			loader >> x >> y;
			DirectX::XMFLOAT2 tempUV = DirectX::XMFLOAT2(stof(x), 1.0 - stof(y));
			uvs.push_back(tempUV);
		}
		else if (type == "f") {//face
			loader >> x >> y >> z;
			PBRVertex firstVertex = ObjModelLoader::OBJVertexBuilder(x, positions, uvs, normals);
			PBRVertex secondVertex = ObjModelLoader::OBJVertexBuilder(y, positions, uvs, normals);
			PBRVertex thirdVertex = ObjModelLoader::OBJVertexBuilder(z, positions, uvs, normals);
			//loading the first vertex
			outputVertices.push_back(firstVertex);
			size_t curIndex = outputVertices.size() - 1;
			outputIndices.push_back(curIndex);
			//loading the second vertex
			outputVertices.push_back(secondVertex);
			curIndex = outputVertices.size() - 1;
			outputIndices.push_back(curIndex);
			//loading the third vertex
			outputVertices.push_back(thirdVertex);
			curIndex = outputVertices.size() - 1;
			outputIndices.push_back(curIndex);
		}
	}
	file.clear();
	file.seekg(0, file.beg); 
}
