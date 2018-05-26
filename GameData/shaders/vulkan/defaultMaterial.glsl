#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef SHADER_STAGE_VERTEX

	layout(push_constant) uniform PushConsts
	{
		mat4 mvp;
		vec4 cameraPosition;
		vec4 cameraCellOffset;
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
	layout(location = 3) out float px;
	layout(location = 4) out vec3 vertex;
	layout(location = 5) out vec3 camPos;

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
		gl_Position = pushConsts.mvp * vec4(rotateByQuaternion(inVertex * inInstancePosition_Scale.w / 10.0, inInstanceRotation) + inInstancePosition_Scale.xyz - pushConsts.cameraCellOffset.xyz, 1);
		vertex = rotateByQuaternion(inVertex * inInstancePosition_Scale.w / 10.0, inInstanceRotation) + inInstancePosition_Scale.xyz;
		camPos = pushConsts.cameraPosition.xyz;
		
		outUV = inUV;
		outNormal = rotateByQuaternion(inNormal, inInstanceRotation);
		outTangent = rotateByQuaternion(inTangent, inInstanceRotation);
		px = gl_Position.x;
		
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
	layout(location = 3) in float px;
	layout(location = 4) in vec3 vertex;
	layout(location = 5) in vec3 camPos;
	
	layout(location = 0) out vec4 albedo_roughness; // rgb - albedo, a - roughness
	layout(location = 1) out vec4 normal_metalness; // rgb - normals, a - metalness

	vec3 calcNormal(in vec2 texcoords)
	{
		vec3 N = normalize(inNormal);
		vec3 T = normalize(inTangent);
		vec3 B = cross(N, T);
		mat3 tbn = mat3(T, B, N);
		
		return tbn * normalize(texture(sampler2DArray(materialTex, materialSampler), vec3(texcoords, 1)).rgb * 2.0f - 1.0f);
	}

	vec2 encodeNormal (in vec3 normal)
	{
		float p = sqrt(normal.z * 8.0f + 8.0f);
		return vec2(normal.xy / p + 0.5f);
	}
	
	void main()
	{	
		vec2 texcoords = inUV;
	
		if (true)
		{
			vec3 N = normalize(inNormal);
			vec3 T = normalize(inTangent);
			vec3 B = cross(N, T);
			mat3 TBN = mat3(T, B, N);
		
			vec3 viewDir = normalize(TBN * camPos - TBN * vertex);
			float height_scale = 0.1f;
			
			const float numLayers = 12;
			float layerDepth = 1 / numLayers;
			float currentLayerDepth = 0;
		
			vec2 P = viewDir.xy * height_scale;
			vec2 deltaTexCoords = P / numLayers * vec2(1, -1);
		
			vec2 currentTexCoords = inUV;
			float currentDepth = texture(sampler2DArray(materialTex, materialSampler), vec3(currentTexCoords, 5)).r;
			
			while (currentLayerDepth < currentDepth)
			{
				currentTexCoords -= deltaTexCoords;
				currentDepth = texture(sampler2DArray(materialTex, materialSampler), vec3(currentTexCoords, 5)).r;
				currentLayerDepth += layerDepth;
			}
			
			vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
			
			float afterDepth = currentDepth - currentLayerDepth;
			float beforeDepth = texture(sampler2DArray(materialTex, materialSampler), vec3(prevTexCoords, 5)).r - currentLayerDepth + layerDepth;
			
			float weight = afterDepth / (afterDepth - beforeDepth);
			
			texcoords = prevTexCoords * weight + currentTexCoords * (1 - weight);
		}
		
		albedo_roughness = vec4(texture(sampler2DArray(materialTex, materialSampler), vec3(texcoords, 0)).rgb, texture(sampler2DArray(materialTex, materialSampler), vec3(texcoords, 2)).r);
		normal_metalness = vec4(encodeNormal(calcNormal(texcoords)), texture(sampler2DArray(materialTex, materialSampler), vec3(texcoords, 4)).r, texture(sampler2DArray(materialTex, materialSampler), vec3(texcoords, 3)).r);
	}

#endif