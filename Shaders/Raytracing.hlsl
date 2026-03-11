//***************************************************************************************
// Raytracing.hlsl - DirectX Raytracing Shader with PBR Lighting
// Ray generation, closest hit, and miss shaders for DXR with physically-based rendering
// Features: inline shadow rays (DXR 1.1), full Cook-Torrance PBR, ACES tone mapping,
//           atmospheric fog, wet floor reflectance, per-light flicker
//***************************************************************************************

#define MaxLights 32
#define PI 3.14159265f

// Max point lights that cast shadow rays (performance knob)
#define MAX_SHADOW_LIGHTS 12

// Fog density for dungeon atmosphere
#define FOG_DENSITY 0.0025f

// Shadow ray bias to prevent self-intersection
#define SHADOW_BIAS 0.15f

// Light structure matching CPU-side
struct Light
{
    float3 Strength;
    float FalloffStart;
    float3 Direction;
    float FalloffEnd;
    float3 Position;
    float SpotPower;
};

// Scene constants
cbuffer SceneConstants : register(b0)
{
    float4x4 gInvViewProj;
    float3 gCameraPos;
    float gPad0;
    float4 gAmbientLight;
    Light gLights[MaxLights];
    uint gNumLights;
    float gTotalTime;
    float gRoughness;
    float gMetallic;
};

// Raytracing output
RWTexture2D<float4> gOutput : register(u0);

// Acceleration structure
RaytracingAccelerationStructure gScene : register(t0);

// Vertex buffer (44 bytes per vertex: Pos(12) + Normal(12) + TexC(8) + TangentU(12))
struct Vertex
{
    float3 Pos;
    float3 Normal;
    float2 TexC;
    float3 TangentU;
};

ByteAddressBuffer gVertices : register(t1);

// Texture array (copied to DXR heap)
Texture2D gTextures[] : register(t2);

// Per-primitive texture index buffer
ByteAddressBuffer gPrimitiveTextureIndices : register(t0, space1);

// Per-primitive normal map texture index buffer (-1 = no normal map)
ByteAddressBuffer gPrimitiveNormalMapIndices : register(t1, space1);

// Sampler for texture sampling
SamplerState gSampler : register(s0);

// Helper to load vertex from byte address buffer
Vertex LoadVertex(uint vertexIndex)
{
    // 44 bytes per vertex = 11 DWORDs
    uint address = vertexIndex * 44;
    
    Vertex v;
    // Load position (3 floats)
    v.Pos.x = asfloat(gVertices.Load(address));
    v.Pos.y = asfloat(gVertices.Load(address + 4));
    v.Pos.z = asfloat(gVertices.Load(address + 8));
    
    // Load normal (3 floats)
    v.Normal.x = asfloat(gVertices.Load(address + 12));
    v.Normal.y = asfloat(gVertices.Load(address + 16));
    v.Normal.z = asfloat(gVertices.Load(address + 20));
    
    // Load texcoord (2 floats)
    v.TexC.x = asfloat(gVertices.Load(address + 24));
    v.TexC.y = asfloat(gVertices.Load(address + 28));
    
    // Load tangent (3 floats)
    v.TangentU.x = asfloat(gVertices.Load(address + 32));
    v.TangentU.y = asfloat(gVertices.Load(address + 36));
    v.TangentU.z = asfloat(gVertices.Load(address + 40));
    
    return v;
}

// Ray payload
struct RayPayload
{
    float4 color;
    uint   depth; // recursion depth for transparency
};

// Hard-coded transparent texture ranges (matching CPU-side alpha skip logic)
bool IsTransparentTexture(uint texIdx)
{
    if (texIdx >= 94  && texIdx <= 101) return true;  // flames / effects
    if (texIdx >= 278 && texIdx <= 295) return true;  // 279-1..296-1
    if (texIdx >= 205 && texIdx <= 209) return true;  // 206-1..210-1
    if (texIdx >= 370 && texIdx <= 378) return true;  // 371-1..379-1
    if (texIdx == 378)                  return true;
    return false;
}

//=============================================================================
// Normal Map Helper
//=============================================================================

float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
    // Unpack from [0,1] to [-1,1]
    float3 normalT = 2.0f * normalMapSample - 1.0f;
    
    // Build orthonormal TBN basis
    float3 N = unitNormalW;
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    
    return mul(normalT, TBN);
}

//=============================================================================
// PBR Helper Functions
//=============================================================================

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001f);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

