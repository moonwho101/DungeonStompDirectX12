// RayGen.hlsl
// Ray Generation Shader

// Output texture (UAV)
RWTexture2D<float4> gOutputTexture : register(u0);

// Top-Level Acceleration Structure (SRV)
RaytracingAccelerationStructure gScene : register(t0);

struct RayPayload
{
    float4 color; // Raytracing Tier 1.0 / 1.1 does not use SV_Target in payload for TraceRay
    // bool hit; // Example: if you want to know if anything was hit
};

// Ray Generation Shader
[shader("raygeneration")]
void RayGen()
{
    // Get the dispatch (thread) index and image dimensions
    uint2 dispatchIdx = DispatchRaysIndex().xy;
    uint2 dispatchDim = DispatchRaysDimensions().xy;

    // Normalize dispatch coordinates to [0, 1]
    float2 normalizedCoords = (dispatchIdx + 0.5f) / dispatchDim;

    // Convert to screen space coordinates [-1, 1]
    // For a typical perspective projection, you'd derive this from camera's view/proj matrices
    float2 screenSpaceCoords = normalizedCoords * 2.0f - 1.0f;

    // Simple orthographic projection for now:
    // Ray origin: Varies with pixel, Z is some distance in front of the view plane
    // Ray direction: Constant, pointing into the scene

    // For a simple setup matching screen space to world space for XY at a certain Z
    float3 origin = float3(screenSpaceCoords.x, -screenSpaceCoords.y, -1.0f); // Y is often flipped
    float3 direction = float3(0.0f, 0.0f, 1.0f); // Pointing along positive Z

    // Define ray parameters
    float tMin = 0.001f;
    float tMax = 10000.0f;

    RayDesc ray = { origin, tMin, direction, tMax };

    RayPayload payload;
    payload.color = float4(0.0f, 0.0f, 0.0f, 1.0f); // Default color (e.g., clear color)
    // payload.hit = false;

    // Trace the ray
    // InstanceInclusionMask: 0xFF to include all instances
    // RayContributionToHitGroupIndex: Offset into hit group records in SBT (0 for now)
    // MultiplierForGeometryContributionToShaderIndex: Offset for geometry within instance (0 for now)
    // MissShaderIndex: Index of miss shader in SBT (0 for now)
    TraceRay(
        gScene,                             // AccelerationStructure
        RAY_FLAG_NONE,                      // RayFlags (e.g. RAY_FLAG_CULL_BACK_FACING_TRIANGLES)
        0xFF,                               // InstanceInclusionMask
        0,                                  // RayContributionToHitGroupIndex
        0,                                  // MultiplierForGeometryContributionToShaderIndex
        0,                                  // MissShaderIndex
        ray,                                // Ray
        payload                             // Payload
    );

    // Write the result to the output texture
    gOutputTexture[dispatchIdx] = payload.color;
}
