//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
// Modified by Mark Longo 2020
// Default shader, currently supports lighting.
//***************************************************************************************

//***************************************************************************************
// Common.hlsl by Frank Luna (C) 2015 All Rights Reserved.
// Modified by Mark Longo 2020
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 16
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 10
#endif

#define MaxLights 32

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction; // directional/spot light only
    float FalloffEnd; // point/spot light only
    float3 Position; // point light only
    float SpotPower; // spot light only
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
    float Roughness;
    float Metallic;
    float Timertick;
};


Texture2D gDiffuseMap : register(t0);
Texture2D gNormalMap : register(t1);
Texture2D gShadowMap : register(t2);
TextureCube gCubeMap : register(t4);
Texture2D gSsaoMap : register(t5);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
};

cbuffer cbMaterial : register(b1)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
    float gMetal;
    float gTimertick;
};

cbuffer cbPass : register(b2)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float4x4 gViewProjTex;
    float4x4 gShadowTransform;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;
    float4 gFogColor;
    float gFogStart;
    float gFogRange;
    float2 cbPerObjectPad2;
    Light gLights[MaxLights];
};

struct MaterialData
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Roughness;
    float4x4 MatTransform;
};


float3 LinearToSRGB(float3 color)
{
    return pow(color, 1.0f / 2.2f);
}

float3 SRGBToLinear(float3 color)
{
    return pow(color, 2.2f);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = 3.14159265f * denom * denom;
    return a2 / denom;
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

float TorchFlicker(float baseStrength, float time, float freq, float amp)
{
    float flicker = sin(time * freq) * 0.5f + 0.5f;
    flicker += sin(time * (freq * 2.13f)) * 0.25f + 0.25f;
    flicker += sin(time * (freq * 0.77f)) * 0.15f + 0.15f;
    flicker = saturate(flicker);
    return baseStrength * (1.0f - amp * flicker);
}

// Unified PBR lighting for all light types
float3 PBRLightingUnified(
    Light L, int lightType, // 0=dir, 1=point, 2=spot
    Material mat,
    float3 pos, float3 N, float3 V,
    float shadowFactor, float timertickOffset)
{
    float3 albedo = mat.DiffuseAlbedo.rgb;
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, mat.Metallic);
    float3 Ld;
    float attenuation = 1.0f;
    float flicker = 1.0f;

    if (lightType == 0) // Directional
    {
        Ld = -normalize(L.Direction);
    }
    else if (lightType == 1) // Point
    {
        float3 lightVec = L.Position - pos;
        float d = length(lightVec);
        if (d > L.FalloffEnd)
            return 0.0f;
        Ld = normalize(lightVec);
        attenuation = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
        flicker = TorchFlicker(1.0f, mat.Timertick + timertickOffset, 8.0f, 0.25f);
    }
    else // Spot
    {
        float3 lightVec = L.Position - pos;
        float d = length(lightVec);
        if (d > L.FalloffEnd)
            return 0.0f;
        Ld = normalize(lightVec);
        attenuation = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
        float spotFactor = pow(max(dot(-Ld, normalize(L.Direction)), 0.0f), L.SpotPower);
        attenuation *= spotFactor;
    }

    float3 H = normalize(V + Ld);
    float NDF = DistributionGGX(N, H, mat.Roughness);
    float G = GeometrySmith(N, V, Ld, mat.Roughness);
    float NdotL = max(dot(N, Ld), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0f * NdotV * NdotL + 0.001f;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - mat.Metallic * flicker;

    float3 diffuse = kD * albedo / 3.14159265f;

    float3 lightStrength = L.Strength * flicker;
    return (diffuse + specular) * lightStrength * NdotL * attenuation * shadowFactor;
}

float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
    float3 normalT = 2.0f * normalMapSample - 1.0f;
    float3 N = unitNormalW;
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    float3 bumpedNormalW = mul(normalT, TBN);
    return bumpedNormalW;
}

