#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConsts
{
	mat4 invCamMVPMat;
	vec4 cameraPosition;
	vec4 cameraDir;
	vec2 prjMat;
	
} pushConsts;

layout(binding = 8) uniform WorldEnvironmentUBO
{
	vec3 sunDirection;
	float worldTime;
	//
	
} weubo;

#ifdef SHADER_STAGE_VERTEX

	layout(location = 0) out vec2 outUV;
	layout(location = 1) out vec3 at_ray;

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
		at_ray = (pushConsts.invCamMVPMat * vec4(positions[gl_VertexIndex], 0, 1)).xyz;
		
		outUV = positions[gl_VertexIndex] * 0.5f + 0.5f;		
	}

#elif defined(SHADER_STAGE_FRAGMENT)

	#define TRANSMITTANCE_TEXTURE_NAME transmittance_texture
	#define SCATTERING_TEXTURE_NAME scattering_texture
	#define SINGLE_MIE_SCATTERING_TEXTURE_NAME single_mie_scattering_texture
	#define IRRADIANCE_TEXTURE_NAME irradiance_texture

	layout(location = 0) in vec2 inUV;
	layout(location = 1) in vec3 inRay;

	layout(set = 0, binding = 0) uniform sampler inputSampler;
	layout(set = 0, binding = 1) uniform texture2D gbuffer_AlbedoRoughnessTexture; // rgb - albedo, a - roughness
	layout(set = 0, binding = 2) uniform texture2D gbuffer_NormalsMetalnessTexture; // rgb - normals, a - metalness
	layout(set = 0, binding = 3) uniform texture2D gbuffer_DepthTexture;
	layout(set = 0, binding = 4) uniform sampler2D TRANSMITTANCE_TEXTURE_NAME;
	layout(set = 0, binding = 5) uniform sampler3D SCATTERING_TEXTURE_NAME;
	layout(set = 0, binding = 6) uniform sampler3D SINGLE_MIE_SCATTERING_TEXTURE_NAME;
	layout(set = 0, binding = 7) uniform sampler2D IRRADIANCE_TEXTURE_NAME;

	

	#SE_BUILTIN_INCLUDE_ATMOSPHERE_LIB

	layout(location = 0) out vec4 out_color;

	//	vec3 GetSkyRadiance(vec3 camera, vec3 view_ray, float shadow_length, vec3 sun_direction, out vec3 transmittance);
	//  vec3 GetSunAndSkyIrradiance( vec3 p, vec3 normal, vec3 sun_direction, out vec3 sky_irradiance);
	//  vec3 GetSkyRadianceToPoint(vec3 camera, vec3 point, float shadow_length, vec3 sun_direction, out vec3 transmittance);
	//	vec3 BRDF_Diffuse (in vec3 normal, in vec3 view, in vec3 lightDir, in vec3 Kalbedo, in vec3 Kspecular, in float Kr, in float lightOcclusion)

	
	vec3 calcSkyLighting (in vec3 albedo, in vec3 normal, in vec3 viewDir, in vec2 KrKm, in vec3 lightDir, in float depth);
	vec3 BRDF_CookTorrance (in vec3 normal, in vec3 view, in vec3 lightDir, in vec3 Kspecular, in float Kr);
	
	const vec3 earthCenter = vec3(0, 0, -ATMOSPHERE.bottom_radius);
	
	#define PI 3.1415926f
	
	const float sunSize = 0.9999f;
	
	void main()
	{	
		vec4  gbuffer_AlbedoRoughness = texture(sampler2D(gbuffer_AlbedoRoughnessTexture, inputSampler), inUV);
		vec4  gbuffer_NormalsMetalness = texture(sampler2D(gbuffer_NormalsMetalnessTexture, inputSampler), inUV);
		float gbuffer_Depth = texture(sampler2D(gbuffer_DepthTexture, inputSampler), inUV).x;
		
		// Normals are packed [0,1] in the gbuffer, need to put them back to [-1,1]
		gbuffer_NormalsMetalness.xyz = normalize(gbuffer_NormalsMetalness.xyz * 2.0f - 1.0f);
		
		const float nearClip = 100000.0f;
		const float farClip = 0.1f;
		
		float projA = farClip / (farClip - nearClip);
		float projB = (-farClip * nearClip) / (farClip - nearClip);
		float z_eye = projB / (gbuffer_Depth - projA);
		float z_eye_old = pushConsts.prjMat.y / (pushConsts.prjMat.x + gbuffer_Depth);
	
		vec3 testLightDir = weubo.sunDirection;
	
		vec3 at_cameraPosition = vec3(0, 0, max(pushConsts.cameraPosition.y, 1)) / 1000.0f - earthCenter;
		vec3 at_ray = normalize(inRay.xzy);		
		vec3 at_worldPoint = at_cameraPosition + at_ray * z_eye * 1e-3;
				
		vec3 transmittance;
		vec3 radiance = vec3(0);//GetSkyRadiance(at_cameraPosition, at_ray, 0, testLightDir.xzy, transmittance) * 4.0f;
	
		if (gbuffer_Depth > 0)
		{
			vec3 mod_at_worldPoint = vec3(at_worldPoint.xy, max(at_worldPoint.z, 0.001f));
			radiance = GetSkyLuminanceToPoint(at_cameraPosition, mod_at_worldPoint, 0, testLightDir.xzy, transmittance) * 1e-3;
		}
		else
			radiance = GetSkyLuminance(at_cameraPosition, at_ray, 0, testLightDir.xzy, transmittance) * 1e-3;
	
		if (dot(normalize(at_ray), testLightDir.xzy) > sunSize && gbuffer_Depth == 0.0f)
			radiance += transmittance * GetSolarLuminance() * 1e-3 * 1e-3;
			
		vec3 Rd = vec3(0);

		if (gbuffer_Depth != 0.0f)
			Rd = calcSkyLighting(gbuffer_AlbedoRoughness.rgb, gbuffer_NormalsMetalness.rgb, normalize(inRay), vec2(gbuffer_AlbedoRoughness.a, gbuffer_NormalsMetalness.a), testLightDir, z_eye);
		
		vec3 finalColor = Rd * transmittance + radiance;

		out_color = vec4(finalColor, 1);
		//out_color.rgb = vec3(abs(z_eye - z_eye_old));
	}
	
	//100000.0f, 0.1f
	vec3 calcSkyLighting (in vec3 albedo, in vec3 normal, in vec3 viewDir, in vec2 KrKm, in vec3 lightDir, in float depth)
	{
		vec3 at_cameraPosition = vec3(0, 0, max(pushConsts.cameraPosition.y, 1)) / 1000.0f - earthCenter;
		vec3 at_ray = normalize(inRay.xzy);		
		vec3 at_worldPoint = at_cameraPosition + at_ray * depth / 1000.0f;
		
		vec3 sky_irradiance;
		vec3 irradiance = GetSunAndSkyIlluminance(at_worldPoint, normal.xzy, lightDir.xzy, sky_irradiance);
		vec3 transmittance;
		
		float Kr = KrKm.x;
		float Km = KrKm.y;
		vec3  Kalbedo   = albedo * irradiance * 1e-3 / PI;
		vec3  Kambient  = albedo * sky_irradiance * 1e-3 / PI;
		vec3  Kspecular = mix(vec3(1.0f - Kr) * 0.18f + 0.04f, albedo, smoothstep(0.2f, 0.45f, Km));

		vec3 Ralbedo   = Kalbedo * max(0.0f, dot(normal, lightDir)) * (vec3(1) - Kspecular);
		vec3 Rambient  = Kambient * 0.25f + albedo * 2.5f;
		vec3 Rspecular = BRDF_CookTorrance(normal, viewDir, lightDir, Kspecular, Kr);
		
		vec3 dielectric = Ralbedo + Rambient + Rspecular;
		vec3 metal = Rspecular;

		return mix(dielectric, metal, smoothstep(0.2f, 0.45f, Km));
	}
	
	vec3 f_shlick (in vec3 F0, in float NdotV)
	{
		return F0 + (vec3(1.0f) - F0) * pow(1.0f - NdotV, 5);
	}
	
	float g_smith_shlick_beckmann (in float NdotV, in float NdotL, in float Kr)
	{
		float k = Kr * Kr * sqrt(PI);
		float i_v = NdotV / (NdotV * (1 - k) + k);
		float i_l = NdotL / (NdotL * (1 - k) + k);
		
		return i_v * i_l;
	}
	
	float d_ggx (in float NdotH, in float Kr)
	{
		float Kr_2 = Kr * Kr;
		return Kr_2 / (PI * pow(1 + NdotH * NdotH * (Kr - 1.0f), 2));
	}
	
	vec3 BRDF_CookTorrance (in vec3 normal, in vec3 view, in vec3 lightDir, in vec3 Kspecular, in float Kr)
	{
		vec3 L = reflect(-view, normal);
		vec3 H = normalize(lightDir + view);
	
		float NdotL = dot(normal, lightDir);
		float NdotV = dot(normal, view);
		float NdotH = dot(normal, H);
		
		vec3  F = f_shlick(Kspecular, NdotV);
		float G = g_smith_shlick_beckmann(NdotV, NdotL, Kr);
		float D = d_ggx(NdotH, Kr);
		
		return (F * D * G) / (4.0f * NdotL * NdotV);
	}
	

#endif