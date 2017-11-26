#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef SHADER_STAGE_VERTEX

	layout(push_constant) uniform PushConsts
	{
		mat4 mvp;
	} pushConsts;
	
	layout(set = 0, binding = 0) uniform sampler heightmapSampler;
	layout(set = 0, binding = 1) uniform texture2D heightmap;

	layout(location = 0) in vec3 inVertex;
	layout(location = 1) in vec2 inUV;
	layout(location = 2) in vec3 inNormal;
	layout(location = 3) in vec3 inTangent;

	layout(location = 0) out vec2 outUV;
	layout(location = 1) out vec3 outNormal;
	layout(location = 2) out vec3 outTangent;

	out gl_PerVertex
	{
		vec4 gl_Position;
	};

	void main()
	{
		vec2 heightmapTexcoord = inVertex.xz / 256.0f;
		float heightmapValue = texture(sampler2D(heightmap, heightmapSampler), heightmapTexcoord).x;
	
		gl_Position = pushConsts.mvp * vec4(inVertex + vec3(0, heightmapValue * 8192.0f - 4096.0f, 0), 1);
		
		vec3 offset = vec3(-1 / 513.0f, 0, 1 / 513.0f) * 2.0f;
		float size = 4.0f;
		float s01 = texture(sampler2D(heightmap, heightmapSampler), heightmapTexcoord + offset.xy).x * 8192.0f - 4096.0f;
		float s21 = texture(sampler2D(heightmap, heightmapSampler), heightmapTexcoord + offset.zy).x * 8192.0f - 4096.0f;
		float s10 = texture(sampler2D(heightmap, heightmapSampler), heightmapTexcoord + offset.yx).x * 8192.0f - 4096.0f;
		float s12 = texture(sampler2D(heightmap, heightmapSampler), heightmapTexcoord + offset.yz).x * 8192.0f - 4096.0f;
		vec3 va = normalize(vec3(size, s21-s01, 0));
		vec3 vb = normalize(vec3(0, s12-s10, -size));
		vec3 triangleNormal = cross(va, vb);
		
		outUV = inUV;
		outNormal = triangleNormal;//inNormal;
		outTangent = inTangent;
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	layout(location = 0) in vec2 inUV;
	layout(location = 1) in vec3 inNormal;
	layout(location = 2) in vec3 inTangent;
	
	layout(location = 0) out vec4 albedo_roughness; // rgb - albedo, a - roughness
	layout(location = 1) out vec4 normal_metalness; // rgb - normals, a - metalness

	void main()
	{
		float h = clamp(dot(normalize(inNormal), normalize(vec3(1, 1, 0))), 0.3f, 1.0f);
		albedo_roughness = vec4(vec3(0, 0.5f, 0) * h, 1.0f);
		normal_metalness = vec4(inNormal, 0.0f);
	}

#endif