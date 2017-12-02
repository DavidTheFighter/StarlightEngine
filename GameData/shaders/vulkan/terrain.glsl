#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef SHADER_STAGE_VERTEX

	layout(push_constant) uniform PushConsts
	{
		mat4 mvp;
		vec2 cameraCellCoord;
		vec2 cellCoordStart;
		int instanceCountWidth;
	} pushConsts;
	
	layout(set = 0, binding = 0) uniform sampler heightmapSampler;
	layout(set = 0, binding = 1) uniform texture2DArray heightmap;

	layout(location = 0) in vec3 inVertex;
	layout(location = 1) in vec2 inUV;
	layout(location = 2) in vec3 inNormal;
	layout(location = 3) in vec3 inTangent;

	layout(location = 0) out vec2 outUV;
	layout(location = 1) out vec3 outNormal;
	layout(location = 2) out vec3 outTangent;
	layout(location = 3) out vec3 outTest;

	out gl_PerVertex
	{
		vec4 gl_Position;
	};

	void main()
	{
		float cameraCellCoordOffset;
		float clipmapScale;
		float clipmapArrayLayer;
	
		vec2 cellCoordOffset = pushConsts.cellCoordStart + vec2(gl_InstanceIndex % pushConsts.instanceCountWidth, gl_InstanceIndex / pushConsts.instanceCountWidth);
		float distToCamera = distance(cellCoordOffset * 256.0f, pushConsts.cameraCellCoord * 256.0f + 128.0f);
		//float distToCamera = max(abs(cellCoordOffset.x * 256.0f - (pushConsts.cameraCellCoord.y * 256.0f + 128.0f)), abs(cellCoordOffset.y * 256.0f - (pushConsts.cameraCellCoord.y * 256.0f + 128.0f)));
		
		if (distToCamera < 256.0f)
		{
			outTest = vec3(0, 0.95f, 0);
			
			cameraCellCoordOffset = 0;
			clipmapScale = 1;
			clipmapArrayLayer = 0;
		}
		else if (distToCamera < 512.0f)
		{
			outTest = vec3(1, 1, 0.1f);
			
			cameraCellCoordOffset = -1;
			clipmapScale = 2;
			clipmapArrayLayer = 1;
		}
		else if (distToCamera < 1024.0f)
		{
			outTest = vec3(0.4f, 0.4f, 1);
			
			cameraCellCoordOffset = -3;
			clipmapScale = 4;
			clipmapArrayLayer = 2;
		}
		else if (distToCamera < 2048.0f)
		{
			outTest = vec3(1, 0.6f, 0.6f);
			
			cameraCellCoordOffset = -7;
			clipmapScale = 8;
			clipmapArrayLayer = 3;
		}
		else if (distToCamera < 4096.0f)
		{
			outTest = vec3(1, 0.5f, 0.05f);
			
			cameraCellCoordOffset = -15;
			clipmapScale = 16;
			clipmapArrayLayer = 4;
		}
		else
		{
			outTest = vec3(0.9f, 0, 0);
			
			cameraCellCoordOffset = -15;
			clipmapScale = 16;
			clipmapArrayLayer = 4;
		}
		
		vec2 heightmapTexcoord = (inVertex.xz + cellCoordOffset * 256.0f - (pushConsts.cameraCellCoord + vec2(cameraCellCoordOffset)) * 256.0f) / (512.0f * clipmapScale);
		float heightmapValue = texture(sampler2DArray(heightmap, heightmapSampler), vec3(heightmapTexcoord, clipmapArrayLayer)).x;
	
		// * vec3(0.9f, 1, 0.9f)
		gl_Position = pushConsts.mvp * vec4(inVertex + vec3(cellCoordOffset.x * 256.0f, heightmapValue * 8192.0f - 4096.0f, cellCoordOffset.y * 256.0f), 1);
		
		vec3 offset = vec3(-1 / 513.0f, 0, 1 / 513.0f) * 2.0f;
		float size = 2.0f * clipmapScale;
		float s01 = texture(sampler2DArray(heightmap, heightmapSampler), vec3(heightmapTexcoord + offset.xy, clipmapArrayLayer)).x * 8192.0f - 4096.0f;
		float s21 = texture(sampler2DArray(heightmap, heightmapSampler), vec3(heightmapTexcoord + offset.zy, clipmapArrayLayer)).x * 8192.0f - 4096.0f;
		float s10 = texture(sampler2DArray(heightmap, heightmapSampler), vec3(heightmapTexcoord + offset.yx, clipmapArrayLayer)).x * 8192.0f - 4096.0f;
		float s12 = texture(sampler2DArray(heightmap, heightmapSampler), vec3(heightmapTexcoord + offset.yz, clipmapArrayLayer)).x * 8192.0f - 4096.0f;
		vec3 va = normalize(vec3(size, s21-s01, 0));
		vec3 vb = normalize(vec3(0, s12-s10, -size));
		vec3 triangleNormal = cross(va, vb);
		
		outUV = inUV;
		outNormal = triangleNormal;
		outTangent = inTangent;
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	layout(location = 0) in vec2 inUV;
	layout(location = 1) in vec3 inNormal;
	layout(location = 2) in vec3 inTangent;
	layout(location = 3) in vec3 inTest;

	layout(location = 0) out vec4 albedo_roughness; // rgb - albedo, a - roughness
	layout(location = 1) out vec4 normal_metalness; // rgb - normals, a - metalness

	void main()
	{
		float h = clamp(dot(normalize(inNormal), normalize(vec3(0, 1, 0))), 0.3f, 1.0f);
		albedo_roughness = vec4(inTest * h, 1.0f);
		normal_metalness = vec4(inNormal, 0.0f);
	}

#endif