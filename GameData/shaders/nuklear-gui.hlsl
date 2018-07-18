struct VSIn
{
	float2 vertex : POSITION0;
	float2 uv : POSITION1;
	float4 color : POSITION2;
};

struct VSOut
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 color : COLOR0;
};

cbuffer PushConsts : register(b0)
{
	float4x4 projMat;
	float guiElementDepth;
};

VSOut NuklearGUIVS(VSin inData)
{
	VSOut outData;
	outData.position = projMat * float4(inData.vertex, guiElementDepth, 1);
	outData.uv = inData.uv;
	outData.color = inData.color;
	
	return outData;
}

struct PSIn
{
	float2 uv : TEXCOORD0;
	float4 color : COLOR0;
};

float4 NuklearGUIPS(PSIn inData)
{
	
}