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

	layout(location = 0) in vec2 inUV;
	
	layout(set = 0, binding = 0) uniform sampler inputSampler;
	layout(set = 0, binding = 1) uniform texture2D gbuffer_AlbedoRoughnessTexture; // rgb - albedo, a - roughness
	layout(set = 0, binding = 2) uniform texture2D gbuffer_NormalsMetalnessTexture; // rgb - normals, a - metalness
	
	layout(location = 0) out vec4 out_color;

	void main()
	{	
		vec4 gbuffer_AlbedoRoughness = texture(sampler2D(gbuffer_AlbedoRoughnessTexture, inputSampler), inUV);
		vec4 gbuffer_NormalsMetalness = texture(sampler2D(gbuffer_NormalsMetalnessTexture, inputSampler), inUV);
		// Normals are packed [0,1] in the gbuffer, need to put them back to [-1,1]
		gbuffer_NormalsMetalness.xyz = normalize(gbuffer_NormalsMetalness.xyz * 2.0f - 1.0f);
	
		vec3 testLightDir = normalize(vec3(1, 0.5f, 0.0f));
		float h = clamp(dot(testLightDir, gbuffer_NormalsMetalness.xyz), 0.2f, 1);
	
		out_color = vec4(gbuffer_AlbedoRoughness.rgb * h, 1);
	}

#endif