/*
 * MIT License
 * 
 * Copyright (c) 2017 David Allen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * AtmosphereRenderer.cpp
 * 
 * Created on: Mar 27, 2018
 *     Author: david
 */

#include "Rendering/Deferred/AtmosphereRenderer.h"

#include <Engine/StarlightEngine.h>

#include <Rendering/Renderer/Renderer.h>
#include <Rendering/Deferred/Atmosphere/constants.h>
#include <Rendering/Deferred/Atmosphere/definitions.glsl.inc>

AtmosphereRenderer::AtmosphereRenderer (StarlightEngine *enginePtr)
{
	engine = enginePtr;

	destroyed = false;
}

AtmosphereRenderer::~AtmosphereRenderer ()
{
	if (!destroyed)
		destroy();
}

void AtmosphereRenderer::init ()
{
	loadScatteringSourceInclude();
	loadPrecomputedTextures();
}

void AtmosphereRenderer::destroy ()
{
	destroyed = true;

	engine->renderer->destroyTextureView(transmittanceTV);
	engine->renderer->destroyTextureView(scatteringTV);
	engine->renderer->destroyTextureView(irradianceTV);

	engine->renderer->destroyTexture(transmittanceTexture);
	engine->renderer->destroyTexture(scatteringTexture);
	engine->renderer->destroyTexture(irradianceTexture);
}

std::string AtmosphereRenderer::getAtmosphericShaderLib ()
{
	return atmosphereShaderInclude;
}

void AtmosphereRenderer::loadPrecomputedTextures ()
{
	transmittanceTexture = engine->renderer->createTexture({(float) TRANSMITTANCE_TEXTURE_WIDTH, (float) TRANSMITTANCE_TEXTURE_HEIGHT, 1}, RESOURCE_FORMAT_R32G32B32A32_SFLOAT, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_TRANSFER_DST_BIT, MEMORY_USAGE_GPU_ONLY, false);
	scatteringTexture = engine->renderer->createTexture({(float) SCATTERING_TEXTURE_WIDTH, (float) SCATTERING_TEXTURE_HEIGHT, (float) SCATTERING_TEXTURE_DEPTH}, RESOURCE_FORMAT_R32G32B32A32_SFLOAT, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_TRANSFER_DST_BIT, MEMORY_USAGE_GPU_ONLY, false, 1, 1, TEXTURE_TYPE_3D);
	irradianceTexture = engine->renderer->createTexture({(float) IRRADIANCE_TEXTURE_WIDTH, (float) IRRADIANCE_TEXTURE_HEIGHT, 1}, RESOURCE_FORMAT_R32G32B32A32_SFLOAT, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_TRANSFER_DST_BIT, MEMORY_USAGE_GPU_ONLY, false);

	transmittanceTV = engine->renderer->createTextureView(transmittanceTexture);
	scatteringTV = engine->renderer->createTextureView(scatteringTexture, TEXTURE_VIEW_TYPE_3D);
	irradianceTV = engine->renderer->createTextureView(irradianceTexture);

	std::vector<char> transmittanceBTD = FileLoader::instance()->readFileBuffer("GameData/textures/atmosphere/transmittance.btd");
	std::vector<char> scatteringBTD = FileLoader::instance()->readFileBuffer("GameData/textures/atmosphere/scattering.btd");
	std::vector<char> irradianceBTD = FileLoader::instance()->readFileBuffer("GameData/textures/atmosphere/irradiance.btd");

	StagingBuffer transmittanceSB = engine->renderer->createAndMapStagingBuffer(transmittanceBTD.size(), transmittanceBTD.data());
	StagingBuffer scatteringSB = engine->renderer->createAndMapStagingBuffer(scatteringBTD.size(), scatteringBTD.data());
	StagingBuffer irradianceSB = engine->renderer->createAndMapStagingBuffer(irradianceBTD.size(), irradianceBTD.data());

	CommandPool tmpCmdPool = engine->renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, COMMAND_POOL_TRANSIENT_BIT);
	CommandBuffer buf = engine->renderer->beginSingleTimeCommand(tmpCmdPool);

	buf->transitionTextureLayout(transmittanceTexture, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL);
	buf->transitionTextureLayout(scatteringTexture, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL);
	buf->transitionTextureLayout(irradianceTexture, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL);

	buf->stageBuffer(transmittanceSB, transmittanceTexture);
	buf->stageBuffer(scatteringSB, scatteringTexture);
	buf->stageBuffer(irradianceSB, irradianceTexture);

	buf->transitionTextureLayout(transmittanceTexture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	buf->transitionTextureLayout(scatteringTexture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	buf->transitionTextureLayout(irradianceTexture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	engine->renderer->endSingleTimeCommand(buf, tmpCmdPool, QUEUE_TYPE_GRAPHICS);

	engine->renderer->destroyStagingBuffer(transmittanceSB);
	engine->renderer->destroyStagingBuffer(scatteringSB);
	engine->renderer->destroyStagingBuffer(irradianceSB);
}

double Interpolate (const std::vector<double>& wavelengths, const std::vector<double>& wavelength_function, double wavelength)
{
	assert(wavelength_function.size() == wavelengths.size());
	if (wavelength < wavelengths[0])
	{
		return wavelength_function[0];
	}
	for (unsigned int i = 0; i < wavelengths.size() - 1; ++ i)
	{
		if (wavelength < wavelengths[i + 1])
		{
			double u = (wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
			return wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
		}
	}
	return wavelength_function[wavelength_function.size() - 1];
}

double CieColorMatchingFunctionTableValue (double wavelength, int column)
{
	if (wavelength <= kLambdaMin || wavelength >= kLambdaMax)
	{
		return 0.0;
	}
	double u = (wavelength - kLambdaMin) / 5.0;
	int row = static_cast<int>(std::floor(u));
	assert(row >= 0 && row + 1 < 95);
	assert(CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row] <= wavelength && CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1)] >= wavelength);
	u -= row;
	return CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u) + CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
}

