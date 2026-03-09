//***************************************************************************************
// Raytracing.hlsl - DirectX Raytracing Shader with PBR Lighting
// Ray generation, closest hit, and miss shaders for DXR with physically-based rendering
//***************************************************************************************

#define MaxLights 32
#define PI 3.14159265f

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

// Ray payload
struct RayPayload
{
    float4 color;
};

// Vertex attributes
struct VertexAttributes
{
    float3 position;
    float3 normal;
};

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
    kD *= 1.0f - metallic * flicker;
    
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
// Ray Generation Shader
//=============================================================================

[shader("raygeneration")]
void RayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;
    
    // Calculate normalized device coordinates
    float2 pixelCenter = float2(launchIndex) + float2(0.5f, 0.5f);
    float2 ndc = pixelCenter / float2(launchDim);
    ndc = ndc * 2.0f - 1.0f;
    ndc.y = -ndc.y; // Flip Y for DirectX
    
    // Generate ray from camera
    float4 worldPos = mul(float4(ndc, 0.0f, 1.0f), gInvViewProj);
    worldPos.xyz /= worldPos.w;
    
    float3 rayOrigin = gCameraPos;
    float3 rayDirection = normalize(worldPos.xyz - gCameraPos);
    
    // Trace ray
    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = rayDirection;
    ray.TMin = 0.001f;
    ray.TMax = 10000.0f;
    
    RayPayload payload = { float4(0.0f, 0.0f, 0.0f, 1.0f) };
    
    TraceRay(
        gScene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
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
    // Sky/background color - gradient based on ray direction
    float3 rayDir = WorldRayDirection();
    float t = 0.5f * (rayDir.y + 1.0f);
    
    // Sky gradient from dark blue to light blue
    float3 bottomColor = float3(0.02f, 0.02f, 0.05f);
    float3 topColor = float3(0.1f, 0.15f, 0.3f);
    
    payload.color = float4(lerp(bottomColor, topColor, t), 1.0f);
}

//=============================================================================
// Closest Hit Shader
//=============================================================================

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    // Get hit position
    float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    // Compute geometric normal from barycentric coordinates
    // Since we don't have access to vertex normals directly in this simple setup,
    // we compute a face normal using cross product of edges
    float3 barycentrics = float3(
        1.0f - attribs.barycentrics.x - attribs.barycentrics.y,
        attribs.barycentrics.x,
        attribs.barycentrics.y
    );
    
    // For now, use the ray direction to compute a simple face normal
    // In a more complete implementation, you'd read vertex normals from a buffer
    float3 N = normalize(-WorldRayDirection());
    
    // Flip normal if we hit the back face
    if (dot(N, WorldRayDirection()) > 0)
        N = -N;
    
    // View direction
    float3 V = normalize(gCameraPos - hitPos);
    
    // Default albedo color (gray stone/dungeon color)
    float3 albedo = float3(0.5f, 0.5f, 0.5f);
    
    // Material properties
    float roughness = gRoughness;
    float metallic = gMetallic;
    
    // Compute lighting using PBR
    float3 color = float3(0.0f, 0.0f, 0.0f);
    
    // Add ambient
    color += gAmbientLight.rgb * albedo * (1.0f - metallic);
    
    // Process lights
    // Lights are organized: [0, NUM_DIR_LIGHTS) = directional
    //                      [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) = point
    //                      [rest] = spot
    const uint NUM_DIR_LIGHTS = 1;
    const uint NUM_POINT_LIGHTS = 16;
    
    for (uint i = 0; i < gNumLights && i < MaxLights; ++i)
    {
        Light L = gLights[i];
        
        if (i < NUM_DIR_LIGHTS)
        {
            // Directional light
            color += ComputeDirectionalLight(L, albedo, N, V, roughness, metallic);
        }
        else if (i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS)
        {
            // Point light (torches with flicker)
            color += ComputePointLight(L, hitPos, albedo, N, V, roughness, metallic, (float)i);
        }
        else
        {
            // Spot light
            color += ComputeSpotLight(L, hitPos, albedo, N, V, roughness, metallic);
        }
    }
    
    // Rim lighting for extra pop
    {
        float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
        float NdotV = max(dot(N, V), 0.0f);
        float rimIntensity = 0.09f;
        float rimExponent = lerp(1.0f, 8.0f, 1.0f - roughness);
        float rimTerm = pow(saturate(1.0f - NdotV), rimExponent);
        float rimEnergy = saturate(rimTerm * rimIntensity);
        float3 rimF = FresnelSchlick(NdotV, F0);
        float3 rimSpecular = rimF * rimEnergy;
        color += rimSpecular;
    }
    
    // Tonemap and gamma correction
    color = color / (color + float3(1.0f, 1.0f, 1.0f)); // Reinhard tonemap
    color = pow(color, 1.0f / 2.2f); // Gamma correction
    
    payload.color = float4(color, 1.0f);
}
