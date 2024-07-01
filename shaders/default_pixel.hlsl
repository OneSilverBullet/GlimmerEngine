TextureCube<float4> baseColorTexture : register(t0);
SamplerState baseColorSampler : register(s0);

struct PixelInputAttributes
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL0;
    float2 uv : TEXCOORD0;
    float3 uvcube : TEXCOORD1;
};

float4 PSMain(PixelInputAttributes input) : SV_Target
{
    float3 sampleColor = baseColorTexture.Sample(baseColorSampler, input.uvcube).rgb;
    return float4(sampleColor, 1.0f);
}
