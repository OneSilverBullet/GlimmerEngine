#include "renderitem.h"

#include "graphicscore.h"

void RenderItem::Initialize(const std::string& objName) {
	//assign the model reference
	m_modelRef = GRAPHICS_CORE::g_staticModelsManager.GetModelRef(objName);

	//get the material 
	int subMeshSize = (int)m_modelRef.GetMeshCount();
	for (int subMeshIndex = 0; subMeshIndex < subMeshSize; ++subMeshIndex) {
		Material* subMeshMaterial = GRAPHICS_CORE::g_materialManager.GetMaterial(objName, std::to_string(subMeshIndex));
		m_materials.push_back(subMeshMaterial);
	}
}
