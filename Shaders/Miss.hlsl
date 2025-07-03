// Miss.hlsl
struct RayPayload
{
    float4 color : SV_RayPayload;
    uint recursionDepth : SV_RayPayload;
};

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = float4(0.2f, 0.3f, 0.5f, 1.0f); // Blueish background
}
