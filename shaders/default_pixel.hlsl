Texture2D<float4> baseColorTexture : register(t0);
SamplerState baseColorSampler : register(s0);

struct PixelInputAttributes
{
    float4 position : SV_POSITION;
    float4 color : COLOR_OUT;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PixelInputAttributes input) : SV_Target
{
    float3 sampleColor = baseColorTexture.Sample(baseColorSampler, input.uv).rgb;
    return float4(sampleColor, 1.0f);
    //return float4(input.uv, sampleColor.r, 1.0f);
}
