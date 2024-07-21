#include "commontypes.hlsli"
#include "pbrmathematics.hlsli"
#include "commondata.hlsli"

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
float3 DirectLighting(FragmentProperties frag, float3 L, float3 lightcolor)
{
	LightProperties d_light;
	d_light.L = L;
	d_light.H = normalize(L + frag.V);
	d_light.NdotL = saturate(dot(frag.N, L));
    d_light.LdotH = saturate(dot(L, H));
    d_light.NdotH = saturate(dot(frag.N, H));
	
	float3 diffuse = Diffuse_Burley(frag, d_light);
	float3 specular = Specular_BRDF(frag, d_light);
	
	return d_light.NdotL * lightcolor * (diffuse + specular);
}

//the indirect lighting calculation
float3 IndirectLighting()
{
	return float3(0.0f, 0.0f, 0.0f);
}

float4 PSMain(PixelInputAttributes input) : SV_Target
{	
	//loading fragment PBR values
	float3 albedo = baseColorTexture.Sample(baseColorSampler, input.uv).rgb;
	float3 normal = normalTexture.Sample(normalSampler, input.uv).rgb;
	float3 metalness = roughnessTexture.Sample(metalnessSampler, input.uv).rgb;
	float3 roughness = metalnessTexture.Sample(roughnessSampler, input.uv).rgb;
	float3 ao = aoTexture.Sample(aoSampler, input.uv).rgb;
	
	//initialize the fragment information
	FragmentProperties frag;
	frag.N = normal;
	frag.V = normalize(CommonCB.eyepos - input.worldposition);
	frag.NdotV = saturate(dot(Surface.N, Surface.V));
	frag.roughness = roughness;
	frag.r2 = roughness * roughness;
	frag.r4 = frag.r2 * frag.r2;
	frag.diffuse = albedo * (1 - kDielectricSpecular) * (1 - kDielectricSpecular) * ao;
	frag.specular = lerp(kDielectricSpecular, albedo.rgb, metalness.x) * ao;
	
	//calculate the direct lighting
	float3 directLightingRes = DirectLighting(frag, CommonCB.sundirection, CommonCB.sunintensity);
	
	//todo: calculate the indirect lighting
	float3 indirectLightingRes = IndirectLighting();
	
	
	float3 lightingRes = directLightingRes + indirectLightingRes;
	
	
    return float4(lightingRes, 1.0f);
}
