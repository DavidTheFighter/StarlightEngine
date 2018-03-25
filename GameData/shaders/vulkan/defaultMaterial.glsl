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
	layout(location = 4) in vec4 inInstancePosition_Scale; // xyz - position, w - scale
	layout(location = 5) in vec4 inInstanceRotation; // quaternion

	layout(location = 0) out vec2 outUV;
	layout(location = 1) out vec3 outNormal;
	layout(location = 2) out vec3 outTangent;

	out gl_PerVertex
	{
		vec4 gl_Position;
	};
		
	vec3 rotateByQuaternion(in vec3 point, in vec4 quat)
	{
		return 2.0f * cross(quat.xyz, quat.w * point + cross(quat.xyz, point)) + point;
	}
		
	void main()
	{	
		gl_Position = pushConsts.mvp * vec4(rotateByQuaternion(inVertex * inInstancePosition_Scale.w / 10.0, inInstanceRotation) + inInstancePosition_Scale.xyz, 1);
		
		outUV = inUV;
		outNormal = rotateByQuaternion(inNormal, inInstanceRotation);
		outTangent = rotateByQuaternion(inTangent, inInstanceRotation);
		
		//texcoord = inPosition;
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	layout(set = 0, binding = 0) uniform sampler materialSampler;
	layout(set = 0, binding = 1) uniform texture2DArray materialTex; // Albedo, Normals, Roughness, Metalness, Unusued (maybe AO or something)
	//layout(set = 0, binding = 2) uniform texture2D materialTex1; // Normals
	//layout(set = 0, binding = 3) uniform texture2D materialTex2; // Roughness
	//layout(set = 0, binding = 4) uniform texture2D materialTex3; // Metalness
	//layout(set = 0, binding = 5) uniform texture2D materialTex4; // Unused for now (probably ao in the future)

	layout(location = 0) in vec2 inUV;
	layout(location = 1) in vec3 inNormal;
	layout(location = 2) in vec3 inTangent;
	
	layout(location = 0) out vec4 albedo_roughness; // rgb - albedo, a - roughness
	layout(location = 1) out vec4 normal_metalness; // rgb - normals, a - metalness

	vec3 calcNormal()
	{
		vec3 N = normalize(inNormal);
		vec3 T = normalize(inTangent);
		vec3 B = cross(N, T);
		mat3 tbn = mat3(T, B, N);
		
		return tbn * normalize(texture(sampler2DArray(materialTex, materialSampler), vec3(inUV, 1)).rgb * 2.0f - 1.0f);
	}

	void main()
	{	
		albedo_roughness = vec4(texture(sampler2DArray(materialTex, materialSampler), vec3(inUV, 0)).rgb, texture(sampler2DArray(materialTex, materialSampler), vec3(inUV, 2)).r);
		normal_metalness = vec4(calcNormal() * 0.5f + 0.5f, texture(sampler2DArray(materialTex, materialSampler), vec3(inUV, 3)).r);
	}

#endif