#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef SHADER_STAGE_VERTEX

	layout(location = 0) out vec2 outUV;

	out gl_PerVertex
	{
		vec4 gl_Position;
	};
	
	const vec2 positions[6] = vec2[](
		vec2(-1, -1),
		vec2(1, -1),
		vec2(-1, 1),

		vec2(1, -1),
		vec2(1, 1),
		vec2(-1, 1)
	);
		
	void main()
	{	
		gl_Position = vec4(positions[gl_VertexIndex], 0, 1);
		
		outUV = positions[gl_VertexIndex] * 0.5f + 0.5f;		
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	layout(location = 0) out vec4 fragColor;

	layout(set = 0, binding = 0) uniform sampler inputSampler;
	layout(set = 0, binding = 1) uniform texture2D deferredOutputTexture;
	
	layout(location = 0) in vec2 inUV;
	
	vec3 reinhard (in vec3 hdrColor, in float exposure, in float gamma);
	vec3 exponential (in vec3 hdrColor, in float exposure, in float gamma);
	vec3 hejl_dawson (in vec3 hdrColor, in float exposure, in float gamma);
	vec3 uncharted2 (in vec3 hdrColor, in float exposure, in float gamma);
	vec3 aces (in vec3 hdrColor, in float exposure, in float gamma);
	
	#define TONEMAP_FUNCTION exponential
	
	void main()
	{
		vec3 deferredOutput = texture(sampler2D(deferredOutputTexture, inputSampler), inUV).rgb;
		
		vec3 finalColor = deferredOutput;
		
		const float exposure = 0.025f;
		const float gamma = 2.2f;
		
		//finalColor = TONEMAP_FUNCTION(finalColor, exposure, gamma);
		
		/*
		switch (int(floor(inUV.x * 2.0f)))
		{
			case 0:
				finalColor = hejl_dawson(finalColor, exposure, gamma);
				break;
			case 1:
				finalColor = aces(finalColor, exposure, gamma);
				break;
			case 2:
				finalColor = hejl_dawson(finalColor, exposure, gamma);
				break;
			case 3:
				finalColor = uncharted2(finalColor, exposure, gamma);
				break;
			case 4:
				finalColor = aces(finalColor, exposure, gamma);
				break;
		}
		*/
		//finalColor = mix(hejl_dawson(finalColor, exposure, gamma), aces(finalColor, exposure, gamma), 0);
		finalColor = aces(finalColor, exposure, gamma);
		
		fragColor = vec4(finalColor, 1);
	}

	vec3 reinhard (in vec3 hdrColor, in float exposure, in float gamma)
	{
		return pow((hdrColor * exposure) / (vec3(1) + hdrColor * exposure), vec3(1 / gamma));
	}
	
	vec3 exponential (in vec3 hdrColor, in float exposure, in float gamma)
	{
		return pow(vec3(1) - exp(-hdrColor * exposure), vec3(1 / gamma));
	}
	
	vec3 hejl_dawson (in vec3 hdrColor, in float exposure, in float gamma)
	{
		vec3 x = max(vec3(0), hdrColor * exposure - vec3(0.004f));
		return (x*(6.2*x+.5))/(x*(6.2*x+1.7)+0.06);
	}
	
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	
	vec3 Uncharted2Tonemap(vec3 x)
	{
	   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
	}
	
	vec3 uncharted2 (in vec3 hdrColor, in float exposure, in float gamma)
	{
		vec3 curr = Uncharted2Tonemap(hdrColor * exposure * 2.0f);
		vec3 whiteScale = 1.0f / Uncharted2Tonemap(vec3(W));
		
		return pow(curr * whiteScale, vec3(1 / gamma));
	}
	
	vec3 ToneMap_ACESFilmic(vec3 hdr) 
	{
		const float a = 2.51f;
		const float b = 0.03f;
		const float c = 2.43f;
		const float d = 0.59f;
		const float e = 0.14f;

		return (hdr * (a * hdr + b)) 
			 / (hdr * (c * hdr + d) + e);
	}

	vec3 aces (in vec3 hdrColor, in float exposure, in float gamma)
	{
		return pow(ToneMap_ACESFilmic(hdrColor * exposure), vec3(1 / gamma));
	}
	
#endif
