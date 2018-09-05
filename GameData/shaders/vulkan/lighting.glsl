#version 450
#extension GL_ARB_separate_shader_objects : enable

#define SUN_CSM_COUNT 3

layout(push_constant) uniform PushConsts
{
	mat4 invCamMVPMat;
	vec4 cameraPosition;
	vec4 prjMat_aspectRatio_tanHalfFOV;
	
} pushConsts;

layout(binding = 7) uniform WorldEnvironmentUBO
{
	vec3 sunDirection;
	float worldTime;
	//
	mat4 sunMVPs[SUN_CSM_COUNT];
	mat4 terrainSunMVPs[5];
} weubo;

#ifdef SHADER_STAGE_VERTEX

	layout(location = 0) out vec2 outUV;
	layout(location = 1) out vec3 at_ray;
	layout(location = 2) out vec2 outViewRay;

	out gl_PerVertex
	{
		vec4 gl_Position;
	};
	
	const vec2 positions[6] = vec2[](
		vec2(-1, -1),
		vec2(1, -1),
		vec2(-1, 1),

		vec2(1, -1),
		vec2(1, 1),
		vec2(-1, 1)
	);
		
	void main()
	{	
		gl_Position = vec4(positions[gl_VertexIndex], 0, 1);
		
		vec4 ray = pushConsts.invCamMVPMat * vec4(positions[gl_VertexIndex], 0, 1);
		at_ray = ray.xyz;
		outViewRay = vec2(positions[gl_VertexIndex].x * pushConsts.prjMat_aspectRatio_tanHalfFOV.z * pushConsts.prjMat_aspectRatio_tanHalfFOV.w, -positions[gl_VertexIndex].y * pushConsts.prjMat_aspectRatio_tanHalfFOV.w);
		
		outUV = positions[gl_VertexIndex] * 0.5f + 0.5f;		
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	#define TRANSMITTANCE_TEXTURE_NAME transmittance_texture
	#define SCATTERING_TEXTURE_NAME scattering_texture
	#define SINGLE_MIE_SCATTERING_TEXTURE_NAME single_mie_scattering_texture
	#define IRRADIANCE_TEXTURE_NAME irradiance_texture

	layout(location = 0) in vec2 inUV;
	layout(location = 1) in vec3 inRay;
	layout(location = 2) in vec2 inViewRay;

	layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput gbuffer_AlbedoRoughnessTexture; // rgb - albedo, a - roughness
	layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput gbuffer_NormalsMetalnessTexture; // rgb - normals, a - metalness
	layout(input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput gbuffer_DepthTexture;
	layout(set = 0, binding = 3) uniform sampler2D TRANSMITTANCE_TEXTURE_NAME;
	layout(set = 0, binding = 4) uniform sampler3D SCATTERING_TEXTURE_NAME;
	layout(set = 0, binding = 5) uniform sampler3D SINGLE_MIE_SCATTERING_TEXTURE_NAME;
	layout(set = 0, binding = 6) uniform sampler2D IRRADIANCE_TEXTURE_NAME;
	layout(set = 0, binding = 8) uniform texture2DArray sunShadowmap;
	layout(set = 0, binding = 9) uniform texture2DArray terrainShadowmap;
	layout(set = 0, binding = 10) uniform sampler sunShadowmapSampler;
	layout(set = 0, binding = 11) uniform sampler ditherSampler;
	layout(set = 0, binding = 12) uniform sampler linearSampler;
	layout(set = 0, binding = 13) uniform texture2D ditherTex;
	layout(set = 0, binding = 14) uniform textureCube skyCubemap;
	layout(set = 0, binding = 15) uniform textureCube skyEnviroCubemap;
	layout(set = 0, binding = 16) uniform texture2D brdfLUT;

	#SE_BUILTIN_INCLUDE_ATMOSPHERE_LIB

	layout(location = 0) out vec4 out_color;

	//	vec3 GetSkyRadiance(vec3 camera, vec3 view_ray, float shadow_length, vec3 sun_direction, out vec3 transmittance);
	//  vec3 GetSunAndSkyIrradiance( vec3 p, vec3 normal, vec3 sun_direction, out vec3 sky_irradiance);
	//  vec3 GetSkyRadianceToPoint(vec3 camera, vec3 point, float shadow_length, vec3 sun_direction, out vec3 transmittance);
	//	vec3 BRDF_Diffuse (in vec3 normal, in vec3 view, in vec3 lightDir, in vec3 Kalbedo, in vec3 Kspecular, in float Kr, in float lightOcclusion)

	
	vec3 calcSkyLighting (in vec3 albedo, in vec3 normal, in vec3 viewDir, in vec2 KrKm, in vec3 lightDir, in float depth, in float sunOcclusion, in float ambientOcclusion);
	vec3 BRDF_CookTorrance (in vec3 normal, in vec3 view, in vec3 lightDir, in vec3 Kspecular, in float Kr, in vec3 Klightcolor);
	
	const vec3 earthCenter = vec3(0, 0, -ATMOSPHERE.bottom_radius);
	
	#define PI 3.1415926535f
	#define SQRT_PI 1.7724538509f
	#define INV_PI 0.3183098861f
	
	const float sunSize = 0.9999890722f;
	
	vec3 decodeNormal (in vec2 enc)
	{
		vec2 f = enc * 2.0f - 1.0f;
		vec3 n = vec3(f.x, f.y, 1.0f - abs(f.x) - abs(f.y));
		float t = clamp(-n.z, 0.0f, 1.0f);
		n.xy += vec2(n.x >= 0 ? -t : t, n.y >= 0 ? -t : t);
		
		return normalize(n);
	}
	
	float getSunOcclusion (in vec3 viewPos);
	vec3 dither(in vec3 c);
	
	void main()
	{	
		vec4  gbuffer_AlbedoRoughness = subpassLoad(gbuffer_AlbedoRoughnessTexture);//texture(sampler2D(gbuffer_AlbedoRoughnessTexture, inputSampler), inUV);
		vec4  gbuffer_NormalsMetalness = subpassLoad(gbuffer_NormalsMetalnessTexture);//texture(sampler2D(gbuffer_NormalsMetalnessTexture, inputSampler), inUV);
		float gbuffer_Depth = subpassLoad(gbuffer_DepthTexture).r;//texture(sampler2D(gbuffer_DepthTexture, inputSampler), inUV).x;
		float gbuffer_AO = gbuffer_NormalsMetalness.z;
		
		// Normals are packed [0,1] in the gbuffer, need to put them back to [-1,1]
		gbuffer_NormalsMetalness.xyz = decodeNormal(gbuffer_NormalsMetalness.xy);//normalize(gbuffer_NormalsMetalness.xyz * 2.0f - 1.0f);
		
		gbuffer_AlbedoRoughness.a = max(gbuffer_AlbedoRoughness.a, 0.01f);
		gbuffer_NormalsMetalness.a = clamp(gbuffer_NormalsMetalness.a, 0.02f, 0.99f);
		
		const float nearClip = 100000.0f;
		const float farClip = 0.1f;
		
		const float projA = farClip / (farClip - nearClip);
		const float projB = (-farClip * nearClip) / (farClip - nearClip);
		float z_eye = projB / (gbuffer_Depth - projA);
	
		vec3 testLightDir = normalize(weubo.sunDirection);
	
		vec3 at_cameraPosition = vec3(0, 0, max(pushConsts.cameraPosition.y, 100.0f)) / 1000.0f - earthCenter;
		vec3 at_ray = normalize(inRay.xzy);		
		vec3 at_worldPoint = at_cameraPosition + at_ray * z_eye * 1e-3;
			
		vec3 transmittance = vec3(1);
		vec3 radiance = vec3(0);//GetSkyRadiance(at_cameraPosition, at_ray, 0, testLightDir.xzy, transmittance) * 4.0f;
	
		if (gbuffer_Depth == 0)
		{
			//radiance = dither(GetSkyLuminance(at_cameraPosition, at_ray, 0, testLightDir.xzy, transmittance) * 1e-3);
			radiance = dither(texture(samplerCube(skyCubemap, linearSampler), at_ray.xzy).rgb);
		}
		
		if (dot(at_ray, testLightDir.xzy) > 0.99995628906 && gbuffer_Depth == 0.0f)
		{
			float r = length(at_cameraPosition);
			float rmu = dot(at_cameraPosition, at_ray);
			float mu = rmu / r;
			bool ray_r_mu_intersects_ground = mu < 0.0 && r * r * (mu * mu - 1.0) + ATMOSPHERE.bottom_radius * ATMOSPHERE.bottom_radius >= 0.0;

			transmittance = ray_r_mu_intersects_ground ? vec3(0.0) : GetTransmittanceToTopAtmosphereBoundary(ATMOSPHERE, transmittance_texture, r, mu);
			
			radiance += GetSolarLuminance() * 1e-3 * 1e-3 * transmittance;
		}
		
		vec3 Rd = vec3(0);
		float sunOcclusion = getSunOcclusion(vec3(inViewRay, -1) * z_eye);
		
		if (gbuffer_Depth > 0)
			Rd = calcSkyLighting(gbuffer_AlbedoRoughness.rgb, gbuffer_NormalsMetalness.xyz, normalize(inRay), vec2(gbuffer_AlbedoRoughness.a, gbuffer_NormalsMetalness.a), testLightDir, z_eye, sunOcclusion, gbuffer_AO);
		
		vec3 finalColor = Rd * transmittance + radiance;
		
		//if (inUV.x < 0.25 && inUV.y < 0.25)
		//	finalColor = abs(texture(sampler2D(testShadowMap, inputSampler), inUV * 4.0f).r - 1) > 0 ? vec3(10) : vec3(0);
		
		out_color = vec4(finalColor, 1);
		//out_color.rgb = gbuffer_NormalsMetalness.xyz;
		//out_color.rgb = normalize(inRay);
		//out_color.rgb = vec3(normalize(inViewRay), -1) * z_eye;
		//out_color.rgb = normalize(reflect(-normalize(inRay), gbuffer_NormalsMetalness.xyz));
		//out_color.rgb = vec3(abs(z_eye - z_eye_old));
		//out_color.rgb = abs(gbuffer_NormalsMetalness.rgb) * 4.0f;
		
	}
	
	vec3 f_shlick (in vec3 F0, in float NdotV);
	vec3 f_shlick_roughness (in vec3 F0, in float roughness, in float NdotV);
	float g_smith_shlick_beckmann (in float NdotV, in float NdotL, in float Kr);
	float d_ggx (in float NdotH, in float Kr);
	
	//100000.0f, 0.1f
	vec3 calcSkyLighting (in vec3 albedo, in vec3 normal, in vec3 viewDir, in vec2 KrKm, in vec3 lightDir, in float depth, in float sunOcclusion, in float ambientOcclusion)
	{
		vec3 at_cameraPosition = vec3(0, 0, max(pushConsts.cameraPosition.y, 1)) / 1000.0f - earthCenter;
		vec3 at_ray = normalize(inRay.xzy);		
		vec3 at_worldPoint = at_cameraPosition + at_ray * depth / 1000.0f;
		
		vec3 sky_irradiance;
		vec3 irradiance = GetSunAndSkyIlluminance(at_worldPoint, normal.xzy, lightDir.xzy, sky_irradiance);
		vec3 transmittance;
		
		float Kr = KrKm.x;
		float Km = KrKm.y;
		float aofactor = smoothstep(0.5f, 1.0f, ambientOcclusion * 0.5f + 0.5f) * 0.5f + 0.5f;
		vec3  Kalbedo   = albedo * irradiance * 1e-3 * INV_PI;
		vec3  Kambient  = albedo * sky_irradiance * 1e-3 * INV_PI;
		vec3  Kspecular = mix(vec3(0.04f), albedo, Km);

		vec3 Ralbedo   = Kalbedo * clamp(dot(normal, lightDir), 0.0f, 1.0f) * (vec3(1) - Kspecular) * (1 - sunOcclusion);
		vec3 Rambient  = Kambient * 0.5f * aofactor + albedo * 1.5f * aofactor;

		vec3 L = normalize(reflect(viewDir, normal));
	
		vec3 prefilteredEnviroColor = max(textureLod(samplerCube(skyEnviroCubemap, linearSampler), L, Kr * 5).rgb, vec3(0));
		vec3 F = f_shlick_roughness(Kspecular, Kr, max(dot(normal, -viewDir), 0));
		vec2 envBRDF = texture(sampler2D(brdfLUT, linearSampler), vec2(max(dot(normal, -viewDir), 0.0f), Kr)).xy;
		vec3 Rspecular = prefilteredEnviroColor * (F * envBRDF.x + vec3(envBRDF.y * Km)) * ambientOcclusion;
		
		vec3 dielectric = Ralbedo + Rambient + Rspecular;
		vec3 metal = Rspecular;

		return mix(dielectric, metal, Km);
	}
	
	vec3 f_shlick (in vec3 F0, in float NdotV)
	{
		return F0 + (vec3(1.0f) - F0) * pow(1.0f - NdotV, 5.0f);
	}
	
	vec3 f_shlick_roughness (in vec3 F0, in float roughness, in float NdotV)
	{
		return F0 + (max(vec3(1.0f - roughness), F0) - F0) * pow(1.0f - NdotV, 5.0f);
	}
	
	float g_smith_shlick_beckmann (in float NdotV, in float NdotL, in float Kr)
	{
		float k = Kr * Kr * SQRT_PI;
		float g0 = NdotV / (NdotV * (1.0f - k) + k);
		float g1 = NdotL / (NdotL * (1.0f - k) + k);
		
		return g0 * g1;
	}
	
	float g_smith_ggx (in float NdotV, in float NdotL, in float Kr)
	{
		float Kr_2 = Kr * Kr * Kr * Kr;
		float NdotL_2 = NdotL * NdotL;
		float NdotV_2 = NdotV * NdotV;
		float G_0 = (2.0f * NdotL) / (NdotL + sqrt(Kr_2 + (1.0f - Kr_2) * NdotL_2));
		float G_1 = (2.0f * NdotV) / (NdotV + sqrt(Kr_2 + (1.0f - Kr_2) * NdotV_2));

		return G_0 * G_1;
	}
	
	float d_ggx (in float NdotH, in float Kr)
	{
		float a = Kr * Kr;
		float a_2 = a * a;
		return a_2 / (PI * pow(1 + NdotH * NdotH * (a_2 - 1.0f), 2));
	}
	
	#define saturate(x) clamp(x, 0, 1)
	
	vec3 BRDF_CookTorrance (in vec3 normal, in vec3 view, in vec3 lightDir, in vec3 Kspecular, in float Kr, in vec3 Klightcolor)
	{
		vec3 H = normalize(view + lightDir);
	
		float NdotL = abs(dot(normal, lightDir));
		float NdotV = abs(dot(normal, view));
		float NdotH = abs(dot(normal, H));
		float HdotV = abs(dot(H, view));
		
		vec3  F = f_shlick(Kspecular, HdotV);
		float G = g_smith_ggx(NdotV, NdotL, Kr);
		float D = d_ggx(NdotH, Kr);
	
		return Klightcolor * ((F * D * G) / (4.0f * NdotV * NdotL));
		//return F * D * G * 0.25f * (1 / (NdotL * NdotV));
		//return (D * G * 0.25f * (1.0f / (NdotL * max(NdotV, 0)))) * F;
	}
		
	#define PCF_WIDTH 3 // kernel size would be 2(PCF_WIDTH) + 1, so 2(4) + 1 = 9
	
	float sampleShadowmapOcclusion (in vec3 uv, in float shadowmapSize, in vec3 shadowCoord, in float bias)
	{
		// Gather the surrounding texels for depth values
		vec4 texels = textureGather(sampler2DArray(sunShadowmap, sunShadowmapSampler), uv, 0);
		
		// Do depth-comparisons
		texels = step(texels, vec4(shadowCoord.z - bias));
		
		// Bilinearly filter the comparisons
		vec2 sampleCoord = fract(shadowCoord.xy * shadowmapSize + vec2(0.5f));
		
		float x0 = mix(texels.x, texels.y, sampleCoord.x);
		float x1 = mix(texels.w, texels.z, sampleCoord.x);
		
		return mix(x1, x0, sampleCoord.y);
	}
	
	float sampleTerrainShadowmapOcclusion (in vec3 uv, in float shadowmapSize, in vec3 shadowCoord, in float bias)
	{
		// Gather the surrounding texels for depth values
		vec4 texels = textureGather(sampler2DArray(terrainShadowmap, sunShadowmapSampler), uv, 0);
		
		// Do depth-comparisons
		texels = step(texels, vec4(shadowCoord.z - bias));
		
		// Bilinearly filter the comparisons
		vec2 sampleCoord = fract(shadowCoord.xy * shadowmapSize + vec2(0.5f));
		
		float x0 = mix(texels.x, texels.y, sampleCoord.x);
		float x1 = mix(texels.w, texels.z, sampleCoord.x);
		
		return mix(x1, x0, sampleCoord.y);
	}
	
	const ivec2 earlyBailGatherOffsets[4] = {ivec2(0, PCF_WIDTH + 2), ivec2(0, -PCF_WIDTH - 1), ivec2(PCF_WIDTH + 2, 0), ivec2(-PCF_WIDTH - 1, 0)};
	
	float getSunOcclusion (in vec3 viewPos)
	{	
		float occl = 0;
	
		// Test normal shadowmaps first
		for (int c = 0; c < SUN_CSM_COUNT; c ++)
		{
			vec4 shadowCoord = weubo.sunMVPs[c] * vec4(viewPos, 1);
			
			if (all(lessThan(shadowCoord.xyz, vec3(1))) && all(greaterThan(shadowCoord.xy, vec2(0))))
			{
				float avgOcclusion = 0;
				float scaledPCFWidth = round(PCF_WIDTH / float(c + 1));
				float shadowmapSize = float(textureSize(sampler2DArray(sunShadowmap, sunShadowmapSampler), 0).x);	
				float shadowmapSizeInv = 1.0f / shadowmapSize;
			
				// Early bailing technique
				vec4 earlyBailSamples = textureGatherOffsets(sampler2DArray(sunShadowmap, sunShadowmapSampler), vec3(shadowCoord.xy, c), earlyBailGatherOffsets);
				earlyBailSamples = step(earlyBailSamples, vec4(shadowCoord.z - 0.0005f));
				float ssum = dot(earlyBailSamples, vec4(1)) * 0.25f;
			
				if (ssum < 0.0001f)
					break;
				else if (ssum > 0.9999f)
					return 1;
			
				for (float x = -scaledPCFWidth; x <= scaledPCFWidth; x ++)
				{
					for (float y = -scaledPCFWidth; y <= scaledPCFWidth; y ++)
					{
						avgOcclusion += sampleShadowmapOcclusion(vec3(shadowCoord.xy + vec2(x, y) * shadowmapSizeInv, c), shadowmapSize, shadowCoord.xyz, 0.0005f);
					}
				}

				occl = avgOcclusion / ((2.0f * scaledPCFWidth + 1.0f) * (2.0f * scaledPCFWidth + 1.0f));
				break;
			}
		}
		
		const float layerBias[] = {0.005f, 0.01f, 0.02f, 0.04f, 0.08f};
		
		// Test the terrain shadowmaps
		for (int l = 5; l < 5; l ++)
		{
			vec4 shadowCoord = weubo.terrainSunMVPs[l] * vec4(viewPos, 1);
			
			if (all(lessThan(shadowCoord.xyz, vec3(1))) && all(greaterThan(shadowCoord.xy, vec2(0))))
			{
				float avgOcclusion = 0;
				const float scaledPCFWidth = PCF_WIDTH;//round(PCF_WIDTH / float(l + 1));
				float shadowmapSize = float(textureSize(sampler2DArray(terrainShadowmap, sunShadowmapSampler), 0).x);	
				float shadowmapSizeInv = 1.0f / shadowmapSize;
			
				// Early bailing technique
				vec4 earlyBailSamples = textureGatherOffsets(sampler2DArray(terrainShadowmap, sunShadowmapSampler), vec3(shadowCoord.xy, l), earlyBailGatherOffsets);
				
				// Test if all the samples are pure 1, in which case skip to next layerBias
				if (earlyBailSamples.x + earlyBailSamples.y + earlyBailSamples.z + earlyBailSamples.w > 3.9999f)
					continue;
				
				earlyBailSamples = step(earlyBailSamples, vec4(shadowCoord.z - layerBias[l]));
				float ssum = dot(earlyBailSamples, vec4(1)) * 0.25f;
				
				if (ssum > 0.9999f)
					return 1;
			
				for (float x = -scaledPCFWidth; x <= scaledPCFWidth; x ++)
				{
					for (float y = -scaledPCFWidth; y <= scaledPCFWidth; y ++)
					{
						avgOcclusion += sampleTerrainShadowmapOcclusion(vec3(shadowCoord.xy + vec2(x, y) * shadowmapSizeInv, l), shadowmapSize, shadowCoord.xyz, layerBias[l]);
					}
				}

				avgOcclusion /= ((2.0f * scaledPCFWidth + 1.0f) * (2.0f * scaledPCFWidth + 1.0f));
				
				if (avgOcclusion > 0.9999f)
					return 1;
					
				occl = avgOcclusion;//max(occl, avgOcclusion);
				
				return avgOcclusion;
			}
		}
		
		return occl;
	}
	
	vec3 dither(in vec3 c)
	{
		float bayer0 = textureOffset(sampler2D(ditherTex, ditherSampler), gl_FragCoord.xy / 8.0f, ivec2(0, 0)).r;
		//float bayer1 = textureOffset(sampler2D(ditherTex, ditherSampler), gl_FragCoord.xy / 8.0f, ivec2(-1, -1)).r;
		//float bayer2 = textureOffset(sampler2D(ditherTex, ditherSampler), gl_FragCoord.xy / 8.0f, ivec2(1, 1)).r;
		//float bayer3 = textureOffset(sampler2D(ditherTex, ditherSampler), gl_FragCoord.xy / 8.0f, ivec2(1, -1)).r;
		//float bayer4 = textureOffset(sampler2D(ditherTex, ditherSampler), gl_FragCoord.xy / 8.0f, ivec2(-1, 1)).r;
		
		vec3 rgb = c * 4.0f;
		vec3 head = floor(rgb);
		vec3 tail = fract(rgb);
		//vec3 dithered = (step(bayer0, tail) + step(bayer1, tail) + step(bayer2, tail) + step(bayer3, tail) + step(bayer4, tail)) / 5.0f;
		vec3 dithered = step(bayer0, tail);
		
		return (head + dithered) / 4.0f;
	}
	
#endif