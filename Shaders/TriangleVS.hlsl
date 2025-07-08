// TriangleVS.hlsl
// Minimal vertex shader to provide a point to the geometry shader.

struct VS_INPUT
{
    float3 Pos : POSITION; // Dummy input, could be anything if not used.
                           // We'll need to provide one vertex with this attribute from the C++ side.
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION; // Output to Geometry Shader
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    // We don't really care about the input position from the CPU for this specific GS,
    // as the GS generates the triangle in NDC.
    // However, we must output SV_POSITION.
    // Let's just output a point at the origin in model space,
    // assuming it will be transformed by WVP matrix if one is bound,
    // or simply passed through if no transformation is expected before GS.
    // For simplicity with a GS that generates in NDC, this position isn't critical.
    // We can also just use the input position directly.
    output.Pos = float4(input.Pos, 1.0f);
    return output;
}
