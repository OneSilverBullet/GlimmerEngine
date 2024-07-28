#ifndef NON_POWER_OF_TWO
#define NON_POWER_OF_TWO 0
#endif

Texture2D<float4> SrcMip : register(t0);
RWTexture2D<float4> OutMip : register(u0);
SamplerState BilinearClamp : register(s0);

cbuffer CB0 : register(b0)
{
    uint SrcMipLevel;	// Texture level of source mip
    float2 TexelSize;	// 1.0 / OutMip1.Dimensions
}

float3 ApplySRGBCurve(float3 x)
{
    // This is exactly the sRGB curve
    //return x < 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;
     
    // This is cheaper but nearly equivalent
    return x < 0.0031308 ? 12.92 * x : 1.13005 * sqrt(abs(x - 0.00228)) - 0.13448 * x + 0.005719;
}

float4 PackColor(float4 Linear)
{
#ifdef CONVERT_TO_SRGB
    return float4(ApplySRGBCurve(Linear.rgb), Linear.a);
#else
    return Linear;
#endif
}

[numthreads( 8, 8, 1 )]
void CSMain( uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID )
{
#if NON_POWER_OF_TWO == 0
    float2 UV = TexelSize * (DTid.xy + 0.5);
    float4 Src1 = SrcMip.SampleLevel(BilinearClamp, UV, SrcMipLevel);
#elif NON_POWER_OF_TWO == 1
    float2 UV1 = TexelSize * (DTid.xy + float2(0.25, 0.5));
    float2 Off = TexelSize * float2(0.5, 0.0);
    float4 Src1 = 0.5 * (SrcMip.SampleLevel(BilinearClamp, UV1, SrcMipLevel) +
        SrcMip.SampleLevel(BilinearClamp, UV1 + Off, SrcMipLevel));
#elif NON_POWER_OF_TWO == 2
    float2 UV1 = TexelSize * (DTid.xy + float2(0.5, 0.25));
    float2 Off = TexelSize * float2(0.0, 0.5);
    float4 Src1 = 0.5 * (SrcMip.SampleLevel(BilinearClamp, UV1, SrcMipLevel) +
        SrcMip.SampleLevel(BilinearClamp, UV1 + Off, SrcMipLevel));
#elif NON_POWER_OF_TWO == 3
    float2 UV1 = TexelSize * (DTid.xy + float2(0.25, 0.25));
    float2 O = TexelSize * 0.5;
    float4 Src1 = SrcMip.SampleLevel(BilinearClamp, UV1, SrcMipLevel);
    Src1 += SrcMip.SampleLevel(BilinearClamp, UV1 + float2(O.x, 0.0), SrcMipLevel);
    Src1 += SrcMip.SampleLevel(BilinearClamp, UV1 + float2(0.0, O.y), SrcMipLevel);
    Src1 += SrcMip.SampleLevel(BilinearClamp, UV1 + float2(O.x, O.y), SrcMipLevel);
    Src1 *= 0.25;
#endif

    OutMip[DTid.xy] = PackColor(Src1);
}
