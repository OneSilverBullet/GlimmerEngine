TextureCube<float4> baseColorTexture : register(t0);
SamplerState baseColorSampler : register(s0);

struct PixelInputAttributes
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL0;
    float2 uv : TEXCOORD0;
    float3 uvcube : TEXCOORD1;
};

float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 PSMain(PixelInputAttributes input) : SV_Target
{
    float3 sampleColor = baseColorTexture.Sample(baseColorSampler, input.uvcube).rgb;
    sampleColor = sampleColor * 5.0f;
    //sampleColor = ACESFilm(sampleColor);
    float3 div = sampleColor + float3(1.0f, 1.0f, 1.0f);
    sampleColor = sampleColor / div;
    sampleColor = pow(sampleColor, 1.0f/2.2f);
    return float4(sampleColor, 1.0f);
}
