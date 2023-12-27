#include <DirectXMath.h>

using namespace DirectX;

namespace Mathematics
{
	template<typename T> __forceinline T AlignUpWithMask(T value, size_t mask) {
		return (T)(((size_t)value + mask) & ~mask);
	}




}


