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
#define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 16
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 10
#endif



//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
// Modified by Mark Longo 2020
// Contains API for shader lighting.
//***************************************************************************************

#define MaxLights 31

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
    float Roughness; // Use roughness instead of shininess for modern PBR
    float Metallic; // Add metallic for PBR

};


// Helper: Linear to sRGB
float3 LinearToSRGB(float3 color)
{
    return pow(color, 1.0f / 2.2f);
}

// Helper: sRGB to Linear
float3 SRGBToLinear(float3 color)
{
    return pow(color, 2.2f);
}

// Fresnel-Schlick approximation
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

// Normal Distribution Function (GGX/Trowbridge-Reitz)
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

// Geometry function (Schlick-GGX)
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

// PBR Lighting (Cook-Torrance BRDF)
float3 PBRLighting(float3 albedo, float3 N, float3 V, float3 L, float3 F0, float roughness, float metallic, float3 lightColor, float attenuation)
{
    float3 H = normalize(V + L);
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0f * NdotV * NdotL + 0.001f;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - metallic;

    float3 diffuse = kD * albedo / 3.14159265f;

    return (diffuse + specular) * lightColor * NdotL * attenuation;
}


float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor * roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = -L.Direction;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

// Modernized ComputePointLight
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    float3 lightVec = L.Position - pos;
    float d = length(lightVec);
    if (d > L.FalloffEnd)
        return 0.0f;
    float3 Ld = normalize(lightVec);
    float3 N = normalize(normal);
    float3 V = normalize(toEye);

    float3 albedo = mat.DiffuseAlbedo.rgb;
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, 1.0f);
    float attenuation = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);

    return PBRLighting(albedo, N, V, Ld, F0, mat.Roughness, mat.Metallic, L.Strength, attenuation);
}


// Modernized ComputeSpotLight
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    float3 lightVec = L.Position - pos;
    float d = length(lightVec);
    if (d > L.FalloffEnd)
        return 0.0f;
    float3 Ld = normalize(lightVec);
    float3 N = normalize(normal);
    float3 V = normalize(toEye);

    float3 albedo = mat.DiffuseAlbedo.rgb;
    float3 F0 = lerp(float3(1.04f, 1.04f, 1.04f), albedo, 1.0f);
    float attenuation = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);

    // Spot factor
    float spotFactor = pow(max(dot(-Ld, normalize(L.Direction)), 0.0f), L.SpotPower);
    attenuation *= spotFactor;

    return PBRLighting(albedo, N, V, Ld, F0, mat.Roughness, mat.Metallic, L.Strength, attenuation);
}


float4 ComputeLighting(Light gLights[MaxLights], Material mat,
                       float3 pos, float3 normal, float3 toEye,
                       float3 shadowFactor)
{
    float3 result = 0.0f;
    int i = 0;

#if (NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += shadowFactor[0] * ComputeDirectionalLight(gLights[i], mat, pos, normal, toEye);
    }
#endif

#if (NUM_POINT_LIGHTS > 0)
    for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
    {
        result += shadowFactor[0] * ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }
#endif

#if (NUM_SPOT_LIGHTS > 0)
    for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += shadowFactor[0] * ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }
#endif 

    // Optional: Tone mapping and gamma correction
    // result = result / (result + 1.0f); // simple Reinhard tone mapping
    // result = LinearToSRGB(result);

    return float4(result, 0.0f);
}


float4 ComputeLighting2(Light gLights[MaxLights], Material mat,
                       float3 pos, float3 normal, float3 toEye,
                       float3 shadowFactor)
{
    float3 result = 0.0f;

    int i = 0;

#if (NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += shadowFactor[0] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
    }
#endif

#if (NUM_POINT_LIGHTS > 0)
    for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
    {
        result += shadowFactor[0] * ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }
#endif

#if (NUM_SPOT_LIGHTS > 0)
    for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += shadowFactor[0] * ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }
#endif 

    return float4(result, 0.0f);
}





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

// Constant data that varies per frame.
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

    // Allow application to change fog parameters once per frame.
    // For example, we may only use fog for certain times of day.
    float4 gFogColor;
    float gFogStart;
    float gFogRange;
    float2 cbPerObjectPad2;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
};

struct MaterialData
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Roughness;
    float4x4 MatTransform;
};


