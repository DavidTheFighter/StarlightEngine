#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef SHADER_STAGE_VERTEX
	
	layout(location = 0) in vec2 inVertex;
	
	layout(location = 0) out vec3 outVertex;
	
	out gl_PerVertex
	{
		vec4 gl_Position;
	};

	void main()
	{
		gl_Position = vec4(inVertex, 0, 1);
		outVertex = vec3(inVertex, gl_InstanceIndex);
	}
	
#elif defined(SHADER_STAGE_TESSELLATION_CONTROL)

	layout (vertices = 4) out;

	layout(location = 0) in vec3 inVertex[];
	layout(location = 0) out vec3 outVertex[];

	layout(push_constant) uniform PushConsts
	{
		mat4 mvp;
		vec2 cameraCellCoord;
		vec2 cellCoordStart;
		vec4 cameraPosition;
		int instanceCountWidth;
	} pushConsts;

	#define ID gl_InvocationID

	float level(in vec3 vert);

	void main()
	{
		outVertex[ID] = inVertex[ID];
		
		if (ID == 0)
		{
			float e0 = level((inVertex[0] + inVertex[1]) * 0.5f);
			float e1 = level((inVertex[1] + inVertex[2]) * 0.5f);
			float e2 = level((inVertex[2] + inVertex[3]) * 0.5f);
			float e3 = level((inVertex[3] + inVertex[0]) * 0.5f);

			gl_TessLevelInner[0] = max((e3 + e2) * 0.5f, 1);
			gl_TessLevelInner[1] = max((e0 + e1) * 0.5f, 1);

			gl_TessLevelOuter[0] = e0;
			gl_TessLevelOuter[1] = e3;
			gl_TessLevelOuter[2] = e2;
			gl_TessLevelOuter[3] = e1;
		
			/*
			gl_TessLevelInner[0] = 2;
			gl_TessLevelInner[1] = 1;

			gl_TessLevelOuter[0] = 1;
			gl_TessLevelOuter[1] = 1;
			gl_TessLevelOuter[2] = 1;
			gl_TessLevelOuter[3] = 1;
			*/
		}
	}
	
	float level (in vec3 vert)
	{	
		vec2 cellCoordOffset = pushConsts.cellCoordStart + vec2(int(vert.z) % pushConsts.instanceCountWidth, int(vert.z) / pushConsts.instanceCountWidth);
		float distToCamera = distance(vert.xy + cellCoordOffset * 256.0f, pushConsts.cameraPosition.xz);
	
		const float dmax = 4096.0f;
		const float dmin = 256.0f;
		const float smult = 32.0f;
		const float spow = 1;
		const float lvlInc = 4.0f;
	
		float lvl = floor(smult * pow((dmax - clamp(distToCamera, dmin, dmax) - dmin) / (dmax - dmin), spow));
		
	
		return max(floor(lvl / lvlInc) * lvlInc, 1);
	}

