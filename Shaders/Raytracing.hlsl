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

// Vertex buffer (44 bytes per vertex: Pos(12) + Normal(12) + TexC(8) + TangentU(12))
struct Vertex
{
    float3 Pos;
    float3 Normal;
    float2 TexC;
    float3 TangentU;
};

ByteAddressBuffer gVertices : register(t1);

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
};

// Vertex attributes (interpolated)
struct VertexAttributes
{
    float3 position;
    float3 normal;
    float2 texCoord;
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
    
    RayPayload payload = { float4(0.0f, 0.0f, 0.0f, 1.0f) };
    
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
    // Sky/background color - dark dungeon atmosphere
    float3 skyColor = float3(0.02f, 0.02f, 0.03f);
    payload.color = float4(skyColor, 1.0f);
}

//=============================================================================
// Closest Hit Shader
//=============================================================================

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    // Get primitive index - for triangle list, each triangle has 3 vertices
    uint primIdx = PrimitiveIndex();
    uint vertexIndex = primIdx * 3; // Triangle list: 3 vertices per primitive
    
    // Load the 3 vertices of this triangle
    Vertex v0 = LoadVertex(vertexIndex);
    Vertex v1 = LoadVertex(vertexIndex + 1);
    Vertex v2 = LoadVertex(vertexIndex + 2);
    
    // Barycentric coordinates
    float3 bary = float3(1.0f - attribs.barycentrics.x - attribs.barycentrics.y,
                         attribs.barycentrics.x,
                         attribs.barycentrics.y);
    
    // Interpolate position
    float3 localPos = v0.Pos * bary.x + v1.Pos * bary.y + v2.Pos * bary.z;
    
    // Interpolate normal and normalize
    float3 N = normalize(v0.Normal * bary.x + v1.Normal * bary.y + v2.Normal * bary.z);
    
    // Interpolate texture coordinates
    float2 texCoord = v0.TexC * bary.x + v1.TexC * bary.y + v2.TexC * bary.z;
    
    // Get hit position from ray
    float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 rayDir = WorldRayDirection();
    
    // Ensure normal faces the camera (flip if pointing away from ray)
    if (dot(N, -rayDir) < 0.0f)
        N = -N;
    
    // View direction
    float3 V = normalize(gCameraPos - hitPos);
    
    // Use texture coordinates for variation in albedo
    float variation = frac(sin(dot(texCoord, float2(12.9898f, 78.233f))) * 43758.5453f);
    float3 albedo = lerp(float3(0.4f, 0.38f, 0.35f), float3(0.55f, 0.52f, 0.48f), variation);
    
    // Material properties
    float roughness = gRoughness;
    float metallic = gMetallic;
    
    // Compute lighting using simplified PBR
    float3 color = float3(0.0f, 0.0f, 0.0f);
    
    // Add ambient
    float3 ambient = gAmbientLight.rgb * albedo * 0.3f;
    color += ambient;
    
    // Simple directional light (main light)
    if (gNumLights > 0)
    {
        Light L = gLights[0];
        float3 lightDir = -normalize(L.Direction);
        float NdotL = max(dot(N, lightDir), 0.0f);
        color += albedo * L.Strength * NdotL * 0.7f;
        
        // Simple specular
        float3 H = normalize(V + lightDir);
        float NdotH = max(dot(N, H), 0.0f);
        float spec = pow(NdotH, 32.0f * (1.0f - roughness + 0.1f));
        color += L.Strength * spec * 0.3f;
    }
    
    // Add point lights (torches) with attenuation
    const uint NUM_DIR_LIGHTS = 1;
    for (uint i = NUM_DIR_LIGHTS; i < min(gNumLights, NUM_DIR_LIGHTS + 8u); ++i)
    {
        Light L = gLights[i];
        float3 lightVec = L.Position - hitPos;
        float d = length(lightVec);
        
        if (d < L.FalloffEnd && d > 0.001f)
        {
            float3 lightDir = lightVec / d;
            float atten = saturate((L.FalloffEnd - d) / (L.FalloffEnd - L.FalloffStart));
            atten *= atten; // Quadratic falloff for softer look
            
            // Torch flicker
            float flicker = TorchFlicker(1.0f, gTotalTime, 8.0f, 0.2f, (float)i);
            
            float NdotL = max(dot(N, lightDir), 0.0f);
            color += albedo * L.Strength * NdotL * atten * flicker;
        }
    }
    
    // Add minimum visibility to confirm hits
    color = max(color, float3(0.15f, 0.1f, 0.08f));
    
    // Clamp and apply simple tone mapping
    color = saturate(color);
    color = pow(color, 1.0f / 2.2f); // Gamma correction
    
    payload.color = float4(color, 1.0f);
}
