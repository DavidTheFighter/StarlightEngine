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

	layout(location = 0) out vec2 outUV;
	layout(location = 1) out vec3 outNormal;
	layout(location = 2) out vec3 outTangent;

	out gl_PerVertex
	{
		vec4 gl_Position;
	};
		
	void main()
	{	
		gl_Position = pushConsts.mvp * vec4(inVertex, 1);
		
		outUV = inUV;
		outNormal = inNormal;
		outTangent = inTangent;
		
		//texcoord = inPosition;
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	layout(set = 0, binding = 0) uniform sampler2D material_tex0;

	layout(location = 0) in vec2 inUV;
	layout(location = 1) in vec3 inNormal;
	layout(location = 2) in vec3 inTangent;
	
	layout(location = 0) out vec4 diffuse_roughness;
	layout(location = 1) out vec4 normal_metalness;

	void main()
	{	
		diffuse_roughness = vec4(texture(material_tex0, inUV).rgb, 1);
		normal_metalness = vec4(normalize(inNormal) * 0.5f + 0.5f, 0);
	}

#endif