#elif defined(SHADER_STAGE_TESSELLATION_EVALUATION)

	//equal_spacing
	//fractional_odd_spacing
	//fractional_even_spacing

	layout(push_constant) uniform PushConsts
	{
		mat4 mvp;
		vec2 cameraCellCoord;
		vec2 cellCoordStart;
		vec4 cameraPosition;
		int instanceCountWidth;
	} pushConsts;
	
	layout(set = 0, binding = 0) uniform sampler heightmapSampler;
	layout(set = 0, binding = 1) uniform texture2DArray heightmap;

	layout(quads, equal_spacing, ccw) in;
	
	layout(location = 0) in vec3 inVertex[];

	layout(location = 0) out vec3 outTest;
	layout(location = 1) out vec3 outNormal;
	layout(location = 2) out vec3 outHeightmapTexcoord;

	float getClipmapArrayLayer (float dist);

	void main()
	{
		vec3 meshVertex = mix(mix(inVertex[0], inVertex[3], gl_TessCoord.x), mix(inVertex[1], inVertex[2], gl_TessCoord.x), gl_TessCoord.y);
		vec3 vertex = vec3(meshVertex.x, 0, meshVertex.y);
				
		float cameraCellCoordOffset;
	
		vec2 cellCoordOffset = pushConsts.cellCoordStart + vec2(int(meshVertex.z) % pushConsts.instanceCountWidth, int(meshVertex.z) / pushConsts.instanceCountWidth);
		float distToCamera = distance(cellCoordOffset * 256.0f, pushConsts.cameraCellCoord * 256.0f + 128.0f);
		//float distToCamera = max(abs(cellCoordOffset.x * 256.0f - (pushConsts.cameraCellCoord.y * 256.0f + 128.0f)), abs(cellCoordOffset.y * 256.0f - (pushConsts.cameraCellCoord.y * 256.0f + 128.0f)));
		float clipmapArrayLayer = getClipmapArrayLayer(distToCamera);
		
		switch (int(clipmapArrayLayer))
		{
			case 0:
				outTest = vec3(0, 0.95f, 0);
				break;
			case 1:
				outTest = vec3(1, 1, 0.1f);
				break;
			case 2:
				outTest = vec3(0.4f, 0.4f, 1);
				break;
			case 3:
				outTest = vec3(1, 0.6f, 0.6f);
				break;
			case 4:
				if (distToCamera < 4096.0f)
					outTest = vec3(1, 0.5f, 0.05f);
				else
					outTest = vec3(0.9f, 0, 0);
				
				break;
		}
		
		switch (int(clipmapArrayLayer))
		{
			case 0:
				cameraCellCoordOffset = 0;
				break;
			case 1:
				cameraCellCoordOffset = -1;
				break;
			case 2:
				cameraCellCoordOffset = -3;
				break;
			case 3:
				cameraCellCoordOffset = -7;
				break;
			case 4:
				cameraCellCoordOffset = -15;
				break;
		}
		
		outHeightmapTexcoord = vec3((vertex.xz + cellCoordOffset * 256.0f - (pushConsts.cameraCellCoord + vec2(cameraCellCoordOffset)) * 256.0f) / (512.0f * pow(2, clipmapArrayLayer)), clipmapArrayLayer);
		
		if (vertex.x < 1e-6)
		{
			if (vertex.z < 1e-6)
			{					
				cellCoordOffset -= vec2(1, 1);
				vertex.xz = vec2(256, 256);

				clipmapArrayLayer = getClipmapArrayLayer(distance(cellCoordOffset * 256.0f, pushConsts.cameraCellCoord * 256.0f + 128.0f));
			}
			else
			{
				cellCoordOffset -= vec2(1, 0);
				vertex.x = 256;

				clipmapArrayLayer = getClipmapArrayLayer(distance(cellCoordOffset * 256.0f, pushConsts.cameraCellCoord * 256.0f + 128.0f));
			}
		}
		else if (vertex.z < 1e-6)
		{
			cellCoordOffset -= vec2(0, 1);
			vertex.z = 256;

			clipmapArrayLayer = getClipmapArrayLayer(distance(cellCoordOffset * 256.0f, pushConsts.cameraCellCoord * 256.0f + 128.0f));
		}
		
		switch (int(clipmapArrayLayer))
		{
			case 0:
				cameraCellCoordOffset = 0;
				break;
			case 1:
				cameraCellCoordOffset = -1;
				break;
			case 2:
				cameraCellCoordOffset = -3;
				break;
			case 3:
				cameraCellCoordOffset = -7;
				break;
			case 4:
				cameraCellCoordOffset = -15;
				break;
		}
		
		vec3 heightmapTexcoord = vec3((vertex.xz + cellCoordOffset * 256.0f - (pushConsts.cameraCellCoord + vec2(cameraCellCoordOffset)) * 256.0f) / (512.0f * pow(2, clipmapArrayLayer)), clipmapArrayLayer);
		//outHeightmapTexcoord = heightmapTexcoord;
		
		float heightmapValue = texture(sampler2DArray(heightmap, heightmapSampler), heightmapTexcoord).x;
		

		vertex.xz += cellCoordOffset * 256.0f;
		vertex.y += heightmapValue * 8192.0f - 4096.0f;
		
		gl_Position = pushConsts.mvp * vec4(vertex, 1);
	}

	float getClipmapArrayLayer (float distToCamera)
	{
		if (distToCamera < 256.0f)
		{
			return 0;
		}
		else if (distToCamera < 512.0f)
		{
			return 1;
		}
		else if (distToCamera < 1024.0f)
		{
			return 2;
		}
		else if (distToCamera < 2048.0f)
		{
			return 3;
		}
		else if (distToCamera < 4096.0f)
		{
			return 4;
		}
		else
		{
			return 4;
		}
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	layout(set = 0, binding = 0) uniform sampler heightmapSampler;
	layout(set = 0, binding = 1) uniform texture2DArray heightmap;	
	
	//layout(set = 0, binding = 2) uniform sampler textureSampler;
	//layout(set = 0, binding = 3) uniform texture2DArray terrainTextures[1];

	layout(location = 0) out vec4 albedo_roughness; // rgb - albedo, a - roughness
	layout(location = 1) out vec4 normal_metalness; // rgb - normals, a - metalness

	layout(location = 0) in vec3 inTest;
	//layout(location = 1) in vec3 inNormal;
	layout(location = 2) in vec3 inHeightmapTexcoord;

	void main()
	{
		//vec3 inNormal = vec3(0, 1, 0);
	
		vec3 heightmapTexcoord = vec3(inHeightmapTexcoord.x, inHeightmapTexcoord.y, inHeightmapTexcoord.z);
		
		float noffset = 1 / (512.0f * pow(2, heightmapTexcoord.z));
		float h01 = texture(sampler2DArray(heightmap, heightmapSampler), heightmapTexcoord + vec3(-noffset, 0, 0)).x * 8192.0f - 4096.0f;
		float h21 = texture(sampler2DArray(heightmap, heightmapSampler), heightmapTexcoord + vec3(noffset, 0, 0)).x * 8192.0f - 4096.0f;
		float h10 = texture(sampler2DArray(heightmap, heightmapSampler), heightmapTexcoord + vec3(0, -noffset, 0)).x * 8192.0f - 4096.0f;
		float h12 = texture(sampler2DArray(heightmap, heightmapSampler), heightmapTexcoord + vec3(0, noffset, 0)).x * 8192.0f - 4096.0f;
		
		float nsize = 16;
		vec3 va = normalize(vec3(nsize, h21 - h01, 0));
		vec3 vb = normalize(vec3(0, h12 - h10, -nsize));
		vec3 inNormal = cross(va, vb);
	
		float h = clamp(dot(normalize(inNormal), normalize(vec3(0, 1, 0))), 0.3f, 1.0f);
		albedo_roughness = vec4(vec3(0, 0.75f, 0) * h, 1.0f);
		normal_metalness = vec4(inNormal, 0.0f);
	}

#endif