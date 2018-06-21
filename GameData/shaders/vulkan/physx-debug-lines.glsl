#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef SHADER_STAGE_VERTEX

	layout(push_constant) uniform PushConsts
	{
		mat4 mvp;
		vec4 cameraPosition;
		vec4 cameraCellOffset;
	} pushConsts;

	layout(location = 0) in vec3 inVertexPosition;
	layout(location = 1) in uint inColorHex;

	layout(location = 0) out vec3 outVertexColor;

	out gl_PerVertex
	{
		vec4 gl_Position;
	};
		
	void main()
	{	
		gl_Position = pushConsts.mvp * vec4(inVertexPosition - pushConsts.cameraCellOffset.xyz, 1);
		
		float a = (inColorHex & 0xFF000000) / 255.0f;
		float r = (inColorHex & 0x00FF0000) / 255.0f;
		float g = (inColorHex & 0x0000FF00) / 255.0f;
		float b = (inColorHex & 0x000000FF) / 255.0f;
		
		outVertexColor = vec3(r, g, b) * a;
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	layout(location = 0) in vec3 inVertexColor;

	layout(location = 0) out vec4 albedo_roughness; // rgb - albedo, a - roughness
	layout(location = 1) out vec4 normal_metalness; // rgb - normals, a - metalness	
	
	vec2 encodeNormal (in vec3 normal)
	{
		vec3 n = normal / (abs(normal.x) + abs(normal.y) + abs(normal.z));
		n.xy = n.z >= 0.0 ? n.xy : ((vec2(1) - abs(n.yx)) * (all(greaterThanEqual(n.xy, vec2(0))) ? 1.0f : -1.0f));
		
		return n.xy * 0.5f + 0.5f;
	}
	
	void main()
	{
		albedo_roughness = vec4(inVertexColor, 1);
		normal_metalness = vec4(encodeNormal(vec3(0, 1, 0)), 1, 0);
	}
	
#endif
