// Raytracing.hlsl

// Output texture for the Ray Generation Shader
RWTexture2D<float4> outputTexture : register(u0);

// Top-Level Acceleration Structure (TLAS)
RaytracingAccelerationStructure gScene : register(t0);

// Pass Constants (subset matching C++ PassConstants for DXR)
cbuffer PassConstants : register(b0)
{
    float4x4 gInvView;
    float4x4 gInvProj;
    float3 gEyePosW;
    // float cbPerObjectPad1; // Not needed for RGS
    float2 gRenderTargetSize;
    // Other fields omitted for brevity if not used by RGS
};

// Payload definition
struct HitInfo
{
    float4 color : SV_RayPayload;
    // bool hit; // Could be added later
};

// Ray Generation Shader: MyRaygenShader
[shader("raygeneration")]
void MyRaygenShader()
{
    uint2 dispatchIdx = DispatchRaysIndex().xy;
    uint2 renderTargetDim = uint2(gRenderTargetSize.x, gRenderTargetSize.y);

    // Generate ray from camera for current pixel
    float2 ndc = float2(
        (dispatchIdx.x + 0.5f) / renderTargetDim.x * 2.0f - 1.0f,
        -((dispatchIdx.y + 0.5f) / renderTargetDim.y * 2.0f - 1.0f) // Invert Y for typical screen coords
    );

    // Simplified unprojection (assumes InvView has translation correctly for eye pos, or use separate eye pos)
    // A common way:
    // float4 nearClip = float4(ndc.x, ndc.y, 0.0f, 1.0f); // Assuming Z=0 is near plane in NDC for projection used
    // float4 farClip  = float4(ndc.x, ndc.y, 1.0f, 1.0f); // Assuming Z=1 is far plane in NDC
    //
    // float4x4 invViewProj = mul(gInvProj, gInvView); // Or pass InvViewProj directly if available
    // float4 nearWorld = mul(nearClip, invViewProj);
    // float4 farWorld  = mul(farClip, invViewProj);
    //
    // nearWorld.xyz /= nearWorld.w;
    // farWorld.xyz  /= farWorld.w;
    //
    // float3 rayOriginW = nearWorld.xyz; // Or directly use gEyePosW
    // float3 rayDirW    = normalize(farWorld.xyz - nearWorld.xyz);

    // Alternative and often simpler: Start with eye position and calculate direction to far plane point
    float4 farClipNDC = float4(ndc.x, ndc.y, 1.0f, 1.0f); // Point on the far plane in NDC
    float4x4 invViewProj = mul(gInvProj, gInvView); // This should be pre-calculated on CPU if cbPass doesn't have InvViewProj
                                                     // For now, assume gInvView and gInvProj are correct from cbPass.
                                                     // DungeonStompApp::UpdateMainPassCB calculates InvViewProj. If that's passed, use it.
                                                     // If PassConstants in HLSL matches C++, InvViewProj is available.
                                                     // Let's assume PassConstants struct in HLSL will be expanded or InvViewProj is passed.
                                                     // For this step, I'll use the provided calculation from the prompt:
                                                     // float4 farWorld = mul(float4(ndc.x, ndc.y, 1.0f, 1.0f), gInvViewProj); // Needs gInvViewProj
                                                     // Since I only added gInvView and gInvProj to the cbuffer for now:
    float4 viewRayDir = mul(float4(ndc.x, ndc.y, 1.0f, 1.0f), gInvProj); // To view space direction (at far plane)
    viewRayDir.xyz /= viewRayDir.w;
    float3 worldRayDir = mul((float3x3)gInvView, viewRayDir.xyz); // Transform direction to world space

    float3 rayOriginW = gEyePosW;
    float3 rayDirW = normalize(worldRayDir);


    HitInfo payload;
    // Initialize payload if necessary, e.g. payload.hit = false;
    // payload.color will be overwritten by Miss or CHS

    TraceRay(
        gScene,                         // AccelerationStructure
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES, // RayFlags
        0xFF,                           // InstanceInclusionMask
        0,                              // RayContributionToHitGroupIndex ( SBT record offset for hit group)
        0,                              // MultiplierForGeometryContributionToHitGroupIndex (usually 0, for multiple geom per instance)
        0,                              // MissShaderIndex (SBT record offset for miss shader)
        rayOriginW,                     // RayOrigin
        0.01f,                          // RayTMin
        rayDirW,                        // RayDirection
        10000.0f,                       // RayTMax
        payload                         // Payload
    );

    outputTexture[dispatchIdx] = payload.color;
}

// Miss Shader: MyMissShader
[shader("miss")]
void MyMissShader(inout HitInfo payload)
{
    // Return dark grey for missed rays
    payload.color = float4(0.2f, 0.2f, 0.2f, 1.0f);
}

Texture2D gDiffuseMap : register(t1);
SamplerState gsamLinearWrap : register(s2); // Matches static sampler in DXR root signature