// Torch flicker effect
float TorchFlicker(float baseStrength, float time, float freq, float amp, float offset)
{
    float t = time + offset;
    float flicker = sin(t * freq) * 0.5f + 0.5f;
    flicker += sin(t * (freq * 2.13f)) * 0.25f + 0.25f;
    flicker += sin(t * (freq * 0.77f)) * 0.15f + 0.15f;
    flicker = saturate(flicker);
    return baseStrength * (1.0f - amp * flicker);
}

// PBR lighting for directional light
float3 ComputeDirectionalLight(Light L, float3 albedo, float3 N, float3 V, float roughness, float metallic)
{
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 Ld = -normalize(L.Direction);
    float3 H = normalize(V + Ld);
    
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, Ld, roughness);
    float NdotL = max(dot(N, Ld), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0f * NdotV * NdotL + 0.001f;
    float3 specular = numerator / denominator;
    
    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - metallic;
    
    float3 diffuse = kD * albedo / PI;
    return (diffuse + specular) * L.Strength * NdotL;
}

// PBR lighting for point light (with torch flicker)
float3 ComputePointLight(Light L, float3 pos, float3 albedo, float3 N, float3 V, float roughness, float metallic, float lightIndex)
{
    float3 lightVec = L.Position - pos;
    float d = length(lightVec);
    
    if (d > L.FalloffEnd)
        return float3(0.0f, 0.0f, 0.0f);
    
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 Ld = normalize(lightVec);
    float3 H = normalize(V + Ld);
    
    float attenuation = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    float flicker = TorchFlicker(1.0f, gTotalTime, 8.0f, 0.25f, lightIndex);
    
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, Ld, roughness);
    float NdotL = max(dot(N, Ld), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0f * NdotV * NdotL + 0.001f;
    float3 specular = numerator / denominator;
    
    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - metallic;
    
    float3 diffuse = kD * albedo / PI;
    float3 lightStrength = L.Strength * flicker;
    
    return (diffuse + specular) * lightStrength * NdotL * attenuation;
}

// PBR lighting for spot light
float3 ComputeSpotLight(Light L, float3 pos, float3 albedo, float3 N, float3 V, float roughness, float metallic)
{
    float3 lightVec = L.Position - pos;
    float d = length(lightVec);
    
    if (d > L.FalloffEnd)
        return float3(0.0f, 0.0f, 0.0f);
    
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 Ld = normalize(lightVec);
    float3 H = normalize(V + Ld);
    
    float attenuation = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    float spotFactor = pow(max(dot(-Ld, normalize(L.Direction)), 0.0f), L.SpotPower);
    attenuation *= spotFactor;
    
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, Ld, roughness);
    float NdotL = max(dot(N, Ld), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0f * NdotV * NdotL + 0.001f;
    float3 specular = numerator / denominator;
    
    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - metallic;
    
    float3 diffuse = kD * albedo / PI;
    return (diffuse + specular) * L.Strength * NdotL * attenuation;
}

//=============================================================================
// Shadow Ray (DXR 1.1 Inline Raytracing)
//=============================================================================

// Returns 1.0 if fully lit, 0.0 if fully shadowed
float TraceShadowRay(float3 origin, float3 direction, float maxDist)
{
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> shadowQuery;
    
    RayDesc shadowRay;
    shadowRay.Origin = origin;
    shadowRay.Direction = direction;
    shadowRay.TMin = 0.05f;
    shadowRay.TMax = maxDist - 0.1f;
    
    shadowQuery.TraceRayInline(gScene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, shadowRay);
    shadowQuery.Proceed();
    
    return (shadowQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT) ? 0.0f : 1.0f;
}

//=============================================================================
// ACES Filmic Tone Mapping
//=============================================================================

float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

//=============================================================================
// Ray Generation Shader
//=============================================================================

