#pragma once

#include <string>
#include <vector>
#include "vertexformat.h"


class DefaultGeometry
{
public:
	static void DefaultSphereMesh(float radius, std::vector<BaseVertex>& outputVertices, std::vector<DWORD>& outputIndices);
	static void DefaultSphereMesh(float radius, std::vector<PBRVertex>& outputVertices, std::vector<DWORD>& outputIndices);
	static void DefaultSphereMesh(float radius, std::vector<GeometryVertex>& outputVertices, std::vector<DWORD>& outputIndices);
	static void DefaultBoxMesh(float length, std::vector<BaseVertex>& outputVertices, std::vector<DWORD>& outputIndices);
	static void DefaultBoxMesh(float length, std::vector<PBRVertex>& outputVertices, std::vector<DWORD>& outputIndices);
	static void DefaultPlaneMesh(float width, float height, float level, float texture_loop_num, 
		std::vector<PBRVertex>& outputVertices, 
		std::vector<DWORD>& outputIndices);
	static void DefaultCylinderMesh(float radius, float thickness,
		std::vector<PBRVertex>& outputVertices, std::vector<DWORD>& outputIndices);

};