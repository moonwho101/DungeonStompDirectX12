// Miss.hlsl
// Basic Miss Shader

struct RayPayload
{
    float4 color : SV_Target;
};

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = float4(0.1, 0.1, 0.1, 1.0); // Output a dark gray for misses
}