float CalcShadowFactor(float4 shadowPosH)
{
    shadowPosH.xyz /= shadowPosH.w;
    float depth = shadowPosH.z;
    if (shadowPosH.x < 0.0f || shadowPosH.x > 1.0f ||
        shadowPosH.y < 0.0f || shadowPosH.y > 1.0f)
    {
        return 1.0f;
    }
    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);
    float dx = 1.0f / (float) width;
    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };
    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += gShadowMap.SampleCmpLevelZero(gsamShadow,
            shadowPosH.xy + offsets[i], depth).r;
    }
    return percentLit / 9.0f;
}

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float4 SsaoPosH : POSITION1;
    float3 PosW : POSITION2;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);
    vout.PosH = mul(posW, gViewProj);
    vout.SsaoPosH = mul(posW, gViewProjTex);
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, gMatTransform).xy;
    vout.ShadowPosH = mul(posW, gShadowTransform);
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // Sample material data
    float4 diffuseAlbedo = gDiffuseAlbedo;
    float3 fresnelR0 = gFresnelR0;
    float roughness = gRoughness;
    float metal = gMetal;

    // Sample diffuse texture
    diffuseAlbedo *= gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);

#ifdef ALPHA_TEST
    clip(diffuseAlbedo.a - 0.1f);
#endif

#ifdef TORCH_TEST
    // Make the torches bright by just returning the texture.
    return diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);
#endif    

    // Normal mapping
    float3 norm = normalize(pin.NormalW);
    float3 bumpedNormalW = norm;
    float4 normalMapSample = float4(1.0f, 1.0f, 1.0f, 1.0f);

    normalMapSample = gNormalMap.Sample(gsamAnisotropicWrap, pin.TexC);
    bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, norm, pin.TangentW);


    // Eye vector
    float3 toEyeW = gEyePosW - pin.PosW;
    float distToEye = length(toEyeW);
    toEyeW /= distToEye;

    // SSAO (ambient occlusion)
    pin.SsaoPosH /= pin.SsaoPosH.w;
    float ambientAccess = 1.0f;
#ifdef SSAO    
    ambientAccess = gSsaoMap.Sample(gsamLinearClamp, pin.SsaoPosH.xy, 0.0f).r;
#endif

    // Ambient
    float3 ambientColor = ambientAccess * gAmbientLight.rgb * diffuseAlbedo.rgb;
    float4 ambient = float4(ambientColor, 0.0f);

    // Shadow
    float shadowFactor = CalcShadowFactor(pin.ShadowPosH);

    // Material struct
    Material mat = { diffuseAlbedo, fresnelR0, 0.0f, roughness, metal, gTotalTime };

    float3 N = normalize(bumpedNormalW);
    float3 V = normalize(toEyeW);
    float3 color = 0.0f;
    int i = 0;

    // Directional lights
#if (NUM_DIR_LIGHTS > 0)
    [unroll]
    for (i = 0; i < NUM_DIR_LIGHTS; ++i)
        color += PBRLightingUnified(gLights[i], 0, mat, pin.PosW, N, V, shadowFactor, 0.0f);
#endif
    // Point lights (torch flicker)
#if (NUM_POINT_LIGHTS > 0)
    [unroll]
    for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
        color += PBRLightingUnified(gLights[i], 1, mat, pin.PosW, N, V, shadowFactor, (float) i);
#endif
    // Spot lights
#if (NUM_SPOT_LIGHTS > 0)
    [unroll]
    for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
        color += PBRLightingUnified(gLights[i], 2, mat, pin.PosW, N, V, shadowFactor, 0.0f);
#endif

    // Add ambient (diffuse only, no IBL)
    color += ambient.rgb * (1.0f - metal);

    // Fog
#ifdef FOG
    float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
    color = lerp(color, gFogColor.rgb, fogAmount);
#endif

    return float4(color, diffuseAlbedo.a);
}