[shader("raygeneration")]
void RayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;
    
    // Calculate normalized device coordinates (0 to 1, then -1 to 1)
    float2 pixelCenter = float2(launchIndex) + float2(0.5f, 0.5f);
    float2 uv = pixelCenter / float2(launchDim);
    
    // Convert to clip space (-1 to 1)
    float2 clipXY = uv * 2.0f - 1.0f;
    clipXY.y = -clipXY.y; // Flip Y for DirectX coordinate system
    
    // Unproject near and far points to get ray
    float4 nearPoint = mul(float4(clipXY, 0.0f, 1.0f), gInvViewProj);
    float4 farPoint = mul(float4(clipXY, 1.0f, 1.0f), gInvViewProj);
    
    nearPoint.xyz /= nearPoint.w;
    farPoint.xyz /= farPoint.w;
    
    float3 rayOrigin = nearPoint.xyz;
    float3 rayDirection = normalize(farPoint.xyz - nearPoint.xyz);
    
    // Trace ray
    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = rayDirection;
    ray.TMin = 0.01f;
    ray.TMax = 100000.0f;
    
    RayPayload payload;
    payload.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    payload.depth = 0;
    
    TraceRay(
        gScene,
        RAY_FLAG_NONE,  // Don't cull - dungeon geometry might have back faces visible
        0xFF,
        0,  // Hit group index
        1,  // Multiplier for geometry index
        0,  // Miss shader index
        ray,
        payload
    );
    
    gOutput[launchIndex] = payload.color;
}

//=============================================================================
// Miss Shader
//=============================================================================

[shader("miss")]
void Miss(inout RayPayload payload)
{
    // Atmospheric dungeon void - slight vertical gradient for depth
    float3 rayDir = WorldRayDirection();
    float upFactor = saturate(rayDir.y * 0.5f + 0.5f);
    float3 darkFloor = float3(0.01f, 0.01f, 0.015f);
    float3 darkCeiling = float3(0.025f, 0.02f, 0.03f);
    float3 skyColor = lerp(darkFloor, darkCeiling, upFactor);
    payload.color = float4(skyColor, 1.0f);
}

