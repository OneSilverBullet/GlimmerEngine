#pragma once
#include <DirectXMath.h>

using namespace DirectX;

namespace Mathematics
{

	template<typename T> __forceinline T AlignUpWithMask(T value, size_t mask) {
		return (T)(((size_t)value + mask) & ~mask);
	}

	template <typename T> __forceinline T AlignUp(T value, size_t alignment)
	{
		return AlignUpWithMask(value, alignment - 1);
	}

	template <typename T> __forceinline T DivideByMultiple(T value, size_t alignment)
	{
		return (T)((value + alignment - 1) / alignment);
	}
}


