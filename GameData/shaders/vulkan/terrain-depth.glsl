#version 450
#extension GL_ARB_separate_shader_objects : enable

#define CLIPMAP_SLICE_SIZE 1024

#ifdef SHADER_STAGE_VERTEX

	layout(push_constant) uniform PushConsts
	{
		mat4 mvp;
		float clipmapArrayLayer;
	} pushConsts;
	
	layout(location = 0) in vec2 inVertex;

	layout(set = 0, binding = 0) uniform sampler heightmapSampler;
	layout(set = 0, binding = 1) uniform texture2DArray heightmap;
	
	out gl_PerVertex
	{
		vec4 gl_Position;
	};
		
		
	void main()
	{	
		float height = texture(sampler2DArray(heightmap, heightmapSampler), vec3(inVertex / CLIPMAP_SLICE_SIZE, pushConsts.clipmapArrayLayer)).r * 8192.0f - 4096.0f;
	
		gl_Position = pushConsts.mvp * vec4(inVertex.x, height, inVertex.y, 1);
	}

#elif defined(SHADER_STAGE_FRAGMENT)
	
	void main()
	{	
	}

#endif
