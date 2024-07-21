#include "commontypes.hlsli"
#include "commondata.hlsli"

float Pow5(float x)
{
    float xSq = x * x;
    return xSq * xSq * x;
}

float3 Fresnel_Shlick(float3 F0, float3 F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}

float Fresnel_Shlick(float F0, float F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}

// Burley's diffuse BRDF
float3 Diffuse_Burley(FragmentProperties Surface, LightProperties Light)
{
    float fd90 = 0.5 + 2.0 * Surface.roughness * Light.LdotH * Light.LdotH;
    return Surface.diffuse * Fresnel_Shlick(1, fd90, Light.NdotL).x * Fresnel_Shlick(1, fd90, Surface.NdotV).x;
}

// GGX specular D (normal distribution)
float DistributionGGX(FragmentProperties Surface, LightProperties Light)
{
    float a = Surface.r2;
    float a2 = Surface.r4;
    float NdotH = Light.NdotH;
    float NdotH2 = NdotH * NdotH;
	
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}


float GeometrySmith(FragmentProperties Surface, LightProperties Light)
{
    float NdotV = max(Surface.NdotV, 0.0f);
    float NdotL = max(Light.NdotL, 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, Surface.roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, Surface.roughness);
    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

//microfacet based BRDF
float3 SpecularBRDF(FragmentProperties Surface, LightProperties Light, float3 albedo, float3 metallic)
{
    float3 N = Surface.N;
    float3 V = Surface.V;

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    float3 F0 = kDielectricSpecular;
    F0 = lerp(F0, albedo, metallic);

    // reflectance equation
    float3 specRes = float3(0.0f, 0.0f, 0.0f);
    
    
    float3 L = Light.L;
    float3 H = normalize(V + L);
    
   
    float3 radiance = float3(3.0f, 3.0f, 3.0f);

        // Cook-Torrance BRDF
    float NDF = DistributionGGX(Surface, Light);
    float G = GeometrySmith(Surface, Light);
    float3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
           
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    float3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
    float3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
    kD *= 1.0 - metallic;

        // scale light by NdotL
    float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
    specRes += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    return specRes;

}
