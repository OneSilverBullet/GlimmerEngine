
struct PixelInputAttributes
{
    float4 position : SV_POSITION;
    float4 color : COLOR_OUT;
};

float4 PSMain(PixelInputAttributes input) : SV_Target
{
    return input.color;
}
