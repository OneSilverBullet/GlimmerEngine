#include "mesh.h"
#include "defaultgeometry.h"

using namespace DirectX;

void DefaultGeometry::DefaultSphereMesh(float radius, std::vector<BaseVertex>& outputVertices, std::vector<DWORD>& outputIndices)
{
	int x_segment_num = 16;
	int y_segment_num = 16;
	float pi = 3.14159265359;
	//create vertices
	for (int y = 0; y <= y_segment_num; ++y)
	{
		for (int x = 0; x <= x_segment_num; ++x)
		{
			float yaw = ((float)x / (float)x_segment_num) * 2.0f * pi; //longtitude
			float pitch = ((float)y / (float)y_segment_num) * pi; //latitude
			float xpos = cos(yaw) * sin(pitch) * radius;
			float ypos = cos(pitch) * radius;
			float zpos = sin(yaw) * sin(pitch) * radius;
			outputVertices.push_back(BaseVertex(xpos, ypos, zpos));
		}
	}
	//create indices
	bool odd = false;
	for (int y = 0; y < y_segment_num; ++y)
	{
		if (!odd) {
			for (int x = 0; x <= x_segment_num; ++x)
			{
				outputIndices.push_back(y * (x_segment_num + 1) + x);
				outputIndices.push_back((y + 1) * (x_segment_num + 1) + x);
			}
		}
		else {
			for (int x = x_segment_num; x >= 0; --x) {
				outputIndices.push_back((y + 1) * (x_segment_num + 1) + x);
				outputIndices.push_back(y * (x_segment_num + 1) + x);

			}
		}
		odd = !odd;
	}
}

void DefaultGeometry::DefaultSphereMesh(float radius, std::vector<PBRVertex>& outputVertices, std::vector<DWORD>& outputIndices)
{

	int x_segment_num = 16;
	int y_segment_num = 16;
	float pi = 3.14159265359;
	//创建vertex
	for (int y = 0; y <= y_segment_num; ++y)
	{
		for (int x = 0; x <= x_segment_num; ++x)
		{
			float x_segment = (float)x / (float)x_segment_num;
			float y_segment = (float)y / (float)y_segment_num;
			float yaw = x_segment * 2.0f * pi; //球体纬度360
			float pitch = y_segment * pi; //球体精度180
			float xpos = cos(yaw) * sin(pitch) * radius;
			float ypos = cos(pitch) * radius;
			float zpos = sin(yaw) * sin(pitch) * radius;
			//position , uv, normal
			PBRVertex pbrVertex(XMFLOAT3(xpos, ypos, zpos), XMFLOAT2(x_segment, y_segment), XMFLOAT3(xpos, ypos, zpos));
			outputVertices.push_back(pbrVertex);
		}
	}
	//创建indices
	bool odd = false;
	for (int y = 0; y < y_segment_num; ++y)
	{
		if (!odd) {
			for (int x = 0; x <= x_segment_num; ++x)
			{
				outputIndices.push_back(y * (x_segment_num + 1) + x);
				outputIndices.push_back((y + 1) * (x_segment_num + 1) + x);
			}
		}
		else {
			for (int x = x_segment_num; x >= 0; --x) {
				outputIndices.push_back((y + 1) * (x_segment_num + 1) + x);
				outputIndices.push_back(y * (x_segment_num + 1) + x);

			}
		}
		odd = !odd;
	}
}

void DefaultGeometry::DefaultBoxMesh(float length, std::vector<BaseVertex>& outputVertices, std::vector<DWORD>& outputIndices)
{
	outputVertices = { 
	{ BaseVertex(-length, length, -length) },
	{ BaseVertex(length, length, -length) },
	{ BaseVertex(length, length, length) },
	{ BaseVertex(-length, length, length) },
	{ BaseVertex(-length, -length, -length)},
	{ BaseVertex(length, -length, -length) },
	{ BaseVertex(length, -length, length)},
	{ BaseVertex(-length, -length, length)},
	{ BaseVertex(-length, -length, length)},
	{ BaseVertex(-length, -length, -length)},
	{ BaseVertex(-length, length, -length)},
	{ BaseVertex(-length, length, length)},
	{ BaseVertex(length, -length, length)},
	{ BaseVertex(length, -length, -length)},
	{ BaseVertex(length, length, -length) },
	{ BaseVertex(length, length, length) },
	{ BaseVertex(-length, -length, -length)},
	{ BaseVertex(length, -length, -length)},
	{ BaseVertex(length, length, -length) },
	{ BaseVertex(-length, length, -length)},
	{ BaseVertex(-length, -length, length) },
	{ BaseVertex(length, -length, length)},
	{ BaseVertex(length, length, length) },
	{ BaseVertex(-length, length, length)},
	};
	outputIndices = { 3, 1, 0, 2, 1, 3, 6, 4, 5, 7, 4, 6, 11, 9, 8, 10, 9, 11, 14, 12, 13, 15, 12, 14, 19, 17, 16, 18, 17, 19, 22, 20, 21, 23, 20, 22 };
}

