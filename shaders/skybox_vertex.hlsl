
struct VertexInputAttributes
{
    float3 position : POSITION;
    float3 normal : NORMAL0;
    float2 uv : TEXCOORD0;
};

struct VertexOutputAttributes
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL0;
    float2 uv : TEXCOORD0;
    float3 uvcube: TEXCOORD1;
};

struct ModelViewProjection
{
    matrix model;
    matrix view;
    matrix proj;
    float3 eyepos;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

VertexOutputAttributes VSMain(VertexInputAttributes input)
{
    VertexOutputAttributes output;
    output.uvcube = input.position;
    output.position = mul(ModelViewProjectionCB.model, float4(input.position, 1.0f));
    output.position.xyz += ModelViewProjectionCB.eyepos;
   
    output.position = mul(ModelViewProjectionCB.proj, mul(ModelViewProjectionCB.view, output.position));
    output.position = output.position.xyww; //the key of the sky box z/w = 1 so we can simulate the sky dome
    
    output.normal = input.normal;
    output.uv = input.uv;
    return output;
}