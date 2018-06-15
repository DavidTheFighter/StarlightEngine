#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef SHADER_STAGE_VERTEX

	layout(location = 0) out vec2 texcoord;

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

		gl_Position = vec4(inPosition.xy, 1, 1);

		texcoord = inPosition * 0.5f + 0.5f;
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	layout(set = 0, binding = 0) uniform sampler2D sourceTexture;

	layout(location = 0) in vec2 texcoord;

	layout(location = 0) out vec4 fragColor;

	void main()
	{
		fragColor = texture(sourceTexture, texcoord);
	}

#endif
