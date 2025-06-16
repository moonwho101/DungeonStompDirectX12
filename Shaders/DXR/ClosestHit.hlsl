// ClosestHit.hlsl
// Basic Closest Hit Shader

struct RayPayload
{
    float4 color : SV_Target;
};

struct HitInfo
{
    float hitT : SV_HitT;
    // Other hit information like barycentrics can be added here
};

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in HitInfo hitInfo)
{
    // For a placeholder, let's output a green color upon hitting something.
    payload.color = float4(0.1, 0.8, 0.2, 1.0);
}