void DefaultGeometry::DefaultBoxMesh(float length, std::vector<PBRVertex>& outputVertices, std::vector<DWORD>& outputIndices) 
{
	outputVertices = {
		// 0 1 0
		PBRVertex(XMFLOAT3(-length, length, -length), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0, 1.0, 0.0)),
		PBRVertex(XMFLOAT3(length, length, -length), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0, 1.0, 0.0)),
		PBRVertex(XMFLOAT3(length, length, length), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0, 1.0, 0.0)),
		PBRVertex(XMFLOAT3(-length, length, length), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0, 1.0, 0.0)),
		//0 -1 0
		PBRVertex(XMFLOAT3(-length, -length, -length), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0, -1.0, 0.0)),
		PBRVertex(XMFLOAT3(length, -length, -length), XMFLOAT2(1.0f, 0.0f),XMFLOAT3(0.0, -1.0, 0.0)),
		PBRVertex(XMFLOAT3(length, -length, length), XMFLOAT2(1.0f, 1.0f),XMFLOAT3(0.0, -1.0, 0.0)),
		PBRVertex(XMFLOAT3(-length, -length, length), XMFLOAT2(0.0f, 1.0f),XMFLOAT3(0.0, -1.0, 0.0)),
		//-1 0 0
		PBRVertex(XMFLOAT3(-length, -length, length), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(-1.0, 0.0, 0.0)),
		PBRVertex(XMFLOAT3(-length, -length, -length), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(-1.0, 0.0, 0.0)),
		PBRVertex(XMFLOAT3(-length, length, -length), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(-1.0, 0.0, 0.0)),
		PBRVertex(XMFLOAT3(-length, length, length), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(-1.0, 0.0, 0.0)),
		//1 0 0
		PBRVertex(XMFLOAT3(length, -length, length), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(1.0, 0.0, 0.0)),
		PBRVertex(XMFLOAT3(length, -length, -length), XMFLOAT2(1.0f, 0.0f) , XMFLOAT3(1.0, 0.0, 0.0)),
		PBRVertex(XMFLOAT3(length, length, -length), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(1.0, 0.0, 0.0)),
		PBRVertex(XMFLOAT3(length, length, length), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(1.0, 0.0, 0.0)),
		//0 0 -1
		PBRVertex(XMFLOAT3(-length, -length, -length), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0, 0.0, -1.0)),
		PBRVertex(XMFLOAT3(length, -length, -length), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0, 0.0, -1.0)),
		PBRVertex(XMFLOAT3(length, length, -length), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0, 0.0, -1.0)),
		PBRVertex(XMFLOAT3(-length, length, -length), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0, 0.0, -1.0)),
		//0 0 1
		PBRVertex(XMFLOAT3(-length, -length, length), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0, 0.0, 1.0)),
		PBRVertex(XMFLOAT3(length, -length, length), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0, 0.0, 1.0)),
		PBRVertex(XMFLOAT3(length, length, length), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0, 0.0, 1.0)),
		PBRVertex(XMFLOAT3(-length, length, length), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0, 0.0, 1.0)),
	};
	outputIndices = { 3, 1, 0, 2, 1, 3, 6, 4, 5, 7, 4, 6, 11, 9, 8, 10, 9, 11, 14, 12, 13, 15, 12, 14, 19, 17, 16, 18, 17, 19, 22, 20, 21, 23, 20, 22 };
}


