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
* SkyCubemapRenderer.cpp
*
*  Created on: Jun 23, 2018
*      Author: David
*/

#include "SkyCubemapRenderer.h"

#include <Rendering/Renderer/Renderer.h>

SkyCubemapRenderer::SkyCubemapRenderer(Renderer *rendererPtr)
{
	renderer = rendererPtr;

	destroyed = false;
}

SkyCubemapRenderer::~SkyCubemapRenderer()
{
	if (!destroyed)
		destroy();
}

glm::vec3 cubemapFaceDir[] = {glm::vec3(1, 0, 0), glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, -1, 0), glm::vec3(0, 0, -1), glm::vec3(0, 0, 1)};
glm::vec3 cubemapFaceUp[] = {glm::vec3(0, 1, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0)};

#define SKY_CUBEMAP_MIP_LEVELS 6
#define SUN_ANGULAR_RADIUS 0.99990165129962f

void SkyCubemapRenderer::renderSkyCubemap(CommandBuffer &cmdBuffer, glm::vec3 lightDir)
{
	cmdBuffer->beginDebugRegion("Sky Cubemap", glm::vec4(0, 0.7f, 1, 1));
	cmdBuffer->beginDebugRegion("Skybox", glm::vec4(0, 0.7f, 1, 1));
	cmdBuffer->beginRenderPass(skyRenderPass, skyCubemapFB, {0, 0, faceResolution, faceResolution}, {}, SUBPASS_CONTENTS_INLINE);
	cmdBuffer->setScissors(0, {{0, 0, faceResolution, faceResolution}});
	cmdBuffer->setViewports(0, {{0, 0, (float) faceResolution, (float) faceResolution, 0, 1}});

	for (int i = 0; i < 6; i++)
	{
		glm::mat4 faceProj = glm::perspective<float>(90.0f * (M_PI / 180.0f), 1, 10, 0.01f);
		faceProj[1][1] *= -1;
		glm::mat4 faceView = glm::lookAt<float>(glm::vec3(0), cubemapFaceDir[i], cubemapFaceUp[i]);
		SkyCubemapRendererPushConsts pushConsts = {glm::inverse(faceProj * faceView), glm::normalize(lightDir), 1.1f};

		cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, skyPipelines[i]);
		cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyCubemapRendererPushConsts), &pushConsts);
		cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {skyDescriptorSet});
		cmdBuffer->draw(6);

		if (i < 5)
			cmdBuffer->nextSubpass(SUBPASS_CONTENTS_INLINE);
	}

	cmdBuffer->endRenderPass();
	cmdBuffer->endDebugRegion();

	cmdBuffer->beginDebugRegion("Pre-filtered Enviro-Map", glm::vec4(0, 0.7f, 1, 1));
	cmdBuffer->beginRenderPass(skyRenderPass, skySrcEnviroMapFB, {0, 0, faceResolution, faceResolution}, {}, SUBPASS_CONTENTS_INLINE);
	cmdBuffer->setScissors(0, {{0, 0, faceResolution, faceResolution}});
	cmdBuffer->setViewports(0, {{0, 0, (float) faceResolution, (float) faceResolution, 0, 1}});

	for (int i = 0; i < 6; i++)
	{
		glm::mat4 faceProj = glm::perspective<float>(90.0f * (M_PI / 180.0f), 1, 10, 0.01f);
		faceProj[1][1] *= -1;
		glm::mat4 faceView = glm::lookAt<float>(glm::vec3(0), cubemapFaceDir[i], cubemapFaceUp[i]);
		SkyCubemapRendererPushConsts pushConsts = {glm::inverse(faceProj * faceView), glm::normalize(lightDir), SUN_ANGULAR_RADIUS};

		cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, skyPipelines[i]);
		cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyCubemapRendererPushConsts), &pushConsts);
		cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {skyDescriptorSet});
		cmdBuffer->draw(6);

		if (i < 5)
			cmdBuffer->nextSubpass(SUBPASS_CONTENTS_INLINE);
	}

	cmdBuffer->endRenderPass();
	cmdBuffer->endDebugRegion();

	cmdBuffer->beginDebugRegion("Mip Generation", glm::vec4(0, 0.7f, 1, 1));
	uint32_t skyCubemapTextureMips = (uint32_t) glm::floor(glm::log2((float) faceResolution)) + 1;

	cmdBuffer->setTextureLayout(skySrcEnviroMapTexture, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 6}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_TRANSFER_BIT);

	for (uint32_t m = 1; m < skyCubemapTextureMips; m++)
	{
		TextureSubresourceRange mipRange = {};
		mipRange.baseMipLevel = m;
		mipRange.levelCount = 1;
		mipRange.baseArrayLayer = 0;
		mipRange.layerCount = 6;

		TextureBlitInfo blitInfo = {};
		blitInfo.srcSubresource = {m - 1, 0, 6};
		blitInfo.dstSubresource = {m, 0, 6};
		blitInfo.srcOffsets[0] = {0, 0, 0};
		blitInfo.dstOffsets[0] = {0, 0, 0};
		blitInfo.srcOffsets[1] = {int32_t(faceResolution >> (m - 1)), int32_t(faceResolution >> (m - 1)), 1};
		blitInfo.dstOffsets[1] = {int32_t(faceResolution >> m), int32_t(faceResolution >> m), 1};

		cmdBuffer->setTextureLayout(skySrcEnviroMapTexture, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, mipRange, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_TRANSFER_BIT);
		cmdBuffer->blitTexture(skySrcEnviroMapTexture, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, skySrcEnviroMapTexture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {blitInfo}, SAMPLER_FILTER_LINEAR);

		if (m < skyCubemapTextureMips - 1)
			cmdBuffer->setTextureLayout(skySrcEnviroMapTexture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, mipRange, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_TRANSFER_BIT);
		else
			cmdBuffer->setTextureLayout(skySrcEnviroMapTexture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipRange, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);

		if (m > 1)
			cmdBuffer->setTextureLayout(skySrcEnviroMapTexture, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {m - 1, 1, 0, 6}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}

	cmdBuffer->endDebugRegion();
	cmdBuffer->beginDebugRegion("Enviro-Map Gen");

	uint32_t mipmapCount = getMipCount();

	// Because the first mip is a mirror-reflection, don't bother doing the fancy importance sampling, just blit it over from the unfiltered environment map
	{
		TextureBlitInfo blitInfo = {};
		blitInfo.srcSubresource = {0, 0, 6};
		blitInfo.dstSubresource = {0, 0, 6};
		blitInfo.srcOffsets[0] = {0, 0, 0};
		blitInfo.dstOffsets[0] = {0, 0, 0};
		blitInfo.srcOffsets[1] = {int32_t(faceResolution), int32_t(faceResolution), 1};
		blitInfo.dstOffsets[1] = {int32_t(faceResolution), int32_t(faceResolution), 1};

		cmdBuffer->setTextureLayout(skyFilteredEnviroMapTexture, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {0, 1, 0, 6}, PIPELINE_STAGE_HOST_BIT, PIPELINE_STAGE_TRANSFER_BIT);
		cmdBuffer->blitTexture(skySrcEnviroMapTexture, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, skyFilteredEnviroMapTexture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {blitInfo}, SAMPLER_FILTER_NEAREST);
		cmdBuffer->setTextureLayout(skyFilteredEnviroMapTexture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 6}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
		cmdBuffer->setTextureLayout(skySrcEnviroMapTexture, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 6}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}

	for (uint32_t m = 1; m < mipmapCount; m++)
	{
		cmdBuffer->beginRenderPass(skyEnvironmentRenderPass, skyFilteredEnviroMapMipFBs[m], {0, 0, faceResolution >> m, faceResolution >> m}, {}, SUBPASS_CONTENTS_INLINE);
		cmdBuffer->setScissors(0, {{0, 0, faceResolution >> m, faceResolution >> m}});
		cmdBuffer->setViewports(0, {{0, 0, (float) (faceResolution >> m), (float) (faceResolution >> m), 0, 1}});

		for (uint32_t f = 0; f < 6; f++)
		{
			glm::mat4 faceProj = glm::perspective<float>(90.0f * (M_PI / 180.0f), 1, 10, 0.01f);
			faceProj[1][1] *= -1;
			glm::mat4 faceView = glm::lookAt<float>(glm::vec3(0), cubemapFaceDir[f], cubemapFaceUp[f]);
			SkyEnvironmentMapPushConsts pushConsts = {glm::inverse(faceProj * faceView), (float(m) / float(mipmapCount - 1))};

			cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, enviroFilterPipelines[f]);
			cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyEnvironmentMapPushConsts), &pushConsts);
			cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {skyEnviroDescriptorSet});
			cmdBuffer->draw(6);

			if (f < 5)
				cmdBuffer->nextSubpass(SUBPASS_CONTENTS_INLINE);
		}

		cmdBuffer->endRenderPass();
	}

	cmdBuffer->endDebugRegion();
	cmdBuffer->endDebugRegion();
}

