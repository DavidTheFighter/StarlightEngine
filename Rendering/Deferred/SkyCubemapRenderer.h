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
* SkyCubemapRenderer.h
*
*  Created on: Jun 23, 2018
*      Author: David
*/

#ifndef RENDERING_DEFERRED_SKYCUBEMAPRENDERER_H_
#define RENDERING_DEFERRED_SKYCUBEMAPRENDERER_H_

#include <common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

class Renderer;
class AtmosphereRenderer;

typedef struct
{
	glm::vec3 lightDir;
	float cosSunAngularRadius;
} SkyCubemapRendererPushConsts;

typedef struct
{
	float roughness;
	float invMipSize;
} SkyEnvironmentMapPushConsts;

class SkyCubemapRenderer
{
	public:

	SkyCubemapRenderer(Renderer *rendererPtr, AtmosphereRenderer *atmospherePtr, uint32_t cubemapFaceSize, uint32_t maxEnviroSpecIBLMipCount);
	virtual ~SkyCubemapRenderer();

	void skyboxGenPassInit(RenderPass renderPass, uint32_t baseSubpass);
	void skyboxGenPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size);
	void skyboxGenPassRender(CommandBuffer cmdBuffer, uint32_t counter);

	void enviroSkyboxGenPassInit(RenderPass renderPass, uint32_t baseSubpass);
	void enviroSkyboxGenPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size);
	void enviroSkyboxGenPassRender(CommandBuffer cmdBuffer, uint32_t counter);

	void enviroSkyboxSpecIBLGenPassInit(RenderPass renderPass, uint32_t baseSubpass);
	void enviroSkyboxSpecIBLGenPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size);
	void enviroSkyboxSpecIBLGenPassRender(CommandBuffer cmdBuffer, uint32_t counter);

	void setSunDirection(glm::vec3 sunLightDir);

	Sampler getSkyEnviroCubemapSampler();

	uint32_t getFaceResolution();
	uint32_t getMaxEnviroSpecIBLMipCount();

	private:

	Renderer *renderer;
	AtmosphereRenderer *atmosphere;

	Buffer skyCubemapInvMVPsBuffer;

	glm::vec3 sunDirection;

	Pipeline skyPipelines;
	Pipeline enviroFilterPipeline;

	DescriptorPool skyDescriptorPool;
	DescriptorSet skyDescriptorSet;

	DescriptorPool skyEnviroDescriptorPool;
	DescriptorSet skyEnviroDescriptorSet;

	DescriptorPool skyEnviroOutputDescPool;
	std::vector<DescriptorSet> skyEnviroOutputMipDescSets;

	Sampler atmosphereSampler;
	Sampler enviroInputSampler;
	Sampler enviroCubemapSampler;

	TextureView enviroSkyboxInput;
	TextureView enviroSkyboxSpecIBLOutput;

	uint32_t getMipCount();

	uint32_t faceResolution;
	uint32_t maxEnviroSpecIBLMips;

	void createCubemapPipeline(RenderPass renderPass, uint32_t baseSubpass);
	void createEnvironmentMapPipeline();
};

#endif /* RENDERING_DEFERRED_SKYCUBEMAPRENDERER_H_ */