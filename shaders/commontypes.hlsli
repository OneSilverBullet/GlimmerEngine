
struct CommonInfo
{
    matrix model;
    matrix view;
    matrix proj;
    float3 eyepos;
	float3 sundirection;
	float3 sunintensity;
	float2 iblparameters;
};

struct MaterialInfo
{
	float4 baseColorFactor;
	float normalTextureScale;
	float2 metalRoughnessFactor;
	uint flags;
};

struct FragmentProperties
{
	float3 N;
	float3 V;
	float NdotV;
	float roughness;
	float r2;
	float r4;
	float3 diffuse;
	float3 specular;
};

struct LightProperties
{
    float3 L;
    float3 H;
	float NdotL;
	float LdotH;
	float NdotH;
};