// The returned constants are in lumen.nm / watt.
void ComputeSpectralRadianceToLuminanceFactors (const std::vector<double>& wavelengths, const std::vector<double>& solar_irradiance, double lambda_power, double* k_r, double* k_g, double* k_b)
{
	*k_r = 0.0;
	*k_g = 0.0;
	*k_b = 0.0;
	double solar_r = Interpolate(wavelengths, solar_irradiance, kLambdaR);
	double solar_g = Interpolate(wavelengths, solar_irradiance, kLambdaG);
	double solar_b = Interpolate(wavelengths, solar_irradiance, kLambdaB);
	int dlambda = 1;
	for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda)
	{
		double x_bar = CieColorMatchingFunctionTableValue(lambda, 1);
		double y_bar = CieColorMatchingFunctionTableValue(lambda, 2);
		double z_bar = CieColorMatchingFunctionTableValue(lambda, 3);
		const double* xyz2srgb = XYZ_TO_SRGB;
		double r_bar = xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
		double g_bar = xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
		double b_bar = xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
		double irradiance = Interpolate(wavelengths, solar_irradiance, lambda);
		*k_r += r_bar * irradiance / solar_r * pow(lambda / kLambdaR, lambda_power);
		*k_g += g_bar * irradiance / solar_g * pow(lambda / kLambdaG, lambda_power);
		*k_b += b_bar * irradiance / solar_b * pow(lambda / kLambdaB, lambda_power);
	}
	*k_r *= MAX_LUMINOUS_EFFICACY * dlambda;
	*k_g *= MAX_LUMINOUS_EFFICACY * dlambda;
	*k_b *= MAX_LUMINOUS_EFFICACY * dlambda;
}

