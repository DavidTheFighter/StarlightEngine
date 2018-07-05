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

typedef struct
{
	glm::mat4 invCamMVPMat;
	glm::vec3 lightDir;
	float cosSunAngularRadius;
} SkyCubemapRendererPushConsts;

typedef struct
{
	glm::mat4 invCamMVPMat;
	float roughness;
} SkyEnvironmentMapPushConsts;

class SkyCubemapRenderer
{
	public:

	SkyCubemapRenderer(Renderer *rendererPtr);
	virtual ~SkyCubemapRenderer();

	void renderSkyCubemap(CommandBuffer &cmdBuffer, glm::vec3 lightDir);

	void init(uint32_t cubemapFaceResolution, const std::string &atmosphericShaderLib, std::vector<TextureView> atmosphereTextures, Sampler atmosphereSampler);
	void destroy();

	TextureView getSkyCubemapTextureView();
	TextureView getSkyEnviroCubemapTextureView();

	Sampler getSkyEnviroCubemapSampler();

	private:

	bool destroyed;

	Renderer *renderer;

	Pipeline skyPipelines[6];
	std::vector<Pipeline> enviroFilterPipelines;
	RenderPass skyRenderPass;
	RenderPass skyEnvironmentRenderPass;

	DescriptorPool skyDescriptorPool;
	DescriptorSet skyDescriptorSet;

	DescriptorPool skyEnviroDescriptorPool;
	DescriptorSet skyEnviroDescriptorSet;

	Texture skyCubemapTexture;
	TextureView skyCubemapFaceTV[6];
	TextureView skyCubemapTV;
	Framebuffer skyCubemapFB;

	Texture skySrcEnviroMapTexture;
	TextureView skySrcEnviroMapTV;
	TextureView skySrcEnviroMapFaceTV[6];
	Framebuffer skySrcEnviroMapFB;

	Texture skyFilteredEnviroMapTexture;
	TextureView skyFilteredEnviroMapTV;
	std::vector<TextureView> skyFilteredEnviroMapTVs; // A texture view of each face's mipmap, skyFilteredEnviroMapTVs[face * <mip_count> + mip_lvl]
	std::vector<Framebuffer> skyFilteredEnviroMapMipFBs;

	Sampler enviroInputSampler;
	Sampler enviroCubemapSampler;

	uint32_t getMipCount();

	uint32_t faceResolution;

	void createCubemapPipeline(const std::string &atmosphericShaderLib);
	void createEnvironmentMapPipeline();
	void createRenderPasses();
};

#endif /* RENDERING_DEFERRED_SKYCUBEMAPRENDERER_H_ */