void DefaultGeometry::DefaultPlaneMesh(float width, float height, float level, float texture_loop_num,
	std::vector<PBRVertex>& outputVertices,
	std::vector<DWORD>& outputIndices) {
	float half_width = width / 2.0;
	float half_height = height / 2.0;

	//vertices: position uv normal
	outputVertices = {
		PBRVertex(XMFLOAT3(-half_width, level, -half_width), XMFLOAT2(0.0f, 0.0f),  XMFLOAT3(0.0f, 1.0f, 1.0)),
		PBRVertex(XMFLOAT3(half_width, level, -half_width), XMFLOAT2(texture_loop_num, 0.0f),  XMFLOAT3(0.0f, 1.0f, 1.0)),
		PBRVertex(XMFLOAT3(half_width, level, half_width), XMFLOAT2(texture_loop_num, texture_loop_num),  XMFLOAT3(0.0f, 1.0f, 1.0)),
		PBRVertex(XMFLOAT3(-half_width, level, half_width), XMFLOAT2(0.0f, texture_loop_num),  XMFLOAT3(0.0f, 1.0f, 1.0)),
	};

	//indices
	outputIndices = { 3, 1, 0, 2, 1, 3 };
}


void DefaultGeometry::DefaultCylinderMesh(float radius, float thickness, 
	std::vector<PBRVertex>& outputVertices,
	std::vector<DWORD>& outputIndices) {
	int slices = 12;
	float radiance = 3.14159 / 180.0f;
	XMFLOAT3 origin = XMFLOAT3(0.0, 0.0, 0.0); //start point 
	//the first circle point
	XMFLOAT3 origin_point1 = XMFLOAT3(thickness / 2.0, 0.0, 0.0);
	//the second circle point
	XMFLOAT3 origin_point2 = XMFLOAT3(-thickness / 2.0, 0.0, 0.0);
	PBRVertex origin_basevertex1(origin_point1, XMFLOAT2(0.5f, 0.5f), XMFLOAT3(1.0, 0.0, 0.0));
	outputVertices.push_back(origin_basevertex1);
	float angleDelta = 360.0 / slices;
	float currentAngle = 0.0f;
	for (int i = 0; i < slices; ++i) {
		float ypos = cos(currentAngle * radiance) * radius;
		float zpos = sin(currentAngle * radiance) * radius;
		float xpos = thickness / 2.0;
		float u = (zpos + radius) / (2 * radius);
		float v = (radius - ypos) / (2 * radius);
		PBRVertex singleVertex(xpos, ypos, zpos, u, v, 1.0, 0.0, 0.0);
		outputVertices.push_back(singleVertex);
		currentAngle += angleDelta;
	}
	for (int i = 1; i <= slices; ++i) {
		int origin_index = 0;
		int first_index = i;
		int second_index = (i == slices) ? 1 : i + 1;
		outputIndices.push_back(origin_index);
		outputIndices.push_back(first_index);
		outputIndices.push_back(second_index);
	}

	PBRVertex origin_basevertex2(origin_point2, XMFLOAT2(0.5f, 0.5f), XMFLOAT3(-1.0, 0.0, 0.0));
	outputVertices.push_back(origin_basevertex2);
	for (int i = 0; i < slices; ++i) {
		float ypos = cos(currentAngle * radiance) * radius;
		float zpos = sin(currentAngle * radiance) * radius;
		float xpos = -thickness / 2.0;
		float u = (zpos + radius) / (2 * radius);
		float v = (radius - ypos) / (2 * radius);
		PBRVertex singleVertex(xpos, ypos, zpos, u, v, -1.0, 0.0, 0.0);
		outputVertices.push_back(singleVertex);
		currentAngle += angleDelta;
	}

	for (int i = 1; i <= slices; ++i) {
		int origin_index = 0;
		int first_index = i;
		int second_index = (i == slices) ? 1 : i + 1;
		outputIndices.push_back(origin_index + slices + 1);
		outputIndices.push_back(first_index + slices + 1);
		outputIndices.push_back(second_index + slices + 1);
	}

	for (int i = 1; i <= slices; ++i) {
		outputIndices.push_back(i);
		outputIndices.push_back(i + slices + 1);
		outputIndices.push_back((i == slices) ? (slices + 2) : (i + slices + 2));
		outputIndices.push_back(i);
		outputIndices.push_back((i == slices) ? (slices + 2) : (i + slices + 2));
		outputIndices.push_back((i == slices) ? (1) : (i + 1));
	}
}
