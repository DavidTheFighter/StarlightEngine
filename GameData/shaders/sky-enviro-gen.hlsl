struct CS_In
{
	uint3 dispatchThreadID : SV_DispatchThreadID;
};

[[vk::binding(0, 0)]]
SamplerState enviroSampler : register(s0);

[[vk::binding(1, 0)]]
TextureCube enviroMap : register(t1);

[[vk::binding(2, 0)]]
cbuffer FaceInvMVPs : register(b2)
{
	float4x4 invCamMVPMats[6];
};

[[vk::binding(0, 1)]]
RWTexture2DArray<float4> outEnviroSpecIBL : register(t3);

[[vk::push_constant]]
cbuffer PushConts : register(b4)
{
	float roughness;
	float invMipSize;
};

float DistributionGGX(float NdotH, float roughness);
float2 hammersley(uint i, uint n);
float3 importanceSampleGGX(float2 Xi, float3 N, float roughness);

#define PI 3.1415926

[numthreads(8, 8, 1)]
void EnviroSkyboxSpecIBLGen(CS_In csIn)
{
	float3 N = normalize(mul(invCamMVPMats[csIn.dispatchThreadID.z], float4(float(csIn.dispatchThreadID.x) * invMipSize * 2.0f - 1.0f, float(csIn.dispatchThreadID.y) * invMipSize * 2.0f - 1.0f, 0, 1)));
	N.z *= -1.0f;
	
	float3 R = N;
	float3 V = R;

	const uint sampleCount = 32u;
	float totalWeight = 0;
	float3 color = float3(0);
	
	for (uint i = 0u; i < sampleCount; i++)
	{
		float2 Xi = hammersley(i, sampleCount);
		float3 H = importanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(-reflect(V, H));
		float a = roughness * roughness;
		L = lerp(N, L, (1.0f - a) * (sqrt(1.0f - a) + a));
		
		float NdotL = max(dot(N, L), 0.0f);
		if (NdotL > 0.0f)
		{
			float NdotH = max(dot(N, H), 0.0f);
			float VdotH = max(dot(V, H), 0.0f);
			float pdf = DistributionGGX(NdotH, roughness);
			float omegaS = 1.0f / (float(sampleCount) * pdf);
			float omegaP = 4.0f * PI / (6.0f * 256.0f * 256.0f);
			float mipBias = 1.0f;
			float mipLevel = max(0.5f * log2(omegaS / omegaP) + mipBias, 0);
		
			color += enviroMap.SampleLevel(enviroSampler, L, mipLevel).xyz * NdotL;
			totalWeight += NdotL;
		}
	}
	
	outEnviroSpecIBL[uint3(csIn.dispatchThreadID.x, csIn.dispatchThreadID.y, csIn.dispatchThreadID.z)] = float4(color / totalWeight, 1);
}

float DistributionGGX(float NdotH, float roughness)
{
	float a = roughness*roughness;
	float a2 = a*a;
	float NdotH2 = NdotH*NdotH;

	float nom   = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

float2 hammersley(uint i, uint n)
{
	return float2(float(i) / float(n), float(reversebits(i)) * 2.3283064365386963e-10);
}

float3 importanceSampleGGX(float2 Xi, float3 N, float roughness)
{
	float a = roughness * roughness;
	float phi = 2.0f * PI * Xi.x;
	float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
	
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	float3 up = abs(N.z) < 0.9999f ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);
	
	float3 samplefloat = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(samplefloat);
}