//=============================================================================
// Closest Hit Shader - Full PBR with Shadows
//=============================================================================

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    // Get primitive index - for triangle list, each triangle has 3 vertices
    uint primIdx = PrimitiveIndex();
    uint vertexIndex = primIdx * 3;
    
    // Load the 3 vertices of this triangle
    Vertex v0 = LoadVertex(vertexIndex);
    Vertex v1 = LoadVertex(vertexIndex + 1);
    Vertex v2 = LoadVertex(vertexIndex + 2);
    
    // Barycentric coordinates
    float3 bary = float3(1.0f - attribs.barycentrics.x - attribs.barycentrics.y,
                         attribs.barycentrics.x,
                         attribs.barycentrics.y);
    
    // Interpolate vertex attributes
    float3 localPos = v0.Pos * bary.x + v1.Pos * bary.y + v2.Pos * bary.z;
    float3 N = normalize(v0.Normal * bary.x + v1.Normal * bary.y + v2.Normal * bary.z);
    float2 texCoord = v0.TexC * bary.x + v1.TexC * bary.y + v2.TexC * bary.z;
    float3 T = normalize(v0.TangentU * bary.x + v1.TangentU * bary.y + v2.TangentU * bary.z);
    
    // Hit position and ray direction
    float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 rayDir = WorldRayDirection();
    
    // Ensure normal faces the camera
    if (dot(N, -rayDir) < 0.0f)
        N = -N;
    
    // View direction
    float3 V = normalize(gCameraPos - hitPos);
    
    // Sample texture
    uint texIndex = gPrimitiveTextureIndices.Load(primIdx * 4);
    
    // Normal mapping: sample and apply if this primitive has a normal map
    int normalMapIndex = asint(gPrimitiveNormalMapIndices.Load(primIdx * 4));
    if (normalMapIndex >= 0 && normalMapIndex < 550)
    {
        float3 normalMapSample = gTextures[NonUniformResourceIndex((uint)normalMapIndex)].SampleLevel(gSampler, texCoord, 0).rgb;
        N = normalize(NormalSampleToWorldSpace(normalMapSample, N, T));
    }
    float4 texSample = float4(0.5f, 0.5f, 0.5f, 1.0f);
    
    if (texIndex < 550)
    {
        texSample = gTextures[NonUniformResourceIndex(texIndex)].SampleLevel(gSampler, texCoord, 0);
    }
    
    // Alpha-test transparency: skip through transparent textures
        if (IsTransparentTexture(texIndex) && texSample.a < 0.3f && payload.depth < 4)
    {
        // Fire a continuation ray through this surface
        RayDesc contRay;
        contRay.Origin = hitPos + rayDir * 0.01f;
        contRay.Direction = rayDir;
        contRay.TMin = 0.01f;
        contRay.TMax = 100000.0f;
        
        RayPayload contPayload;
        contPayload.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
        contPayload.depth = payload.depth + 1;
        
        TraceRay(gScene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, contRay, contPayload);
        payload.color = contPayload.color;
        return;
    }
    
    float3 albedo;
    if (texIndex < 550)
    {
        albedo = texSample.rgb;
    }
    else
    {
        float variation = frac(sin(dot(texCoord, float2(12.9898f, 78.233f))) * 43758.5453f);
        albedo = lerp(float3(0.4f, 0.38f, 0.35f), float3(0.55f, 0.52f, 0.48f), variation);
    }
    
    // Transparent textures (torches/flames) are emissive: return raw texture color, no shading
    if (IsTransparentTexture(texIndex))
    {
        payload.color = float4(albedo, texSample.a);
        return;
    }
    
    // ---- Material properties ----
    float roughness = gRoughness;
    float metallic = gMetallic;
    
    // Wet floor effect: horizontal surfaces get slight glossiness
    float floorFactor = saturate(dot(N, float3(0.0f, 1.0f, 0.0f)));
    if (floorFactor > 0.7f)
    {
        float wetness = (floorFactor - 0.7f) / 0.3f; // 0 to 1
        roughness = lerp(roughness, roughness * 0.5f, wetness * 0.6f);
        // Darken wet albedo slightly (wet surfaces absorb more light)
        albedo *= lerp(1.0f, 0.85f, wetness * 0.4f);
    }
    
    // ---- Lighting accumulation ----
    float3 color = float3(0.0f, 0.0f, 0.0f);
    
    // Ambient: scaled down for dramatic contrast, with hemisphere variation
    // Surfaces facing up get slightly more ambient (indirect sky bounce)
    // Surfaces facing down (ceilings) get less
    float hemiBlend = dot(N, float3(0.0f, 1.0f, 0.0f)) * 0.5f + 0.5f;
    float ambientScale = lerp(0.08f, 0.18f, hemiBlend);
    float3 ambient = gAmbientLight.rgb * ambientScale * albedo;
    // Cool tint in ambient shadows
    ambient *= float3(0.85f, 0.9f, 1.0f);
    color += ambient;
    
    // Shadow ray origin: offset along normal to prevent self-intersection
    float3 shadowOrigin = hitPos + N * SHADOW_BIAS;
    
    // ---- Directional light (gLights[0]) with shadow ----
    if (gNumLights > 0)
    {
        Light L = gLights[0];
        float3 lightDir = -normalize(L.Direction);
        float NdotL = max(dot(N, lightDir), 0.0f);
        
        if (NdotL > 0.001f)
        {
            float shadow = TraceShadowRay(shadowOrigin, lightDir, 10000.0f);
            color += ComputeDirectionalLight(L, albedo, N, V, roughness, metallic) * shadow;
        }
    }
    
    // ---- Point lights (torches + missiles) with shadows ----
    for (uint i = 1; i < min(gNumLights, (uint)MaxLights); ++i)
    {
        Light L = gLights[i];
        float3 lightVec = L.Position - hitPos;
        float d = length(lightVec);
        
        if (d < L.FalloffEnd && d > 0.01f)
        {
            float3 lightDir = lightVec / d;
            float NdotL = max(dot(N, lightDir), 0.0f);
            
            if (NdotL > 0.001f)
            {
                // Shadow ray for nearby lights (skip distant ones for performance)
                float shadow = 1.0f;
                if (i <= MAX_SHADOW_LIGHTS)
                {
                   // shadow = TraceShadowRay(shadowOrigin, lightDir, d);
                }
                
                color += ComputePointLight(L, hitPos, albedo, N, V, roughness, metallic, (float)i) * shadow;
            }
        }
    }
    
    // ---- Atmospheric distance fog ----
    float dist = length(hitPos - gCameraPos);
    float fogFactor = 1.0f - exp(-dist * FOG_DENSITY);
    fogFactor = saturate(fogFactor);
    // Fog color: dark blue-gray with slight warmth from nearby torches
    float3 fogColor = float3(0.015f, 0.015f, 0.025f);
    color = lerp(color, fogColor, fogFactor);
    
    // Minimum visibility so geometry silhouettes are faintly visible
    color = max(color, float3(0.012f, 0.01f, 0.008f));
    
    // ---- ACES filmic tone mapping ----
    color = ACESFilm(color);
    
    // Gamma correction
    color = pow(color, 1.0f / 2.2f);
    
    payload.color = float4(color, 1.0f);
}