void AtmosphereRenderer::loadScatteringSourceInclude ()
{
	DensityProfileLayer rayleigh_layer(0.0, 1.0, -1.0 / kRayleighScaleHeight, 0.0, 0.0);
	DensityProfileLayer mie_layer(0.0, 1.0, -1.0 / kMieScaleHeight, 0.0, 0.0);
	// Density profile increasing linearly from 0 to 1 between 10 and 25km, and
	// decreasing linearly from 1 to 0 between 25 and 40km. This is an approximate
	// profile from http://www.kln.ac.lk/science/Chemistry/Teaching_Resources/
	// Documents/Introduction%20to%20atmospheric%20chemistry.pdf (page 10).
	std::vector<DensityProfileLayer> ozone_density;
	ozone_density.push_back(DensityProfileLayer(25000.0, 0.0, 0.0, 1.0 / 15000.0, -2.0 / 3.0));
	ozone_density.push_back(DensityProfileLayer(0.0, 0.0, 0.0, -1.0 / 15000.0, 8.0 / 3.0));

	std::vector<double> wavelengths;
	std::vector<double> solar_irradiance;
	std::vector<double> rayleigh_scattering;
	std::vector<double> mie_scattering;
	std::vector<double> mie_extinction;
	std::vector<double> absorption_extinction;
	std::vector<double> ground_albedo;
	for (int l = kLambdaMin; l <= kLambdaMax; l += 10)
	{
		double lambda = static_cast<double>(l) * 1e-3;  // micro-meters
		double mie = kMieAngstromBeta / kMieScaleHeight * pow(lambda, -kMieAngstromAlpha);
		wavelengths.push_back(l);
		if (use_constant_solar_spectrum)
		{
			solar_irradiance.push_back(kConstantSolarIrradiance);
		}
		else
		{
			solar_irradiance.push_back(kSolarIrradiance[(l - kLambdaMin) / 10]);
		}
		rayleigh_scattering.push_back(kRayleigh * pow(lambda, -4));
		mie_scattering.push_back(mie * kMieSingleScatteringAlbedo);
		mie_extinction.push_back(mie);
		absorption_extinction.push_back(use_ozone ? kMaxOzoneNumberDensity * kOzoneCrossSection[(l - kLambdaMin) / 10] : 0.0);
		ground_albedo.push_back(kGroundAlbedo);
	}

	bool precompute_illuminance = 15 > 3;
	double sky_k_r, sky_k_g, sky_k_b;
	if (precompute_illuminance)
	{
		sky_k_r = sky_k_g = sky_k_b = MAX_LUMINOUS_EFFICACY;
	}
	else
	{
		ComputeSpectralRadianceToLuminanceFactors(wavelengths, solar_irradiance, -3 /* lambda_power */, &sky_k_r, &sky_k_g, &sky_k_b);
	}
	// Compute the values for the SUN_RADIANCE_TO_LUMINANCE constant.
	double sun_k_r, sun_k_g, sun_k_b;
	ComputeSpectralRadianceToLuminanceFactors(wavelengths, solar_irradiance, 0 /* lambda_power */, &sun_k_r, &sun_k_g, &sun_k_b);

	auto density_layer = [](const DensityProfileLayer& layer)
	{
		return "DensityProfileLayer(" +
		std::to_string(layer.width / kLengthUnitInMeters) + "," +
		std::to_string(layer.exp_term) + "," +
		std::to_string(layer.exp_scale * kLengthUnitInMeters) + "," +
		std::to_string(layer.linear_term * kLengthUnitInMeters) + "," +
		std::to_string(layer.constant_term) + ")";
	};
	auto density_profile = [density_layer](std::vector<DensityProfileLayer> layers)
	{
		constexpr int kLayerCount = 2;
		while (layers.size() < kLayerCount)
		{
			layers.insert(layers.begin(), DensityProfileLayer());
		}
		std::string result = "DensityProfile(DensityProfileLayer[" +
		std::to_string(kLayerCount) + "](";
		for (int i = 0; i < kLayerCount; ++i)
		{
			result += density_layer(layers[i]);
			result += i < kLayerCount - 1 ? "," : "))";
		}
		return result;
	};

	auto to_string = [&wavelengths](const std::vector<double>& v,
			const glm::vec3& lambdas, double scale)
	{
		double r = Interpolate(wavelengths, v, lambdas[0]) * scale;
		double g = Interpolate(wavelengths, v, lambdas[1]) * scale;
		double b = Interpolate(wavelengths, v, lambdas[2]) * scale;
		return "vec3(" + std::to_string(r) + "," + std::to_string(g) + "," +
		std::to_string(b) + ")";
	};

	auto glsl_header_factory_ = [=](const glm::vec3& lambdas)
	{
		return
		"#define IN(x) const in x\n"
		"#define OUT(x) out x\n"
		"#define TEMPLATE(x)\n"
		"#define TEMPLATE_ARGUMENT(x)\n"
		"#define assert(x)\n"
		"const int TRANSMITTANCE_TEXTURE_WIDTH = " +
		std::to_string(TRANSMITTANCE_TEXTURE_WIDTH) + ";\n" +
		"const int TRANSMITTANCE_TEXTURE_HEIGHT = " +
		std::to_string(TRANSMITTANCE_TEXTURE_HEIGHT) + ";\n" +
		"const int SCATTERING_TEXTURE_R_SIZE = " +
		std::to_string(SCATTERING_TEXTURE_R_SIZE) + ";\n" +
		"const int SCATTERING_TEXTURE_MU_SIZE = " +
		std::to_string(SCATTERING_TEXTURE_MU_SIZE) + ";\n" +
		"const int SCATTERING_TEXTURE_MU_S_SIZE = " +
		std::to_string(SCATTERING_TEXTURE_MU_S_SIZE) + ";\n" +
		"const int SCATTERING_TEXTURE_NU_SIZE = " +
		std::to_string(SCATTERING_TEXTURE_NU_SIZE) + ";\n" +
		"const int IRRADIANCE_TEXTURE_WIDTH = " +
		std::to_string(IRRADIANCE_TEXTURE_WIDTH) + ";\n" +
		"const int IRRADIANCE_TEXTURE_HEIGHT = " +
		std::to_string(IRRADIANCE_TEXTURE_HEIGHT) + ";\n" +
		(use_combined_textures ?
				"#define COMBINED_SCATTERING_TEXTURES\n" : "") +
		definitions_glsl +
		"const AtmosphereParameters ATMOSPHERE = AtmosphereParameters(\n" +
		to_string(solar_irradiance, lambdas, 1.0) + ",\n" +
		std::to_string(kSunAngularRadius) + ",\n" +
		std::to_string(kBottomRadius / kLengthUnitInMeters) + ",\n" +
		std::to_string(kTopRadius / kLengthUnitInMeters) + ",\n" +
		density_profile(
				{	rayleigh_layer}) + ",\n" +
		to_string(
				rayleigh_scattering, lambdas, kLengthUnitInMeters) + ",\n" +
		density_profile(
				{	mie_layer}) + ",\n" +
		to_string(mie_scattering, lambdas, kLengthUnitInMeters) + ",\n" +
		to_string(mie_extinction, lambdas, kLengthUnitInMeters) + ",\n" +
		std::to_string(kMiePhaseFunctionG) + ",\n" +
		density_profile(ozone_density) + ",\n" +
		to_string(
				absorption_extinction, lambdas, kLengthUnitInMeters) + ",\n" +
		to_string(ground_albedo, lambdas, 1.0) + ",\n" +
		std::to_string(cos(max_sun_zenith_angle)) + ");\n" +
		"const vec3 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(" +
		std::to_string(sky_k_r) + "," +
		std::to_string(sky_k_g) + "," +
		std::to_string(sky_k_b) + ");\n" +
		"const vec3 SUN_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(" +
		std::to_string(sun_k_r) + "," +
		std::to_string(sun_k_g) + "," +
		std::to_string(sun_k_b) + ");\n" +
			FileLoader::instance()->readFile("GameData/shaders/atm-functions.glsl");
	};

	atmosphereShaderInclude = glsl_header_factory_({kLambdaR, kLambdaG, kLambdaB}) +
		      (precompute_illuminance ? "" : "#define RADIANCE_API_ENABLED\n") +
		      kAtmosphereShader;
}
