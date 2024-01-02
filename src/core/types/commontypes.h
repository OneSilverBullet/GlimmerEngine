#include <DirectXMath.h>

using namespace DirectX;


class DWParam
{
public:
	DWParam(float v): Float(v){}
	DWParam(uint32_t v):Uint(v){}
	DWParam(int v): Int(v){}

	void operator=(float v) { Float = v; }
	void operator=(uint32_t v) { Uint = v; }
	void operator=(int v) { Int = v; }

	union {
		float Float;
		uint32_t Uint;
		int Int;
	};
};

