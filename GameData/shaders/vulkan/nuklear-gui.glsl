#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef SHADER_STAGE_VERTEX

	layout(push_constant) uniform PushConsts
	{
		mat4 projMat;
		float guiElementDepth;
	} pushConsts;

	layout(location = 0) in vec2 inVertex;
	layout(location = 1) in vec2 inUV;
	layout(location = 2) in vec4 inColor;

	layout(location = 0) out vec3 outUV_Depth;
	layout(location = 1) out vec4 outColor;

	out gl_PerVertex
	{
		vec4 gl_Position;
	};
		
	void main()
	{	
		gl_Position = pushConsts.projMat * vec4(inVertex.xy, 0, 1);
		
		outUV_Depth = vec3(inUV, pushConsts.guiElementDepth);
		outColor = inColor;
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	layout(set = 0, binding = 0) uniform sampler2D nuklearTexture;

	layout(location = 0) in vec3 inUV_Depth;
	layout(location = 1) in vec4 inColor;
	
	layout(location = 0) out vec4 outputFragColor;

	void main()
	{	
		gl_FragDepth = inUV_Depth.z;
	
		outputFragColor = inColor * texture(nuklearTexture, inUV_Depth.xy);
	}

#endif