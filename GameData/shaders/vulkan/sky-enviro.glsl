#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConsts
{
	mat4 invCamMVPMat;
	float roughness;

} pushConsts;

#ifdef SHADER_STAGE_VERTEX

	layout(location = 0) out vec3 outRay;
	layout(location = 1) out float px;

	out gl_PerVertex
	{
		vec4 gl_Position;
	};

	vec2 positions[6] = vec2[](
		vec2(-1, -1),
		vec2(1, -1),
		vec2(-1, 1),

		vec2(1, -1),
		vec2(1, 1),
		vec2(-1, 1)
	);

	void main()
	{
		vec2 inPosition = positions[gl_VertexIndex];

		gl_Position = vec4(inPosition.xy, 0, 1);

		outRay = vec3(pushConsts.invCamMVPMat * vec4(inPosition.xy, 0, 1)) * vec3(1, 1, -1);
		px = inPosition.x;
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	layout(set = 0, binding = 0) uniform sampler inputSampler;
	layout(set = 0, binding = 1) uniform textureCube enviroMap;

	layout(location = 0) in vec3 inRay;
	layout(location = 1) in float px;

	layout(location = 0) out vec4 fragColor;
	
	#define PI 3.1415926535
	
	vec2 hammersley(uint i, uint n);
	
	vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness);
	
	float DistributionGGX(float NdotH, float roughness);
	
	void main()
	{
		vec3 N = normalize(inRay);
		vec3 R = N;
		vec3 V = R;
	
		const uint sampleCount = 32u;
		float totalWeight = 0;
		vec3 color = vec3(0);
		
		for (uint i = 0u; i < sampleCount; i++)
		{
			vec2 Xi = hammersley(i, sampleCount);
			vec3 H = importanceSampleGGX(Xi, N, pushConsts.roughness);
			vec3 L = normalize(-reflect(V, H));
			float a = pushConsts.roughness * pushConsts.roughness;
			L = mix(N, L, (1.0f - a) * (sqrt(1.0f - a) + a));
			
			float NdotL = max(dot(N, L), 0.0f);
			if (NdotL > 0.0f)
			{
				float NdotH = max(dot(N, H), 0.0f);
				float VdotH = max(dot(V, H), 0.0f);
				float pdf = DistributionGGX(NdotH, pushConsts.roughness);
				float omegaS = 1.0f / (float(sampleCount) * pdf);
				float omegaP = 4.0f * PI / (6.0f * 256.0f * 256.0f);
				float mipBias = 1.0f;
				float mipLevel = max(0.5f * log2(omegaS / omegaP) + mipBias, 0);
			
				color += textureLod(samplerCube(enviroMap, inputSampler), L, mipLevel).rgb * NdotL;
				totalWeight += NdotL;
			}
		}
		
		fragColor = vec4(color / totalWeight, 1);
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
	
	vec2 hammersley(uint i, uint n)
	{
		return vec2(float(i) / float(n), float(bitfieldReverse(i)) * 2.3283064365386963e-10);
	}
	
	float randAngle()
	{
	  uint x = uint(gl_FragCoord.x);
	  uint y = uint(gl_FragCoord.y);
	  return float(30u * x ^ y + 10u * x * y);
	}
	
	vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness)
	{
		float a = roughness * roughness;
		float phi = 2.0f * PI * Xi.x;// + randAngle();
		float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
		float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
		
		vec3 H;
		H.x = cos(phi) * sinTheta;
		H.y = sin(phi) * sinTheta;
		H.z = cosTheta;
		
		vec3 up = abs(N.z) < 0.9999f ? vec3(0, 0, 1) : vec3(1, 0, 0);
		vec3 tangent = normalize(cross(up, N));
		vec3 bitangent = cross(N, tangent);
		
		vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
		return normalize(sampleVec);
	}

#endif
