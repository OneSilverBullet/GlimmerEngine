#include "commontypes.hlsli"

struct VertexInputAttributes
{
    float3 position : POSITION;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
};

struct VertexOutputAttributes
{
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
	float3 worldposition : TEXCOORD1;
};

ConstantBuffer<CommonInfo> CommonCB : register(b0);

VertexOutputAttributes VSMain(VertexInputAttributes input)
{
    VertexOutputAttributes output;
	output.worldposition = mul(CommonCB.model,
    float4(input.position, 1.0f)).xyz;
    output.position = mul(CommonCB.proj,
    mul(CommonCB.view, float4(output.worldposition, 1.0f)));
    output.normal = input.normal;
    output.tangent = input.tangent;
    output.uv = input.uv;
    return output;
}