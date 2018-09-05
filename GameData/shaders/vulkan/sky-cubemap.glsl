#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConsts
{
	vec3 lightDir;
	float cosSunAngularRadius;
} pushConsts;

#ifdef SHADER_STAGE_VERTEX

	layout(location = 0) out vec4 outPosition;

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

		outPosition = vec4(inPosition.xy, 0, 1);
	}

#elif defined(SHADER_STAGE_GEOMETRY)

	layout(triangles, invocations = 6) in;
	layout(triangle_strip, max_vertices = 3) out;

	layout(set = 0, binding = 4) uniform FaceInvMVPs
	{
		mat4 invCamMVPMat[6];
	} mvps;
	
	layout(location = 0) in vec4 inPosition[3];
	
	
	layout(location = 0) out vec3 outRay;
	
	void main()
	{
		for (int i = 0; i < 3; i ++)
		{
			outRay = vec3(mvps.invCamMVPMat[gl_InvocationID] * inPosition[i]) * vec3(1, 1, -1);

			gl_Position = inPosition[i];
			gl_Layer = gl_InvocationID;
			
			EmitVertex();
		}
		
		EndPrimitive();
	}
	
#elif defined(SHADER_STAGE_FRAGMENT)

	#define TRANSMITTANCE_TEXTURE_NAME transmittance_texture
	#define SCATTERING_TEXTURE_NAME scattering_texture
	#define SINGLE_MIE_SCATTERING_TEXTURE_NAME single_mie_scattering_texture
	#define IRRADIANCE_TEXTURE_NAME irradiance_texture

	layout(set = 0, binding = 0) uniform sampler2D TRANSMITTANCE_TEXTURE_NAME;
	layout(set = 0, binding = 1) uniform sampler3D SCATTERING_TEXTURE_NAME;
	layout(set = 0, binding = 2) uniform sampler3D SINGLE_MIE_SCATTERING_TEXTURE_NAME;
	layout(set = 0, binding = 3) uniform sampler2D IRRADIANCE_TEXTURE_NAME;

	#SE_BUILTIN_INCLUDE_ATMOSPHERE_LIB
	
	layout(location = 0) in vec3 inRay;

	layout(location = 0) out vec4 fragColor;

	const vec3 earthCenter = vec3(0, 0, -ATMOSPHERE.bottom_radius);
	
	void main()
	{
		vec3 viewRay = normalize(inRay);
	
		vec3 transmittance;
		vec3 lum = GetSkyLuminance(vec3(0, 0, 1e-2) - earthCenter, viewRay.xzy, 0, pushConsts.lightDir.xzy, transmittance) * 1e-3;
		
		if (dot(viewRay, pushConsts.lightDir) > pushConsts.cosSunAngularRadius)
			lum += GetSolarLuminance() * transmittance * 1e-6;
		
		fragColor = vec4(lum, 1);
	}

#endif
