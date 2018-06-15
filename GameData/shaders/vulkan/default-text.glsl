#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef SHADER_STAGE_VERTEX

	layout(location = 0) in vec3 inVertexPosition;
	layout(location = 1) in vec3 inUV;
	
	out gl_PerVertex
	{
		vec4 gl_Position;
	};
	
	layout(location = 0) out vec3 outUV;
	
	void main()
	{	
		gl_Position = vec4(inVertexPosition, 1);
		
		outUV = inUV;
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	layout(set = 0, binding = 0) uniform sampler inputSampler;
	layout(set = 0, binding = 1) uniform texture2DArray fontPages;

	layout(location = 0) in vec3 inUV;

	layout(location = 0) out vec4 outColor;
	
	void main()
	{
		outColor = vec4(vec3(1), texture(sampler2DArray(fontPages, inputSampler), inUV).r);
	}
	
#endif
