struct VS_OUT
{
	float4 position : SV_POSITION;
};

VS_OUT SwapchainVS(float3 pos : POSITION)
{
	VS_OUT outData;
	outData.position = float4(pos, 1.0f);
	
	return outData;
}

struct PS_IN
{
	float4 position : SV_POSITION;
};

cbuffer ConstantBuffer : register(b0)
{
    float4 color;
};

Texture2D testTex : register(t1);
SamplerState testSampl : register(s2);

float4 SwapchainPS(PS_IN pIn) : SV_TARGET
{
	float4 texCol = testTex.Sample(testSampl, normalize(pIn.position.xy));
	return float4(color.xyz * texCol.xyz, 1.0f);
}