//---------------------------------------------------------------------------------------
// Transforms a normal map sample to world space.
//---------------------------------------------------------------------------------------
float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
	// Uncompress each component from [0,1] to [-1,1].
    float3 normalT = 2.0f * normalMapSample - 1.0f;

	// Build orthonormal basis.
    float3 N = unitNormalW;
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);

	// Transform from tangent space to world space.
    float3 bumpedNormalW = mul(normalT, TBN);

    return bumpedNormalW;
}

//---------------------------------------------------------------------------------------
// PCF for shadow mapping.
//---------------------------------------------------------------------------------------

float CalcShadowFactor(float4 shadowPosH)
{
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

    // Depth in NDC space.
    float depth = shadowPosH.z;
    
        // Early out if outside shadow map.
    if (shadowPosH.x < 0.0f || shadowPosH.x > 1.0f ||
        shadowPosH.y < 0.0f || shadowPosH.y > 1.0f)
    {
        return 1.0f;
    }

    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);

    // Texel size.
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
    float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC    : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct VertexOut
{
    float4 PosH    : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float4 SsaoPosH   : POSITION1;
    float3 PosW    : POSITION2;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC    : TEXCOORD;
};



VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;

    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3)gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);

    // Generate projective tex-coords to project SSAO map onto scene.
    vout.SsaoPosH = mul(posW, gViewProjTex);

    // Output vertex attributes for interpolation across triangle.
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, gMatTransform).xy;

    //float4x4 worldShadowTransform = mul(gWorld, gShadowTransform);  // normal-offset approach
    //vout.ShadowPosH = mul(float4(vin.PosL + (vin.NormalL * 0.1f), 1.0f), worldShadowTransform);

    // Generate projective tex-coords to project shadow map onto scene.
    vout.ShadowPosH = mul(posW, gShadowTransform);




    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    //float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC) * gDiffuseAlbedo;
    //float4 diffuseAlbedo = gNormalMap.Sample(gsamAnisotropicWrap, pin.TexC) * gDiffuseAlbedo;

    float4 diffuseAlbedo = gDiffuseAlbedo;
    //float4 diffuseAlbedo =  gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
    float3 fresnelR0 = gFresnelR0;
    float  roughness = gRoughness;
    //uint diffuseMapIndex = gDiffuseMapIndex;
    //uint normalMapIndex = gNormalMapIndex;

    // Dynamically look up the texture in the array.
    diffuseAlbedo *= gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);

#ifdef ALPHA_TEST
    // Discard pixel if texture alpha < 0.1.  We do this test as soon 
    // as possible in the shader so that we can potentially exit the
    // shader early, thereby skipping the rest of the shader code.
    clip(diffuseAlbedo.a - 0.1f);
#endif

    // Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);


    float4 normalMapSample = gNormalMap.Sample(gsamAnisotropicWrap, pin.TexC);
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);

    // Uncomment to turn off normal mapping.
    //bumpedNormalW = pin.NormalW;


    // Vector from point being lit to eye. 
    float3 toEyeW = gEyePosW - pin.PosW;
    float distToEye = length(toEyeW);
    toEyeW /= distToEye; // normalize

    // Finish texture projection and sample SSAO map.
    pin.SsaoPosH /= pin.SsaoPosH.w;

    float ambientAccess = 1.0f;

#ifdef SSAO    
    ambientAccess = gSsaoMap.Sample(gsamLinearClamp, pin.SsaoPosH.xy, 0.0f).r;
#endif
    // Light terms.
    float4 ambient = ambientAccess * gAmbientLight * diffuseAlbedo;


    const float shininess = (1.0f - roughness) * (normalMapSample.a * 1.0f);
    Material mat = { diffuseAlbedo, gFresnelR0, shininess, gRoughness, gMetal };
    
    //float3 shadowFactor = 1.0f;

    // Only the first light casts a shadow.
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    
    float result = CalcShadowFactor(pin.ShadowPosH);

    shadowFactor[0] = result;
    //shadowFactor[1] = result;
    //shadowFactor[2] = result;


    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        bumpedNormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // Add in specular reflections.
    float3 r = reflect(-toEyeW, bumpedNormalW);
    //float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
    float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    litColor.rgb += shininess * fresnelFactor; // *reflectionColor.rgb;
    

#ifdef FOG
    float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
    litColor = lerp(litColor, gFogColor, fogAmount);
#endif

    // Common convention to take alpha from diffuse albedo.
    litColor.a = diffuseAlbedo.a;

    return litColor;
}



