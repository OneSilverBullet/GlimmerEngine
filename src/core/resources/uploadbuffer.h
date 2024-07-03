#pragma once
#include "gpuresource.h"

class UploadBuffer : public GPUResource
{
public:
	virtual ~UploadBuffer() { Destroy(); }

	void Create(const std::wstring& name, size_t bufferSize);
	void* Map();
	void Unmap(size_t begin = 0, size_t end = -1);

	size_t GetBufferSize() const { return m_bufferSize; }

private:
	size_t m_bufferSize;
};