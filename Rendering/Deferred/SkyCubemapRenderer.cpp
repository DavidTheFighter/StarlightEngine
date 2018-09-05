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
#include <Rendering/Deferred/AtmosphereRenderer.h>

glm::vec3 cubemapFaceDir[] = {glm::vec3(1, 0, 0), glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, -1, 0), glm::vec3(0, 0, -1), glm::vec3(0, 0, 1)};
glm::vec3 cubemapFaceUp[] = {glm::vec3(0, 1, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0)};

SkyCubemapRenderer::SkyCubemapRenderer(Renderer *rendererPtr, AtmosphereRenderer *atmospherePtr, uint32_t cubemapFaceSize, uint32_t maxEnviroSpecIBLMipCount)
{
	renderer = rendererPtr;
	atmosphere = atmospherePtr;
	faceResolution = cubemapFaceSize;
	maxEnviroSpecIBLMips = maxEnviroSpecIBLMipCount;

	skyCubemapInvMVPsBuffer = renderer->createBuffer(sizeof(glm::mat4) * 6, BUFFER_USAGE_UNIFORM_BUFFER, true, false, MEMORY_USAGE_CPU_ONLY);

	glm::mat4 invCamMVPs[6];

	glm::mat4 faceProj = glm::perspective<float>(90.0f * (M_PI / 180.0f), 1, 10, 0.01f);
	faceProj[1][1] *= -1;

	for (int i = 0; i < 6; i++)
		invCamMVPs[i] = glm::inverse(faceProj * glm::lookAt<float>(glm::vec3(0), cubemapFaceDir[i], cubemapFaceUp[i]));
	
	void *skyCubemapInvMVPsBufferDataPtr = renderer->mapBuffer(skyCubemapInvMVPsBuffer);
	memcpy(skyCubemapInvMVPsBufferDataPtr, &invCamMVPs[0][0][0], sizeof(glm::mat4) * 6);
	renderer->unmapBuffer(skyCubemapInvMVPsBuffer);
}

SkyCubemapRenderer::~SkyCubemapRenderer()
{
	renderer->destroySampler(enviroInputSampler);
	renderer->destroySampler(enviroCubemapSampler);
	renderer->destroySampler(atmosphereSampler);

	renderer->destroyDescriptorPool(skyDescriptorPool);
	renderer->destroyDescriptorPool(skyEnviroDescriptorPool);
	renderer->destroyDescriptorPool(skyEnviroOutputDescPool);

	renderer->destroyPipeline(skyPipelines);

	renderer->destroyPipeline(enviroFilterPipeline);

	renderer->destroyBuffer(skyCubemapInvMVPsBuffer);
}

#define SUN_ANGULAR_RADIUS 0.99990165129962f

void SkyCubemapRenderer::skyboxGenPassInit(RenderPass renderPass, uint32_t baseSubpass)
{
	createCubemapPipeline(renderPass, baseSubpass);

	atmosphereSampler = renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	skyDescriptorPool = renderer->createDescriptorPool({
		{0, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{1, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{2, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{3, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{4, DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, SHADER_STAGE_GEOMETRY_BIT}
		}, 1);
	skyDescriptorSet = skyDescriptorPool->allocateDescriptorSet();

	DescriptorImageInfo transmittanceImageInfo = {};
	transmittanceImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	transmittanceImageInfo.sampler = atmosphereSampler;
	transmittanceImageInfo.view = atmosphere->transmittanceTV;

	DescriptorImageInfo scatteringImageInfo = {};
	scatteringImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	scatteringImageInfo.sampler = atmosphereSampler;
	scatteringImageInfo.view = atmosphere->scatteringTV;

	DescriptorImageInfo singleMieScatteringImageInfo = {};
	singleMieScatteringImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	singleMieScatteringImageInfo.sampler = atmosphereSampler;
	singleMieScatteringImageInfo.view = atmosphere->scatteringTV;

	DescriptorImageInfo irradianceImageInfo = {};
	irradianceImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	irradianceImageInfo.sampler = atmosphereSampler;
	irradianceImageInfo.view = atmosphere->irradianceTV;

	DescriptorBufferInfo invMVPsBufferInfo = {};
	invMVPsBufferInfo.buffer = skyCubemapInvMVPsBuffer;
	invMVPsBufferInfo.range = sizeof(glm::mat4) * 6;

	DescriptorWriteInfo transmittanceWrite = {}, scatteringWrite = {}, mieWrite = {}, irradianceWrite = {}, invMVPsWrite = {};
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

	invMVPsWrite.descriptorCount = 1;
	invMVPsWrite.descriptorType = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	invMVPsWrite.dstBinding = 4;
	invMVPsWrite.dstSet = skyDescriptorSet;
	invMVPsWrite.bufferInfo = {invMVPsBufferInfo};

	renderer->writeDescriptorSets({transmittanceWrite, scatteringWrite, mieWrite, irradianceWrite, invMVPsWrite});
}

void SkyCubemapRenderer::skyboxGenPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size)
{
}

void SkyCubemapRenderer::skyboxGenPassRender(CommandBuffer cmdBuffer, uint32_t counter)
{
	glm::mat4 faceProj = glm::perspective<float>(90.0f * (M_PI / 180.0f), 1, 10, 0.01f);
	faceProj[1][1] *= -1;
	glm::mat4 faceView = glm::lookAt<float>(glm::vec3(0), cubemapFaceDir[0], cubemapFaceUp[0]);
	SkyCubemapRendererPushConsts pushConsts = {glm::normalize(sunDirection), 1.1f};

	cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, skyPipelines);
	cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyCubemapRendererPushConsts), &pushConsts);
	cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {skyDescriptorSet});
	cmdBuffer->draw(6);
}

