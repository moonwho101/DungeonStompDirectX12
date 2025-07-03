// RayGeneration.hlsl
#include "Common.hlsl" // Assuming Common.hlsl might have some useful structs or constants later

// Globals
RaytracingAccelerationStructure SceneBVH : register(t0, space0); // t0 for scene BVH
RWTexture2D<float4> RaytracedOutput : register(u0, space0);    // u0 for output texture

// Camera parameters - will be in a constant buffer
cbuffer RayGenConstants : register(b0)
{
    float4x4 CameraToWorld;
    float4x4 ProjectionToWorld; // InverseProjection * InverseView
    float3 CameraPosition;
}

struct RayPayload
{
    float4 color : SV_RayPayload;
    uint recursionDepth : SV_RayPayload;
};

[shader("raygeneration")]
void RayGen()
{
    uint2 dispatchIdx = DispatchRaysIndex().xy;
    uint2 renderTargetSize = DispatchRaysDimensions().xy;

    // Normalized device coordinates (NDC)
    float2 ndc = (float2(dispatchIdx) + 0.5f) / float2(renderTargetSize) * 2.0f - 1.0f;
    ndc.y = -ndc.y; // Invert Y for DirectX coordinate system

    // Create ray
    float4 nearPlane = float4(ndc.x, ndc.y, 0.0f, 1.0f);
    float4 farPlane  = float4(ndc.x, ndc.y, 1.0f, 1.0f);

    float4 worldNear = mul(ProjectionToWorld, nearPlane);
    float4 worldFar  = mul(ProjectionToWorld, farPlane);

    worldNear.xyz /= worldNear.w;
    worldFar.xyz /= worldFar.w;

    RayDesc ray;
    ray.Origin = worldNear.xyz; // Or CameraPosition for perspective
    ray.Direction = normalize(worldFar.xyz - worldNear.xyz);
    ray.TMin = 0.001f;
    ray.TMax = 10000.0f;

    RayPayload payload;
    payload.color = float4(0.0f, 0.0f, 0.0f, 0.0f); // Initialize to black
    payload.recursionDepth = 0;

    TraceRay(
        SceneBVH,                           // AccelerationStructure
        RAY_FLAG_NONE,                      // RayFlags
        0xFF,                               // InstanceInclusionMask
        0,                                  // RayContributionToHitGroupIndex
        1,                                  // MultiplierForGeometryContributionToHitGroupIndex
        0,                                  // MissShaderIndex
        ray,                                // RayDesc
        payload                             // Payload
    );

    RaytracedOutput[dispatchIdx] = payload.color;
}
