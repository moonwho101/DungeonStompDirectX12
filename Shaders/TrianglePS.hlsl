// TrianglePS.hlsl
// Pixel shader that outputs the color interpolated from the geometry shader.

struct GS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

float4 main(GS_OUTPUT input) : SV_TARGET
{
    return input.Color;
}