void SkyCubemapRenderer::enviroSkyboxGenPassInit(RenderPass renderPass, uint32_t baseSubpass)
{
}

void SkyCubemapRenderer::enviroSkyboxGenPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size)
{
}

void SkyCubemapRenderer::enviroSkyboxGenPassRender(CommandBuffer cmdBuffer, uint32_t counter)
{
	glm::mat4 faceProj = glm::perspective<float>(90.0f * (M_PI / 180.0f), 1, 10, 0.01f);
	faceProj[1][1] *= -1;
	glm::mat4 faceView = glm::lookAt<float>(glm::vec3(0), cubemapFaceDir[0], cubemapFaceUp[0]);
	SkyCubemapRendererPushConsts pushConsts = {glm::normalize(sunDirection), SUN_ANGULAR_RADIUS};

	cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, skyPipelines);
	cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyCubemapRendererPushConsts), &pushConsts);
	cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {skyDescriptorSet});
	cmdBuffer->draw(6);
}

void SkyCubemapRenderer::enviroSkyboxSpecIBLGenPassInit(RenderPass renderPass, uint32_t baseSubpass)
{
	skyEnviroDescriptorPool = renderer->createDescriptorPool({
		{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_COMPUTE_BIT},
		{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_COMPUTE_BIT},
		{2, DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, SHADER_STAGE_COMPUTE_BIT}
	}, 1);
	skyEnviroDescriptorSet = skyEnviroDescriptorPool->allocateDescriptorSet();

	skyEnviroOutputDescPool = renderer->createDescriptorPool({
		{0, DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, SHADER_STAGE_COMPUTE_BIT}
	}, 1);
	skyEnviroOutputMipDescSets = skyEnviroOutputDescPool->allocateDescriptorSets(getMipCount() - 1);

	enviroInputSampler = renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SAMPLER_FILTER_LINEAR, SAMPLER_FILTER_LINEAR, 1, {0, glm::floor(glm::log2((float) faceResolution)) + 1, 0});
	enviroCubemapSampler = renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SAMPLER_FILTER_LINEAR, SAMPLER_FILTER_LINEAR, 1, {0, (float) getMipCount(), 0});

	DescriptorImageInfo enviroInputSamplerInfo = {};
	enviroInputSamplerInfo.sampler = enviroInputSampler;

	DescriptorBufferInfo invMVPsBufferInfo = {};
	invMVPsBufferInfo.buffer = skyCubemapInvMVPsBuffer;
	invMVPsBufferInfo.range = sizeof(glm::mat4) * 6;

	DescriptorWriteInfo enviroSamplerWrite = {}, invMVPsBufferWrite = {};
	enviroSamplerWrite.descriptorCount = 1;
	enviroSamplerWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLER;
	enviroSamplerWrite.dstBinding = 0;
	enviroSamplerWrite.dstSet = skyEnviroDescriptorSet;
	enviroSamplerWrite.imageInfo = {enviroInputSamplerInfo};

	invMVPsBufferWrite.descriptorCount = 1;
	invMVPsBufferWrite.descriptorType = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	invMVPsBufferWrite.dstBinding = 2;
	invMVPsBufferWrite.dstSet = skyEnviroDescriptorSet;
	invMVPsBufferWrite.bufferInfo = {invMVPsBufferInfo};

	renderer->writeDescriptorSets({enviroSamplerWrite, invMVPsBufferWrite});

	createEnvironmentMapPipeline();
}

