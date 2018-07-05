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
		vec4 cameraCellOffset;
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
		const float smult = 64.0f;
		const float spow = 8;
		const float lvlInc = 4.0f;
	
		float lvl = floor(smult * pow((dmax - clamp(distToCamera, dmin, dmax) - dmin) / (dmax - dmin), spow));

		return max(floor(lvl / lvlInc) * lvlInc, 2);
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
		vec4 cameraCellOffset;
		int instanceCountWidth;
	} pushConsts;
	
	layout(set = 0, binding = 0) uniform sampler heightmapSampler;
	layout(set = 0, binding = 1) uniform texture2DArray heightmap;

	layout(quads, equal_spacing, ccw) in;
	
	layout(location = 0) in vec3 inVertex[];

	layout(location = 0) out vec3 outVertex;
	layout(location = 1) out vec2 outTerrainTexcoord;
	layout(location = 2) out vec3 outHeightmapTexcoord;
	layout(location = 3) out float outClipmapArrayLayer;

	float getClipmapArrayLayer (float dist);

	float cameraCellCoordOffsetArray[5] = {0, -1, -3, -7, -15};

	void main()
	{
		vec3 meshVertex = mix(mix(inVertex[0], inVertex[3], gl_TessCoord.x), mix(inVertex[1], inVertex[2], gl_TessCoord.x), gl_TessCoord.y);
		vec3 vertex = vec3(meshVertex.x, 0, meshVertex.y);
					
		vec2 cellCoordOffset = pushConsts.cellCoordStart + vec2(int(meshVertex.z) % pushConsts.instanceCountWidth, int(meshVertex.z) / pushConsts.instanceCountWidth);
		float distToCamera = distance(cellCoordOffset * 256.0f, pushConsts.cameraCellCoord * 256.0f + 128.0f);
		float clipmapArrayLayer = getClipmapArrayLayer(distToCamera);
		float cameraCellCoordOffset = cameraCellCoordOffsetArray[int(clipmapArrayLayer)];

		outHeightmapTexcoord = vec3((vertex.xz + cellCoordOffset * 256.0f - (pushConsts.cameraCellCoord + vec2(cameraCellCoordOffset)) * 256.0f) / (512.0f * pow(2, clipmapArrayLayer)), clipmapArrayLayer);
		outTerrainTexcoord = (vertex.xz + cellCoordOffset * 256.0f) * 0.5f;
		
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
		
		outClipmapArrayLayer = clipmapArrayLayer;
		
		cameraCellCoordOffset = cameraCellCoordOffsetArray[int(clipmapArrayLayer)];
		
		vec3 heightmapTexcoord = vec3((vertex.xz + cellCoordOffset * 256.0f - (pushConsts.cameraCellCoord + vec2(cameraCellCoordOffset)) * 256.0f) / (512.0f * pow(2, clipmapArrayLayer)), clipmapArrayLayer);
		
		float heightmapValue = texture(sampler2DArray(heightmap, heightmapSampler), heightmapTexcoord).x;

		vertex.xz += cellCoordOffset * 256.0f;
		vertex.xyz -= pushConsts.cameraCellOffset.xyz;
		vertex.y += (heightmapValue * 8192.0f - 4096.0f);
		
		outVertex = vertex;
		
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
	
	layout(set = 0, binding = 2) uniform sampler textureSampler;
		
	/*
	 * TexArray order: Albedo, normals, roughness, metalness, unused
	 * Accessed like the 2D array: terrainTextures[32][8]
	 */
	layout(set = 0, binding = 3) uniform texture2D terrainTextures[32 * 8]; 

	layout(location = 0) out vec4 albedo_roughness; // rgb - albedo, a - roughness
	layout(location = 1) out vec4 normal_metalness; // rgb - normals, a - metalness

	layout(location = 0) in vec3 inVertex;
	layout(location = 1) in vec2 inTerrainTexcoord;
	layout(location = 2) in vec3 inHeightmapTexcoord;
	layout(location = 3) in float inClipmapArrayLayer;

	vec3 calcNormal(vec3 inNormal, vec3 texNormals)
	{
		vec3 N = normalize(inNormal);
		vec3 T = normalize(cross(N, vec3(1, 0, 0)));
		vec3 B = cross(N, T);
		mat3 tbn = mat3(T, B, N);
		
		return tbn * normalize(texNormals * 2.0f - 1.0f);
	}

	const float uvScale = 1;
	const float tightenFactor = 0.5f;
	
	vec3 triplanarSample (in int texIndex, in vec3 vertex, in vec3 normal, in int arrayLayer, in float blendSharpness)
	{		
		vec2 uvX = vertex.zy / uvScale;
		vec2 uvY = vertex.xz / uvScale;
		vec2 uvZ = vertex.xy / uvScale;
		
		vec3 blend = pow(abs(normal), vec3(blendSharpness)) - vec3(tightenFactor);
		if (max(blend.x, 0) + max(blend.y, 0) + max(blend.z, 0) < 1e-3)
			blend += vec3(tightenFactor);
		
		blend = max(blend, vec3(0));
		
		blend = blend / (blend.x + blend.y + blend.z);
		
		vec3 ttexX = vec3(0);
		vec3 ttexY = vec3(0);
		vec3 ttexZ = vec3(0);
				
		if (blend.x > 0)
			ttexX = texture(sampler2D(terrainTextures[texIndex * 8 + arrayLayer], textureSampler), uvX).rgb;
		if (blend.y > 0)
			ttexY = texture(sampler2D(terrainTextures[texIndex * 8 + arrayLayer], textureSampler), uvY).rgb;
		if (blend.z > 0)
			ttexZ = texture(sampler2D(terrainTextures[texIndex * 8 + arrayLayer], textureSampler), uvZ).rgb;
		
		return ttexX * blend.x + ttexY * blend.y + ttexZ * blend.z;
	}

	vec2 encodeNormal (in vec3 normal)
	{
		vec3 n = normal / (abs(normal.x) + abs(normal.y) + abs(normal.z));
		n.xy = n.z >= 0.0 ? n.xy : ((vec2(1) - abs(n.yx)) * vec2(n.x >= 0 ? 1 : -1, n.y >= 0 ? 1 : -1));
		
		return n.xy * 0.5f + 0.5f;
	}
	
	void main()
	{
		//vec3 inNormal = vec3(0, 1, 0);
	
		vec3 heightmapTexcoord = vec3(inHeightmapTexcoord.x, inHeightmapTexcoord.y, inHeightmapTexcoord.z);
		
		float noffset = 1 / (1025.0f * pow(2, heightmapTexcoord.z));
		float h01 = texture(sampler2DArray(heightmap, heightmapSampler), heightmapTexcoord + vec3(-noffset, 0, 0)).x * 8192.0f - 4096.0f;
		float h21 = texture(sampler2DArray(heightmap, heightmapSampler), heightmapTexcoord + vec3(noffset, 0, 0)).x * 8192.0f - 4096.0f;
		float h10 = texture(sampler2DArray(heightmap, heightmapSampler), heightmapTexcoord + vec3(0, -noffset, 0)).x * 8192.0f - 4096.0f;
		float h12 = texture(sampler2DArray(heightmap, heightmapSampler), heightmapTexcoord + vec3(0, noffset, 0)).x * 8192.0f - 4096.0f;
		
		const float nsize = 16;
		vec3 va = normalize(vec3(nsize, h21 - h01, 0));
		vec3 vb = normalize(vec3(0, h12 - h10, -nsize));
		vec3 inNormal = cross(va, vb);
		
		const int backgroundIndex = 0;
		const int overlayIndex = 1;
		
		float lerpFactor = smoothstep(0.8f, 1.0f, clamp(inNormal.y, 0.8f, 1.0f));
		
		vec3 background_albedo = vec3(0);
		vec3 background_normals = vec3(0);
		float background_roughness = 0;
		float background_metalness = 0;
		float background_ao = 0;
		
		vec3 overlay_albedo = vec3(0);
		vec3 overlay_normals = vec3(0);
		float overlay_roughness = 0;
		float overlay_metalness = 0;
		float overlay_ao = 0;
		
		if (lerpFactor < 1)
		{
			background_albedo = triplanarSample(backgroundIndex, inVertex, inNormal, 0, 1);
			background_normals = triplanarSample(backgroundIndex, inVertex, inNormal, 1, 1);
			background_roughness = triplanarSample(backgroundIndex, inVertex, inNormal, 2, 1).r;
			background_metalness = triplanarSample(backgroundIndex, inVertex, inNormal, 3, 1).r;
			background_ao = triplanarSample(backgroundIndex, inVertex, inNormal, 4, 1).r;
		}
		
		if (lerpFactor > 0)
		{
			overlay_albedo = triplanarSample(overlayIndex, inVertex, inNormal, 0, 1);
			overlay_normals = triplanarSample(overlayIndex, inVertex, inNormal, 1, 1);
			overlay_roughness = triplanarSample(overlayIndex, inVertex, inNormal, 2, 1).r;
			overlay_metalness = triplanarSample(overlayIndex, inVertex, inNormal, 3, 1).r;
			overlay_ao = triplanarSample(overlayIndex, inVertex, inNormal, 4, 1).r;
		}
		
		vec3 malbedo = mix(background_albedo, overlay_albedo, lerpFactor);
		vec3 mnormals = mix(background_normals, overlay_normals, lerpFactor);
		float mroughness = mix(background_roughness, overlay_roughness, lerpFactor);
		float mmetalness = mix(background_metalness, overlay_metalness, lerpFactor);
		float mao = mix(background_ao, overlay_ao, lerpFactor);
		
		vec3 fragNormal = calcNormal(inNormal, mnormals);
			
		albedo_roughness = vec4(malbedo, mroughness);
		normal_metalness = vec4(encodeNormal(fragNormal), mao, mmetalness);
	}

#endif