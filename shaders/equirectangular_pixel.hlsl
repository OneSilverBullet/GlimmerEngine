Texture2D<float3> equirectangularMap : register(t0);
SamplerState baseColorSampler : register(s0);

struct PixelInputAttributes
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL0;
    float2 uv : TEXCOORD0;
    float3 localpos : TEXCOORD1;
};

float2 SampleSphericalMap(float3 v) {
    float2 invAtan = float2(0.1591f, 0.3183f);
    float2 uv = float2(atan2(v.z, v.x), -asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}


float4 PSMain(PixelInputAttributes input) : SV_Target
{
    float2 uv = SampleSphericalMap(normalize(input.localpos.xyz));
    float3 sampleColor = equirectangularMap.Sample(baseColorSampler, uv).rgb;
    float3 div = sampleColor + float3(1.0f, 1.0f, 1.0f);
    sampleColor = sampleColor / div;
    sampleColor = pow(sampleColor, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));
    return float4(sampleColor, 1.0f);
}
