struct VSOut
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

const float2 positions[6] = {
	float2(-1, -1),
	float2(-1, 1),
	float2(1, -1),

	float2(1, -1),
	float2(-1, 1),
	float2(1, 1)
};

VSOut CombineVS(uint vertexID : SV_VertexID)
{
	VSOut vsData;
	vsData.position = float4(positions[vertexID], 0, 1);
	vsData.texcoord = positions[vertexID] * 0.5f + 0.5f;
	
	return vsData;
}

[[vk::binding(0, 0)]]
SamplerState inputSampler : register(s0);

[[vk::binding(1, 0)]]
Texture2D deferredLightingOutput : register(t1);

float3 aces (in float3 hdrColor, in float exposure, in float gamma);

float4 CombineFS(float2 texcoord : TEXCOORD0)
{
	float3 deferredOutputValue = deferredLightingOutput.Sample(inputSampler, texcoord).xyz;
	const float exposure = 0.025f;
	const float gamma = 2.2f;
	
	float3 finalColor = aces(deferredOutputValue, exposure, gamma);
	
	return float4(finalColor, 1);
}

float3 ToneMap_ACESFilmic(float3 hdr) 
{
	const float a = 2.51f;
	const float b = 0.03f;
	const float c = 2.43f;
	const float d = 0.59f;
	const float e = 0.14f;

	float3 num = (hdr * (a * hdr + b));
	float3 denom = (hdr * (c * hdr + d) + e);
	return num * rcp(denom);
}

float3 aces (in float3 hdrColor, in float exposure, in float gamma)
{
	return pow(ToneMap_ACESFilmic(hdrColor * exposure), float3(1 / gamma));
}