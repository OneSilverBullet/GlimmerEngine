

struct VertexInputAttributes
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VertexOutputAttributes
{
    float3 position : SV_POSITION;
    float4 color : COLOR;
};

struct PixelInputAttributes
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

struct ModelViewProjection
{
    matrix MVP;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

VertexOutputAttributes VSMain(VertexInputAttributes input)
{
    VertexOutputAttributes output;

    output.position = mul(float4(input.position, 1.0f), ModelViewProjectionCB.MVP).xyz;
    output.color = float4(input.color, 1.0f);

    return output;
}


float4 PSMain(PixelInputAttributes input):SV_Target
{
    return input.color;
}
