#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef SHADER_STAGE_VERTEX

	layout(push_constant) uniform PushConsts
	{
		mat4 mvp;
	} pushConsts;
	
	layout(location = 0) in vec3 inVertex;
	layout(location = 1) in vec2 inUV;
	layout(location = 2) in vec3 inNormal;
	layout(location = 3) in vec3 inTangent;
	layout(location = 4) in vec4 inInstancePosition_Scale;
	layout(location = 5) in vec4 inInstanceRotation;

	layout(location = 0) out vec2 outUV;
	layout(location = 1) out vec3 outNormal;
	layout(location = 2) out vec3 outTangent;

	out gl_PerVertex
	{
		vec4 gl_Position;
	};
		
	void main()
	{	
		gl_Position = pushConsts.mvp * vec4(inVertex + inInstancePosition_Scale.xyz + vec3(1), 1);
		
		outUV = inUV;
		outNormal = inNormal;
		outTangent = inTangent;
		
		//texcoord = inPosition;
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	layout(set = 0, binding = 0) uniform sampler materialSampler;
	layout(set = 0, binding = 1) uniform texture2D materialTex0; // Albedo
	layout(set = 0, binding = 2) uniform texture2D materialTex1; // Normals
	layout(set = 0, binding = 3) uniform texture2D materialTex2; // Roughness
	layout(set = 0, binding = 4) uniform texture2D materialTex3; // Metalness
	layout(set = 0, binding = 5) uniform texture2D materialTex4; // Unused for now (probably ao in the future)

	layout(location = 0) in vec2 inUV;
	layout(location = 1) in vec3 inNormal;
	layout(location = 2) in vec3 inTangent;
	
	layout(location = 0) out vec4 diffuse_roughness;
	layout(location = 1) out vec4 normal_metalness;

	void main()
	{	
		diffuse_roughness = vec4(texture(sampler2D(materialTex0, materialSampler), inUV).rgb, texture(sampler2D(materialTex2, materialSampler), inUV).r);
		normal_metalness = vec4(normalize(inNormal) * 0.5f + 0.5f, texture(sampler2D(materialTex3, materialSampler), inUV).r);
	}

#endif