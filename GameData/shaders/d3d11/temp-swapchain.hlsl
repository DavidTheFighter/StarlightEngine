float4 SwapchainVS(float4 inPos : POSITION) : SV_POSITION
{
    return inPos;
}

struct PS_OUT
{
	float4 color : SV_TARGET0;
};

PS_OUT SwapchainPS()
{
	PS_OUT output;
	output.color = float4(1, 0, 0, 1);

	return output;
}