void SkyCubemapRenderer::enviroSkyboxSpecIBLGenPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size)
{
	DescriptorImageInfo enviroInputImageInfo = {};
	enviroInputImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	enviroInputImageInfo.view = views["enviroSkybox"];

	std::vector<DescriptorWriteInfo> writes = {};

	DescriptorWriteInfo enviroTextureWrite = {};
	enviroTextureWrite.descriptorCount = 1;
	enviroTextureWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	enviroTextureWrite.dstBinding = 1;
	enviroTextureWrite.dstSet = skyEnviroDescriptorSet;
	enviroTextureWrite.imageInfo = {enviroInputImageInfo};

	writes.push_back(enviroTextureWrite);

	uint32_t mipCount = getMipCount();
	for (uint32_t m = 1; m < mipCount; m++)
	{
		DescriptorImageInfo enviroSpecIBLOutputImageInfo = {};
		enviroSpecIBLOutputImageInfo.layout = TEXTURE_LAYOUT_GENERAL;
		enviroSpecIBLOutputImageInfo.view = views["enviroSkyboxSpecIBL_fg_miplvl" + toString(m)];

		DescriptorWriteInfo enviroSpecIBLOutputWrite = {};
		enviroSpecIBLOutputWrite.descriptorCount = 1;
		enviroSpecIBLOutputWrite.descriptorType = DESCRIPTOR_TYPE_STORAGE_IMAGE;
		enviroSpecIBLOutputWrite.dstBinding = 0;
		enviroSpecIBLOutputWrite.dstSet = skyEnviroOutputMipDescSets[m - 1];
		enviroSpecIBLOutputWrite.imageInfo = {enviroSpecIBLOutputImageInfo};

		writes.push_back(enviroSpecIBLOutputWrite);
	}

	renderer->writeDescriptorSets(writes);

	enviroSkyboxInput = views["enviroSkybox"];
	enviroSkyboxSpecIBLOutput = views["enviroSkyboxSpecIBL"];
}

void SkyCubemapRenderer::enviroSkyboxSpecIBLGenPassRender(CommandBuffer cmdBuffer, uint32_t counter)
{
	cmdBuffer->setTextureLayout(enviroSkyboxSpecIBLOutput->parentTexture, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {0, 1, 0, 6}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_TRANSFER_BIT);
	cmdBuffer->setTextureLayout(enviroSkyboxInput->parentTexture, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 6}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_TRANSFER_BIT);

	TextureBlitInfo blitInfo = {};
	blitInfo.srcSubresource = {0, 0, 6};
	blitInfo.dstSubresource = {0, 0, 6};
	blitInfo.srcOffsets[0] = {0, 0, 0};
	blitInfo.dstOffsets[0] = {0, 0, 0};
	blitInfo.srcOffsets[1] = {int32_t(faceResolution), int32_t(faceResolution), 1};
	blitInfo.dstOffsets[1] = {int32_t(faceResolution), int32_t(faceResolution), 1};

	cmdBuffer->blitTexture(enviroSkyboxInput->parentTexture, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, enviroSkyboxSpecIBLOutput->parentTexture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {blitInfo}, SAMPLER_FILTER_NEAREST);

	uint32_t mipCount = getMipCount();

	cmdBuffer->setTextureLayout(enviroSkyboxInput->parentTexture, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 6}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
	cmdBuffer->setTextureLayout(enviroSkyboxSpecIBLOutput->parentTexture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 6}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
	cmdBuffer->setTextureLayout(enviroSkyboxSpecIBLOutput->parentTexture, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_GENERAL, {1, mipCount - 1, 0, 6}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);

	cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_COMPUTE, enviroFilterPipeline);
	cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_COMPUTE, 0, {skyEnviroDescriptorSet});

	for (uint32_t m = 1; m < mipCount; m++)
	{
		SkyEnvironmentMapPushConsts pushConsts = {(float(m) / float(mipCount - 1)), 1.0f / float(faceResolution >> m)};

		cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_COMPUTE, enviroFilterPipeline);
		cmdBuffer->pushConstants(SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SkyEnvironmentMapPushConsts), &pushConsts);
		cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_COMPUTE, 1, {skyEnviroOutputMipDescSets[m - 1]});
		cmdBuffer->dispatch((faceResolution >> m) / 8, (faceResolution >> m) / 8, 6 / 1);
	}

	cmdBuffer->setTextureLayout(enviroSkyboxSpecIBLOutput->parentTexture, TEXTURE_LAYOUT_GENERAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {1, mipCount - 1, 0, 6}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
}

void SkyCubemapRenderer::setSunDirection(glm::vec3 sunLightDir)
{
	sunDirection = sunLightDir;
}

uint32_t SkyCubemapRenderer::getFaceResolution()
{
	return faceResolution;
}

uint32_t SkyCubemapRenderer::getMaxEnviroSpecIBLMipCount()
{
	return maxEnviroSpecIBLMips;
}

Sampler SkyCubemapRenderer::getSkyEnviroCubemapSampler()
{
	return enviroCubemapSampler;
}

