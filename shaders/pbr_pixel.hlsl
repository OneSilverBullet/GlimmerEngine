#include "pbrmathematics.hlsli"


//the pbr texture properties
Texture2D<float3> baseColorTexture : register(t0);
Texture2D<float3> normalTexture : register(t1);
Texture2D<float3> roughnessTexture : register(t2);
Texture2D<float3> metalnessTexture : register(t3);
Texture2D<float3> aoTexture : register(t4);

SamplerState baseColorSampler : register(s0);
SamplerState normalSampler : register(s1);
SamplerState metalnessSampler : register(s2);
SamplerState roughnessSampler : register(s3);
SamplerState aoSampler : register(s4);

//common parameters
ConstantBuffer<CommonInfo> CommonCB : register(b0);

struct PixelInputAttributes
{
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
	float3 worldposition : TEXCOORD1;
};


//the direct lighting calculation
float3 DirectLighting(FragmentProperties frag, float3 L, float3 lightcolor, float3 albedo, float3 metal)
{
	LightProperties d_light;
	d_light.L = L;
	d_light.H = normalize(L + frag.V);
	d_light.NdotL = saturate(dot(frag.N, L));
    d_light.LdotH = saturate(dot(L, d_light.H));
    d_light.NdotH = saturate(dot(frag.N, d_light.H));
	
    float3 diffuse = d_light.NdotL * float3(1.0f, 1.0f, 1.0f) * frag.diffuse;
    float3 specular = SpecularBRDF(frag, d_light, albedo, metal);
	
    return (diffuse + specular);
}

//the indirect lighting calculation
float3 IndirectLighting()
{
	return float3(0.0f, 0.0f, 0.0f);
}

float3 ComputeNormal(PixelInputAttributes vsOutput, float3 rawnormal)
{
    float3 normal = normalize(vsOutput.normal);
    float3 tangent = normalize(vsOutput.tangent.xyz);
    float3 bitangent = normalize(cross(normal, tangent));
    float3x3 tangentFrame = float3x3(tangent, bitangent, normal);

    // Read normal map and convert to SNORM (TODO:  convert all normal maps to R8G8B8A8_SNORM?)
    normal = rawnormal * 2.0 - 1.0;
	
    return mul(normal, tangentFrame);
}


float4 PSMain(PixelInputAttributes input) : SV_Target
{	
	//loading fragment PBR values
	float3 albedo = baseColorTexture.Sample(baseColorSampler, input.uv).rgb;
	float3 normal = normalTexture.Sample(normalSampler, input.uv).rgb;
    normal = ComputeNormal(input, normal);
	float3 metalness = roughnessTexture.Sample(metalnessSampler, input.uv).rgb;
	float3 roughness = metalnessTexture.Sample(roughnessSampler, input.uv).rgb;
	float3 ao = aoTexture.Sample(aoSampler, input.uv).rgb;
	
	//initialize the fragment information
	FragmentProperties frag;
	frag.N = normal;
	frag.V = normalize(CommonCB.eyepos - input.worldposition);	
    frag.NdotV = saturate(dot(frag.N, frag.V));
	frag.roughness = roughness.x;
    frag.r2 = frag.roughness * frag.roughness;
	frag.r4 = frag.r2 * frag.r2;
	frag.diffuse = albedo * (1 - kDielectricSpecular) * (1 - kDielectricSpecular) * ao;
	frag.specular = lerp(kDielectricSpecular, albedo.rgb, metalness.x) * ao;
	
	
    float3 L = normalize(CommonCB.sundirection - input.position.xyz);
	
    float3 directLightingRes = DirectLighting(frag, L, CommonCB.sunintensity, albedo, metalness);
	
    return float4(directLightingRes, 1.0f);
	
	/*
	//calculate the direct lighting
	
	
	//todo: calculate the indirect lighting
	float3 indirectLightingRes = IndirectLighting();
	
	
	float3 lightingRes = directLightingRes + indirectLightingRes;
	
	
    return float4(lightingRes, 1.0f);
	*/
}
