#pragma once
#include "resources/gpubuffer.h"

class ReadbackBuffer : public GPUBuffer {
public:
	virtual ~ReadbackBuffer() { Destroy(); }

	void Create(const std::wstring& name, uint32_t numElements, uint32_t elementSize);

	void* Map();
	void Unmap();

protected:
	void CreateDerivedViews(){}
};