// Closest Hit Shader: MyClosestHitShader
[shader("closesthit")]
void MyClosestHitShader(inout HitInfo payload, in BuiltInTriangleIntersectionAttributes barycentrics, float3 normalW : NORMAL, float2 texCoord : TEXCOORD0)
{
    // Get hit position
    float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    // Normalize the interpolated normal
    float3 N = normalize(normalW);

    // Sample diffuse texture
    float4 albedo = gDiffuseMap.Sample(gsamLinearWrap, texCoord);

    // Basic lighting (using first light from PassConstants, assuming it's a point/directional light)
    // Light struct might need to be defined in HLSL if not part of PassConstants directly for DXR shaders
    // For now, assume gLights[0].Strength and gLights[0].Direction or Position are accessible via PassConstants
    // Let's use a fixed light direction for simplicity if Light struct is not fully available/defined here yet.
    float3 lightDirW = normalize(float3(0.577f, 0.577f, -0.577f)); // Example fixed light direction
    float3 lightColor = float3(1.0f, 1.0f, 1.0f); // Example light color

    // If using Lights from PassConstants (ensure PassConstants in HLSL has Lights array)
    // float3 lightVec = normalize(gLights[0].Position - hitPos); // If point light
    // float3 lightStrength = gLights[0].Strength;
    // For a directional light defined by its direction:
    // lightDirW = normalize(gLights[0].Direction);
    // lightColor = gLights[0].Strength;


    float ndotl = saturate(dot(N, lightDirW));
    float3 directLighting = ndotl * lightColor * albedo.rgb;

    // Add a small ambient term
    float3 ambient = 0.1f * albedo.rgb;
    float3 litColor = ambient;

    // Shadow Ray
    ShadowHitInfo shadowPayload;
    shadowPayload.hit = 0.0f; // 0.0f for no hit, 1.0f for hit

    float3 shadowRayOrigin = hitPos + N * 0.001f; // Offset to avoid self-intersection
    // Assuming gLights[0] is the primary light source for now from PassConstants
    // This requires PassConstants in HLSL to include the Lights array.
    // For now, using the same fixed light direction as in the lighting calculation.
    // float3 lightPos = gLights[0].Position; // If point light
    // float shadowRayDir = normalize(lightPos - shadowRayOrigin);
    // float maxShadowDist = length(lightPos - shadowRayOrigin);

    // Using the same fixed light direction for shadow test (matches lighting)
    float3 shadowRayDir = lightDirW; // lightDirW is already normalized
    float maxShadowDist = 10000.0f; // Max distance for shadow ray (effectively scene size)


    TraceRay(
        gScene,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        0xFF,                                 // InstanceInclusionMask
        1,                                    // RayContributionToHitGroupIndex (1 for "ShadowHitGroup")
        0,                                    // MultiplierForGeometryContributionToHitGroupIndex
        1,                                    // MissShaderIndex (1 for "MyShadowMissShader")
        shadowRayOrigin,
        0.001f,                               // RayTMin
        shadowRayDir,
        maxShadowDist,                        // RayTMax
        shadowPayload
    );

    if (shadowPayload.hit < 0.5f) // If shadowPayload.hit is close to 0.0 (false)
    {
        litColor += directLighting;
    }

    // Reflection Ray
    float3 incidentVec = normalize(hitPos - WorldRayOrigin()); // Points from eye to hitPos
    float3 reflectionDir = reflect(incidentVec, N);
    float3 reflectionRayOrigin = hitPos + N * 0.001f; // Offset from surface along normal

    HitInfo reflectionPayload;
    reflectionPayload.color = float4(0.0f, 0.0f, 0.0f, 0.0f); // Initialize to black

    // Only trace reflection if recursion depth allows (MaxTraceRecursionDepth is 2 for this setup)
    // The primary ray is depth 0, shadow is depth 1. Reflection from primary hit is also depth 1.
    // If reflection hit CHS, its shadow ray would be depth 2.
    // This simple reflection will be at the same "level" as the shadow ray from the primary hit point.
    // No explicit depth check needed here if MaxTraceRecursionDepth >= 2 and reflections don't recurse further themselves.

    TraceRay(
        gScene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        0xFF,          // InstanceInclusionMask
        0,             // RayContributionToHitGroupIndex (0 for "MyHitGroup")
        0,             // MultiplierForGeometryContributionToHitGroupIndex
        0,             // MissShaderIndex (0 for "MyMissShader")
        reflectionRayOrigin,
        0.001f,        // RayTMin
        reflectionDir,
        10000.0f,      // RayTMax
        reflectionPayload
    );

    float reflectivity = 0.35f; // Constant reflectivity for now
    litColor += reflectionPayload.color.rgb * reflectivity;

    payload.color = float4(litColor, 1.0f);
    // payload.color = albedo; // To test texturing only
    // payload.color = float4(N * 0.5f + 0.5f, 1.0f); // To test normals
}

// --- Shadow Ray Shaders ---

// Shadow Ray Payload
struct ShadowHitInfo
{
    float hit : SV_RayPayload; // Using float for safety, 0.0 = false (no shadow), 1.0 = true (shadow)
};

// Shadow Miss Shader
[shader("miss")]
void MyShadowMissShader(inout ShadowHitInfo payload)
{
    payload.hit = 0.0f; // Ray reached light, not shadowed
}

// Shadow Any Hit Shader (for opaque objects)
[shader("anyhit")]
void MyShadowAnyHitShader(inout ShadowHitInfo payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.hit = 1.0f; // Ray hit an occluder, object is in shadow
    AcceptHitAndEndSearch(); // Important for efficiency
}
