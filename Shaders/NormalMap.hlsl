//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

#include "Common.hlsl"
 
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
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    
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



