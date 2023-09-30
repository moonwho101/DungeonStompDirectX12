// Modified by Mark Longo 2020

Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

struct VS_OUTPUT
{
	float4 pos: SV_POSITION;
	float4 color: COLOR;
	float2 texCoord: TEXCOORD;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{

	float4 result = t1.Sample(s1, input.texCoord).r;

	//drop black alpha for dice and hud
	//clip(result.a - 0.1f);

	return result;
}