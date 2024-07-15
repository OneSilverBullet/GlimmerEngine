TextureCube<float4> baseColorTexture : register(t0);
SamplerState baseColorSampler : register(s0);

struct PixelInputAttributes
{
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PixelInputAttributes input) : SV_Target
{
    float3 sampleColor = float3(1.0f, 1.0f, 1.0f);
    return float4(sampleColor, 1.0f);
}