void SkyCubemapRenderer::createCubemapPipeline(RenderPass renderPass, uint32_t baseSubpass)
{
	const std::string insertMarker = "#SE_BUILTIN_INCLUDE_ATMOSPHERE_LIB";
	const std::string shaderSourceFile = "GameData/shaders/vulkan/sky-cubemap.glsl";

	std::string lightingSource = FileLoader::instance()->readFile(shaderSourceFile);
	size_t insertPos = lightingSource.find(insertMarker);

	if (insertPos != std::string::npos)
	{
		lightingSource.replace(insertPos, insertMarker.length(), atmosphere->getAtmosphericShaderLib());
	}
	else
	{
		printf("%s Didn't find location to insert atmopsheric shader lib into the deferred lighting shader\n", WARN_PREFIX);

		lightingSource.replace(insertPos, insertMarker.length(), "");
	}

	ShaderModule vertShader = renderer->createShaderModuleFromSource(lightingSource, shaderSourceFile, SHADER_STAGE_VERTEX_BIT, SHADER_LANGUAGE_GLSL);
	ShaderModule geomShader = renderer->createShaderModuleFromSource(lightingSource, shaderSourceFile, SHADER_STAGE_GEOMETRY_BIT, SHADER_LANGUAGE_GLSL);
	ShaderModule fragShader = renderer->createShaderModuleFromSource(lightingSource, shaderSourceFile, SHADER_STAGE_FRAGMENT_BIT, SHADER_LANGUAGE_GLSL);

	PipelineShaderStage vertShaderStage = {};
	vertShaderStage.entry = "main";
	vertShaderStage.module = vertShader;

	PipelineShaderStage geomShaderStage = {};
	geomShaderStage.entry = "main";
	geomShaderStage.module = geomShader;

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
	rastInfo.enableOutOfOrderRasterization = false;

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

	GraphicsPipelineInfo info = {};
	info.stages = {vertShaderStage, geomShaderStage, fragShaderStage};
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
		{3, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{4, DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, SHADER_STAGE_GEOMETRY_BIT}
	}};

	skyPipelines = renderer->createGraphicsPipeline(info, renderPass, baseSubpass);

	renderer->destroyShaderModule(vertShaderStage.module);
	renderer->destroyShaderModule(fragShaderStage.module);
}

void SkyCubemapRenderer::createEnvironmentMapPipeline()
{
	ShaderModule compShader = renderer->createShaderModule("GameData/shaders/sky-enviro-gen.hlsl", SHADER_STAGE_COMPUTE_BIT, SHADER_LANGUAGE_HLSL, "EnviroSkyboxSpecIBLGen");
	
	PipelineShaderStage compStage = {};
	compStage.entry = "EnviroSkyboxSpecIBLGen";
	compStage.module = compShader;

	ComputePipelineInfo pipelineInfo = {};
	pipelineInfo.shader = compStage;
	pipelineInfo.inputPushConstantRanges = {{0, sizeof(SkyEnvironmentMapPushConsts), SHADER_STAGE_COMPUTE_BIT}};
	pipelineInfo.inputSetLayouts = {{
		{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_COMPUTE_BIT},
		{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_COMPUTE_BIT},
		{2, DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, SHADER_STAGE_COMPUTE_BIT}
	},{
		{0, DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, SHADER_STAGE_COMPUTE_BIT}
	}};

	enviroFilterPipeline = renderer->createComputePipeline(pipelineInfo);

	renderer->destroyShaderModule(compShader);
}

/*
void SkyCubemapRenderer::createEnvironmentMapPipeline(RenderPass renderPass, uint32_t baseSubpass)
{
	ShaderModule vertShader = renderer->createShaderModule("GameData/shaders/vulkan/sky-enviro.glsl", SHADER_STAGE_VERTEX_BIT, SHADER_LANGUAGE_GLSL);
	ShaderModule fragShader = renderer->createShaderModule("GameData/shaders/vulkan/sky-enviro.glsl", SHADER_STAGE_FRAGMENT_BIT, SHADER_LANGUAGE_GLSL);

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
	rastInfo.enableOutOfOrderRasterization = false;

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

	GraphicsPipelineInfo info = {};
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

	for (uint32_t m = 0; m < getMipCount(); m++)
		enviroFilterPipelines.push_back(renderer->createGraphicsPipeline(info, renderPass, baseSubpass + m));

	renderer->destroyShaderModule(vertShaderStage.module);
	renderer->destroyShaderModule(fragShaderStage.module);
}
*/
uint32_t SkyCubemapRenderer::getMipCount()
{
	return glm::min<uint32_t>((uint32_t) glm::floor(glm::log2((float) faceResolution)) + 1, maxEnviroSpecIBLMips);
}