void SkyCubemapRenderer::init(uint32_t cubemapFaceResolution, const std::string &atmosphericShaderLib, std::vector<TextureView> atmosphereTextures, Sampler atmosphereSampler)
{
	faceResolution = cubemapFaceResolution;

	createRenderPasses();
	createCubemapPipeline(atmosphericShaderLib);
	createEnvironmentMapPipeline();

	uint32_t skyCubemapTextureMips = (uint32_t) glm::floor(glm::log2((float) faceResolution)) + 1;

	// Create the sky cubemap for the lighting shader

	skyCubemapTexture = renderer->createTexture({(float) cubemapFaceResolution, (float) cubemapFaceResolution, 1}, RESOURCE_FORMAT_B10G11R11_UFLOAT_PACK32, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_COLOR_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true, 1, 6);
	skyCubemapTV = renderer->createTextureView(skyCubemapTexture, TEXTURE_VIEW_TYPE_CUBE, {0, 1, 0, 6});

	for (uint32_t f = 0; f < 6; f++)
		skyCubemapFaceTV[f] = renderer->createTextureView(skyCubemapTexture, TEXTURE_VIEW_TYPE_2D, {0, 1, f, 1});

	skyCubemapFB = renderer->createFramebuffer(skyRenderPass, std::vector<TextureView>(skyCubemapFaceTV, skyCubemapFaceTV + 6), faceResolution, faceResolution);

	// Create the pre-filtered environment map

	skySrcEnviroMapTexture = renderer->createTexture({(float) faceResolution, (float) faceResolution, 1}, RESOURCE_FORMAT_B10G11R11_UFLOAT_PACK32, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | TEXTURE_USAGE_TRANSFER_DST_BIT | TEXTURE_USAGE_TRANSFER_SRC_BIT, MEMORY_USAGE_GPU_ONLY, true, skyCubemapTextureMips, 6);
	skySrcEnviroMapTV = renderer->createTextureView(skySrcEnviroMapTexture, TEXTURE_VIEW_TYPE_CUBE, {0, skyCubemapTextureMips, 0, 6});

	for (uint32_t f = 0; f < 6; f++)
		skySrcEnviroMapFaceTV[f] = renderer->createTextureView(skySrcEnviroMapTexture, TEXTURE_VIEW_TYPE_2D, {0, 1, f, 1});

	skySrcEnviroMapFB = renderer->createFramebuffer(skyRenderPass, std::vector<TextureView>(skySrcEnviroMapFaceTV, skySrcEnviroMapFaceTV + 6), faceResolution, faceResolution);

	// Create the post-filtered environment map/specular IBL texture

	uint32_t mipmapCount = getMipCount();

	skyFilteredEnviroMapTexture = renderer->createTexture({(float) cubemapFaceResolution, (float) cubemapFaceResolution, 1}, RESOURCE_FORMAT_B10G11R11_UFLOAT_PACK32, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | TEXTURE_USAGE_TRANSFER_DST_BIT, MEMORY_USAGE_GPU_ONLY, true, mipmapCount, 6);
	skyFilteredEnviroMapTV = renderer->createTextureView(skyFilteredEnviroMapTexture, TEXTURE_VIEW_TYPE_CUBE, {0, mipmapCount, 0, 6});

	for (uint32_t m = 0; m < mipmapCount; m++)
		for (uint32_t f = 0; f < 6; f++)
			skyFilteredEnviroMapTVs.push_back(renderer->createTextureView(skyFilteredEnviroMapTexture, TEXTURE_VIEW_TYPE_2D, {m, 1, f, 1}));

	for (uint32_t m = 0; m < mipmapCount; m++)
		skyFilteredEnviroMapMipFBs.push_back(renderer->createFramebuffer(skyEnvironmentRenderPass, std::vector<TextureView>(skyFilteredEnviroMapTVs.data() + m * 6, skyFilteredEnviroMapTVs.data() + m * 6 + 6), cubemapFaceResolution >> m, cubemapFaceResolution >> m));

	skyDescriptorPool = renderer->createDescriptorPool({
		{0, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{1, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{2, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{3, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT}
		}, 1);
	skyDescriptorSet = skyDescriptorPool->allocateDescriptorSet();

	skyEnviroDescriptorPool = renderer->createDescriptorPool({
		{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		}, 1);
	skyEnviroDescriptorSet = skyEnviroDescriptorPool->allocateDescriptorSet();

	enviroInputSampler = renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SAMPLER_FILTER_LINEAR, SAMPLER_FILTER_LINEAR, 1, {0, (float) skyCubemapTextureMips, 0});
	enviroCubemapSampler = renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SAMPLER_FILTER_LINEAR, SAMPLER_FILTER_LINEAR, 1, {0, (float) mipmapCount, 0});

	DescriptorImageInfo transmittanceImageInfo = {};
	transmittanceImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	transmittanceImageInfo.sampler = atmosphereSampler;
	transmittanceImageInfo.view = atmosphereTextures[0];

	DescriptorImageInfo scatteringImageInfo = {};
	scatteringImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	scatteringImageInfo.sampler = atmosphereSampler;
	scatteringImageInfo.view = atmosphereTextures[1];

	DescriptorImageInfo singleMieScatteringImageInfo = {};
	singleMieScatteringImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	singleMieScatteringImageInfo.sampler = atmosphereSampler;
	singleMieScatteringImageInfo.view = atmosphereTextures[2];

	DescriptorImageInfo irradianceImageInfo = {};
	irradianceImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	irradianceImageInfo.sampler = atmosphereSampler;
	irradianceImageInfo.view = atmosphereTextures[3];

	DescriptorImageInfo enviroInputImageInfo = {};
	enviroInputImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	enviroInputImageInfo.sampler = enviroInputSampler;
	enviroInputImageInfo.view = skySrcEnviroMapTV;

	DescriptorWriteInfo transmittanceWrite = {}, scatteringWrite = {}, mieWrite = {}, irradianceWrite = {}, enviroSamplerWrite = {}, enviroTextureWrite = {};
	transmittanceWrite.descriptorCount = 1;
	transmittanceWrite.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	transmittanceWrite.dstBinding = 0;
	transmittanceWrite.dstSet = skyDescriptorSet;
	transmittanceWrite.imageInfo = {transmittanceImageInfo};

	scatteringWrite.descriptorCount = 1;
	scatteringWrite.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	scatteringWrite.dstBinding = 1;
	scatteringWrite.dstSet = skyDescriptorSet;
	scatteringWrite.imageInfo = {scatteringImageInfo};

	mieWrite.descriptorCount = 1;
	mieWrite.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	mieWrite.dstBinding = 2;
	mieWrite.dstSet = skyDescriptorSet;
	mieWrite.imageInfo = {singleMieScatteringImageInfo};

	irradianceWrite.descriptorCount = 1;
	irradianceWrite.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	irradianceWrite.dstBinding = 3;
	irradianceWrite.dstSet = skyDescriptorSet;
	irradianceWrite.imageInfo = {irradianceImageInfo};

	enviroSamplerWrite.descriptorCount = 1;
	enviroSamplerWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLER;
	enviroSamplerWrite.dstBinding = 0;
	enviroSamplerWrite.dstSet = skyEnviroDescriptorSet;
	enviroSamplerWrite.imageInfo = {enviroInputImageInfo};

	enviroTextureWrite.descriptorCount = 1;
	enviroTextureWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	enviroTextureWrite.dstBinding = 1;
	enviroTextureWrite.dstSet = skyEnviroDescriptorSet;
	enviroTextureWrite.imageInfo = {enviroInputImageInfo};

	renderer->writeDescriptorSets({transmittanceWrite, scatteringWrite, mieWrite, irradianceWrite, enviroSamplerWrite, enviroTextureWrite});
}

void SkyCubemapRenderer::destroy()
{
	destroyed = true;

	renderer->destroySampler(enviroInputSampler);
	renderer->destroySampler(enviroCubemapSampler);

	renderer->destroyTexture(skyCubemapTexture);
	renderer->destroyTexture(skySrcEnviroMapTexture);
	renderer->destroyTexture(skyFilteredEnviroMapTexture);

	renderer->destroyTextureView(skyCubemapTV);
	renderer->destroyTextureView(skySrcEnviroMapTV);
	renderer->destroyTextureView(skyFilteredEnviroMapTV);

	for (size_t i = 0; i < 6; i++)
	{
		renderer->destroyTextureView(skyCubemapFaceTV[i]);
		renderer->destroyTextureView(skySrcEnviroMapFaceTV[i]);
	}

	for (size_t i = 0; i < skyFilteredEnviroMapTVs.size(); i++)
		renderer->destroyTextureView(skyFilteredEnviroMapTVs[i]);

	renderer->destroyFramebuffer(skyCubemapFB);
	renderer->destroyFramebuffer(skySrcEnviroMapFB);

	for (size_t i = 0; i < skyFilteredEnviroMapMipFBs.size(); i++)
		renderer->destroyFramebuffer(skyFilteredEnviroMapMipFBs[i]);

	renderer->destroyDescriptorPool(skyDescriptorPool);
	renderer->destroyDescriptorPool(skyEnviroDescriptorPool);

	for (int i = 0; i < 6; i++)
		renderer->destroyPipeline(skyPipelines[i]);

	for (size_t i = 0; i < enviroFilterPipelines.size(); i++)
		renderer->destroyPipeline(enviroFilterPipelines[i]);
	
	renderer->destroyRenderPass(skyRenderPass);
	renderer->destroyRenderPass(skyEnvironmentRenderPass);
}

TextureView SkyCubemapRenderer::getSkyCubemapTextureView()
{
	return skyCubemapTV;
}

TextureView SkyCubemapRenderer::getSkyEnviroCubemapTextureView()
{
	return skyFilteredEnviroMapTV;
}

Sampler SkyCubemapRenderer::getSkyEnviroCubemapSampler()
{
	return enviroCubemapSampler;
}

void SkyCubemapRenderer::createRenderPasses()
{
	{
		std::vector<AttachmentDescription> cubemapFaceAttachments;
		std::vector<SubpassDescription> cubemapFaceSubpasses;

		for (uint32_t i = 0; i < 6; i++)
		{
			AttachmentDescription outputAttachment = {};
			outputAttachment.format = RESOURCE_FORMAT_B10G11R11_UFLOAT_PACK32;
			outputAttachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
			outputAttachment.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			outputAttachment.loadOp = ATTACHMENT_LOAD_OP_DONT_CARE;
			outputAttachment.storeOp = ATTACHMENT_STORE_OP_STORE;

			cubemapFaceAttachments.push_back(outputAttachment);
		}

		for (int i = 0; i < 6; i++)
		{
			AttachmentReference cubemapFaceRef = {};
			cubemapFaceRef.attachment = (uint32_t) i;
			cubemapFaceRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			SubpassDescription subpass = {};
			subpass.bindPoint = PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachments = {cubemapFaceRef};
			subpass.depthStencilAttachment = nullptr;

			cubemapFaceSubpasses.push_back(subpass);
		}

		skyRenderPass = renderer->createRenderPass(cubemapFaceAttachments, cubemapFaceSubpasses, {});
	}

	{
		std::vector<AttachmentDescription> cubemapFaceAttachments;
		std::vector<SubpassDescription> cubemapFaceSubpasses;

		for (uint32_t f = 0; f < 6; f++)
		{
			AttachmentDescription outputAttachment = {};
			outputAttachment.format = RESOURCE_FORMAT_B10G11R11_UFLOAT_PACK32;
			outputAttachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
			outputAttachment.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			outputAttachment.loadOp = ATTACHMENT_LOAD_OP_DONT_CARE;
			outputAttachment.storeOp = ATTACHMENT_STORE_OP_STORE;

			cubemapFaceAttachments.push_back(outputAttachment);
		}

		for (uint32_t f = 0; f < 6; f++)
		{
			AttachmentReference cubemapFaceRef = {};
			cubemapFaceRef.attachment = f;
			cubemapFaceRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			SubpassDescription subpass = {};
			subpass.bindPoint = PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachments = {cubemapFaceRef};
			subpass.depthStencilAttachment = nullptr;

			cubemapFaceSubpasses.push_back(subpass);
		}

		skyEnvironmentRenderPass = renderer->createRenderPass(cubemapFaceAttachments, cubemapFaceSubpasses, {});
	}
}

void SkyCubemapRenderer::createCubemapPipeline(const std::string &atmosphericShaderLib)
{
	const std::string insertMarker = "#SE_BUILTIN_INCLUDE_ATMOSPHERE_LIB";
	const std::string shaderSourceFile = "GameData/shaders/vulkan/sky-cubemap.glsl";

	std::string lightingSource = FileLoader::instance()->readFile(shaderSourceFile);
	size_t insertPos = lightingSource.find(insertMarker);

	if (insertPos != std::string::npos)
	{
		lightingSource.replace(insertPos, insertMarker.length(), atmosphericShaderLib);
	}
	else
	{
		printf("%s Didn't find location to insert atmopsheric shader lib into the deferred lighting shader\n", WARN_PREFIX);

		lightingSource.replace(insertPos, insertMarker.length(), "");
	}

	ShaderModule vertShader = renderer->createShaderModuleFromSource(lightingSource, shaderSourceFile, SHADER_STAGE_VERTEX_BIT);
	ShaderModule fragShader = renderer->createShaderModuleFromSource(lightingSource, shaderSourceFile, SHADER_STAGE_FRAGMENT_BIT);

	PipelineShaderStage vertShaderStage = {};
	vertShaderStage.entry = "main";
	vertShaderStage.module = vertShader;

	PipelineShaderStage fragShaderStage = {};
	fragShaderStage.entry = "main";
	fragShaderStage.module = fragShader;

	PipelineVertexInputInfo vertexInput = {};
	vertexInput.vertexInputAttribs = {};
	vertexInput.vertexInputBindings = {};

	PipelineInputAssemblyInfo inputAssembly = {};
	inputAssembly.primitiveRestart = false;
	inputAssembly.topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	PipelineViewportInfo viewportInfo = {};
	viewportInfo.scissors = {{0, 0, 256, 256}};
	viewportInfo.viewports = {{0, 0, 256, 256}};

	PipelineRasterizationInfo rastInfo = {};
	rastInfo.clockwiseFrontFace = false;
	rastInfo.cullMode = CULL_MODE_NONE;
	rastInfo.lineWidth = 1;
	rastInfo.polygonMode = POLYGON_MODE_FILL;
	rastInfo.rasterizerDiscardEnable = false;

	PipelineDepthStencilInfo depthInfo = {};
	depthInfo.enableDepthTest = false;
	depthInfo.enableDepthWrite = false;
	depthInfo.minDepthBounds = 0;
	depthInfo.maxDepthBounds = 1;
	depthInfo.depthCompareOp = COMPARE_OP_ALWAYS;

	PipelineColorBlendAttachment colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = false;
	colorBlendAttachment.colorWriteMask = COLOR_COMPONENT_R_BIT | COLOR_COMPONENT_G_BIT | COLOR_COMPONENT_B_BIT | COLOR_COMPONENT_A_BIT;

	PipelineColorBlendInfo colorBlend = {};
	colorBlend.attachments = {colorBlendAttachment};
	colorBlend.logicOpEnable = false;
	colorBlend.logicOp = LOGIC_OP_COPY;
	colorBlend.blendConstants[0] = 1.0f;
	colorBlend.blendConstants[1] = 1.0f;
	colorBlend.blendConstants[2] = 1.0f;
	colorBlend.blendConstants[3] = 1.0f;

	PipelineDynamicStateInfo dynamicState = {};
	dynamicState.dynamicStates = {DYNAMIC_STATE_VIEWPORT, DYNAMIC_STATE_SCISSOR};

	PipelineInfo info = {};
	info.stages = {vertShaderStage, fragShaderStage};
	info.vertexInputInfo = vertexInput;
	info.inputAssemblyInfo = inputAssembly;
	info.viewportInfo = viewportInfo;
	info.rasterizationInfo = rastInfo;
	info.depthStencilInfo = depthInfo;
	info.colorBlendInfo = colorBlend;
	info.dynamicStateInfo = dynamicState;

	info.inputPushConstantRanges = {{0, sizeof(SkyCubemapRendererPushConsts), SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_FRAGMENT_BIT}};
	info.inputSetLayouts = {{
		{0, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{1, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{2, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{3, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT}
		}};

	for (int i = 0; i < 6; i++)
		skyPipelines[i] = renderer->createGraphicsPipeline(info, skyEnvironmentRenderPass, (uint32_t) i);

	renderer->destroyShaderModule(vertShaderStage.module);
	renderer->destroyShaderModule(fragShaderStage.module);
}

void SkyCubemapRenderer::createEnvironmentMapPipeline()
{
	ShaderModule vertShader = renderer->createShaderModule("GameData/shaders/vulkan/sky-enviro.glsl", SHADER_STAGE_VERTEX_BIT);
	ShaderModule fragShader = renderer->createShaderModule("GameData/shaders/vulkan/sky-enviro.glsl", SHADER_STAGE_FRAGMENT_BIT);

	PipelineShaderStage vertShaderStage = {};
	vertShaderStage.entry = "main";
	vertShaderStage.module = vertShader;

	PipelineShaderStage fragShaderStage = {};
	fragShaderStage.entry = "main";
	fragShaderStage.module = fragShader;

	PipelineVertexInputInfo vertexInput = {};
	vertexInput.vertexInputAttribs = {};
	vertexInput.vertexInputBindings = {};

	PipelineInputAssemblyInfo inputAssembly = {};
	inputAssembly.primitiveRestart = false;
	inputAssembly.topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	PipelineViewportInfo viewportInfo = {};
	viewportInfo.scissors = {{0, 0, 256, 256}};
	viewportInfo.viewports = {{0, 0, 256, 256}};

	PipelineRasterizationInfo rastInfo = {};
	rastInfo.clockwiseFrontFace = false;
	rastInfo.cullMode = CULL_MODE_NONE;
	rastInfo.lineWidth = 1;
	rastInfo.polygonMode = POLYGON_MODE_FILL;
	rastInfo.rasterizerDiscardEnable = false;

	PipelineDepthStencilInfo depthInfo = {};
	depthInfo.enableDepthTest = false;
	depthInfo.enableDepthWrite = false;
	depthInfo.minDepthBounds = 0;
	depthInfo.maxDepthBounds = 1;
	depthInfo.depthCompareOp = COMPARE_OP_ALWAYS;

	PipelineColorBlendAttachment colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = false;
	colorBlendAttachment.colorWriteMask = COLOR_COMPONENT_R_BIT | COLOR_COMPONENT_G_BIT | COLOR_COMPONENT_B_BIT | COLOR_COMPONENT_A_BIT;

	PipelineColorBlendInfo colorBlend = {};
	colorBlend.attachments = {colorBlendAttachment};
	colorBlend.logicOpEnable = false;
	colorBlend.logicOp = LOGIC_OP_COPY;
	colorBlend.blendConstants[0] = 1.0f;
	colorBlend.blendConstants[1] = 1.0f;
	colorBlend.blendConstants[2] = 1.0f;
	colorBlend.blendConstants[3] = 1.0f;

	PipelineDynamicStateInfo dynamicState = {};
	dynamicState.dynamicStates = {DYNAMIC_STATE_VIEWPORT, DYNAMIC_STATE_SCISSOR};

	PipelineInfo info = {};
	info.stages = {vertShaderStage, fragShaderStage};
	info.vertexInputInfo = vertexInput;
	info.inputAssemblyInfo = inputAssembly;
	info.viewportInfo = viewportInfo;
	info.rasterizationInfo = rastInfo;
	info.depthStencilInfo = depthInfo;
	info.colorBlendInfo = colorBlend;
	info.dynamicStateInfo = dynamicState;

	info.inputPushConstantRanges = {{0, sizeof(SkyEnvironmentMapPushConsts), SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_FRAGMENT_BIT}};
	info.inputSetLayouts = {{
		{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		}};

	for (uint32_t f = 0; f < 6; f++)
		enviroFilterPipelines.push_back(renderer->createGraphicsPipeline(info, skyEnvironmentRenderPass, f));

	renderer->destroyShaderModule(vertShaderStage.module);
	renderer->destroyShaderModule(fragShaderStage.module);
}

uint32_t SkyCubemapRenderer::getMipCount()
{
	return glm::min<uint32_t>((uint32_t) glm::floor(glm::log2((float) faceResolution)) + 1, SKY_CUBEMAP_MIP_LEVELS);
}