
struct VertexInputAttributes
{
    float3 position : POSITION;
    float3 color : COLOR_IN;
};

struct VertexOutputAttributes
{
    float4 position : SV_POSITION;
    float4 color : COLOR_OUT;
};

struct ModelViewProjection
{
    matrix MVP;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

VertexOutputAttributes VSMain(VertexInputAttributes input)
{
    VertexOutputAttributes output;
    
    output.position = mul(ModelViewProjectionCB.MVP, float4(input.position, 1.0f)); 
    output.color = float4(input.color, 1.0f);

    return output;
}