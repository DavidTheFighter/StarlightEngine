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
 * DeferredRenderer.cpp
 * 
 * Created on: Mar 24, 2018
 *     Author: david
 */

#include "Rendering/Deferred/DeferredRenderer.h"

#include <Engine/StarlightEngine.h>

#include <Game/Game.h>

#include <Rendering/Renderer/Renderer.h>
#include <Rendering/World/WorldRenderer.h>
#include <Rendering/Deferred/AtmosphereRenderer.h>

DeferredRenderer::DeferredRenderer (StarlightEngine *enginePtr, WorldRenderer *worldRendererPtr)
{
	engine = enginePtr;
	worldRenderer = worldRendererPtr;
	atmosphere = nullptr;

	gbuffer_AlbedoRoughnessView = nullptr;
	gbuffer_NormalsMetalnessView = nullptr;
	gbuffer_DepthView = nullptr;
	gbufferDirty = false;
	destroyed = false;

	deferredRenderPass = nullptr;
	deferredPipeline = nullptr;
	deferredFramebuffer = nullptr;

	deferredCommandPool = nullptr;
	deferredCommandBuffer = nullptr;

	deferredOutput = nullptr;
	deferredOutputView = nullptr;
	deferredInputsSampler = nullptr;
	atmosphereTextureSampler = nullptr;

	deferredInputsDescriptorPool = nullptr;
	deferredInputDescriptorSet = nullptr;
	game = nullptr;
}

DeferredRenderer::~DeferredRenderer ()
{
	if (!destroyed)
		destroy();
}

uint32_t lightingPcSize = (uint32_t) (sizeof(glm::mat4) + sizeof(glm::vec4) + sizeof(glm::vec4) + sizeof(glm::vec2));

void DeferredRenderer::renderDeferredLighting ()
{
	std::vector<ClearValue> clearValues = std::vector<ClearValue>(1);
	clearValues[0].color =
	{	0, 0, 0, 0};

	char pushConstData[lightingPcSize];
	size_t seqOffset = 0;
	memset(pushConstData, 0, sizeof(pushConstData));

	glm::vec3 playerLookDir = glm::normalize(glm::vec3(cos(game->mainCamera.lookAngles.y) * sin(game->mainCamera.lookAngles.x), sin(game->mainCamera.lookAngles.y), cos(game->mainCamera.lookAngles.y) * cos(game->mainCamera.lookAngles.x)));
	glm::vec2 prjMat = glm::vec2(worldRenderer->camProjMat[2][2], worldRenderer->camProjMat[3][2]);

	seqmemcpy(pushConstData, &invCamMVPMat[0][0], sizeof(glm::mat4), seqOffset);
	seqmemcpy(pushConstData, &game->mainCamera.position.x, sizeof(glm::vec4), seqOffset);
	seqmemcpy(pushConstData, &playerLookDir.x, sizeof(glm::vec4), seqOffset);
	seqmemcpy(pushConstData, &prjMat, sizeof(glm::vec2), seqOffset);

	CommandBuffer cmdBuffer = deferredCommandPool->allocateCommandBuffer(COMMAND_BUFFER_LEVEL_PRIMARY);
	cmdBuffer->beginCommands(COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	cmdBuffer->beginRenderPass(deferredRenderPass, deferredFramebuffer, {0, 0, (uint32_t) gbufferSize.x, (uint32_t) gbufferSize.y}, clearValues, SUBPASS_CONTENTS_INLINE);
	cmdBuffer->setScissors(0, {{0, 0, (uint32_t) gbufferSize.x, (uint32_t) gbufferSize.y}});
	cmdBuffer->setViewports(0, {{0, 0, gbufferSize.x, gbufferSize.y, 0.0f, 1.0f}});
	cmdBuffer->beginDebugRegion("Deferred Lighting", glm::vec4(0.9f, 0.85f, 0.1f, 1.0f));

	cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, deferredPipeline);
	cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {deferredInputDescriptorSet});
	cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_FRAGMENT_BIT, 0, lightingPcSize, pushConstData);

	// Vertex positions contained in the shader as constants
	cmdBuffer->draw(6);

	cmdBuffer->endDebugRegion();
	cmdBuffer->endRenderPass();
	cmdBuffer->endCommands();
	std::vector<Semaphore> waitSems = {};
	std::vector<PipelineStageFlags> waitSemStages = {};

	engine->renderer->submitToQueue(QUEUE_TYPE_GRAPHICS, {cmdBuffer}, waitSems, waitSemStages);
	engine->renderer->waitForQueueIdle(QUEUE_TYPE_GRAPHICS);
	deferredCommandPool->freeCommandBuffer(cmdBuffer);
}

void DeferredRenderer::init ()
{
	atmosphere = new AtmosphereRenderer (engine);
	atmosphere->init();

	createDeferredLightingRenderPass();
	createDeferredLightingPipeline();

	deferredCommandPool = engine->renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, 0);
	deferredInputsSampler = engine->renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SAMPLER_FILTER_NEAREST, SAMPLER_FILTER_NEAREST);
	atmosphereTextureSampler = engine->renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SAMPLER_FILTER_LINEAR, SAMPLER_FILTER_LINEAR);

	deferredInputsDescriptorPool = engine->renderer->createDescriptorPool({{
		{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{2, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{3, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{4, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{5, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{6, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{7, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT}
	}}, 1);

	deferredInputDescriptorSet = deferredInputsDescriptorPool->allocateDescriptorSet();

	DescriptorImageInfo deferredInputSamplerImageInfo = {};
	deferredInputSamplerImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	deferredInputSamplerImageInfo.sampler = deferredInputsSampler;

	DescriptorImageInfo transmittanceImageInfo = {};
	transmittanceImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	transmittanceImageInfo.sampler = atmosphereTextureSampler;
	transmittanceImageInfo.view = atmosphere->transmittanceTV;

	DescriptorImageInfo scatteringImageInfo = {};
	scatteringImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	scatteringImageInfo.sampler = atmosphereTextureSampler;
	scatteringImageInfo.view = atmosphere->scatteringTV;

	// Need to upload something or the validation layer will have a coronary, even if we don't use this texture
	DescriptorImageInfo singleMieScatteringImageInfo = {};
	singleMieScatteringImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	singleMieScatteringImageInfo.sampler = atmosphereTextureSampler;
	singleMieScatteringImageInfo.view = atmosphere->scatteringTV;

	DescriptorImageInfo irradianceImageInfo = {};
	irradianceImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	irradianceImageInfo.sampler = atmosphereTextureSampler;
	irradianceImageInfo.view = atmosphere->irradianceTV;

	DescriptorWriteInfo swrite = {}, twrite = {}, stwrite = {}, smswrite = {}, iwrite = {};
	swrite.descriptorCount = 1;
	swrite.descriptorType = DESCRIPTOR_TYPE_SAMPLER;
	swrite.dstBinding = 0;
	swrite.dstSet = deferredInputDescriptorSet;
	swrite.imageInfo =
	{	deferredInputSamplerImageInfo};

	twrite.descriptorCount = 1;
	twrite.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	twrite.dstBinding = 4;
	twrite.dstSet = deferredInputDescriptorSet;
	twrite.imageInfo =
	{	transmittanceImageInfo};

	stwrite.descriptorCount = 1;
	stwrite.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	stwrite.dstBinding = 5;
	stwrite.dstSet = deferredInputDescriptorSet;
	stwrite.imageInfo =
	{	scatteringImageInfo};

	smswrite.descriptorCount = 1;
	smswrite.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	smswrite.dstBinding = 6;
	smswrite.dstSet = deferredInputDescriptorSet;
	smswrite.imageInfo =
	{	singleMieScatteringImageInfo};

	iwrite.descriptorCount = 1;
	iwrite.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	iwrite.dstBinding = 7;
	iwrite.dstSet = deferredInputDescriptorSet;
	iwrite.imageInfo =
	{	irradianceImageInfo};

	engine->renderer->writeDescriptorSets({swrite, twrite, stwrite, smswrite, iwrite});
}

void DeferredRenderer::setGBuffer (TextureView gbuffer_AlbedoRoughnessView, TextureView gbuffer_NormalsMetalnessView, TextureView gbuffer_DepthView, suvec2 gbufferDim)
{
	gbufferDirty = true;

	this->gbufferSize = {(float) gbufferDim.x, (float) gbufferDim.y};
	this->gbuffer_AlbedoRoughnessView = gbuffer_AlbedoRoughnessView;
	this->gbuffer_NormalsMetalnessView = gbuffer_NormalsMetalnessView;
	this->gbuffer_DepthView = gbuffer_DepthView;

	if (deferredOutput != nullptr)
		engine->renderer->destroyTexture(deferredOutput);
	if (deferredOutputView != nullptr)
		engine->renderer->destroyTextureView(deferredOutputView);
	if (deferredFramebuffer != nullptr)
		engine->renderer->destroyFramebuffer(deferredFramebuffer);

	deferredOutput = engine->renderer->createTexture({(float) gbufferDim.x, (float) gbufferDim.y, 1}, RESOURCE_FORMAT_R16G16B16A16_SFLOAT, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_COLOR_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true);
	deferredOutputView = engine->renderer->createTextureView(deferredOutput, TEXTURE_VIEW_TYPE_2D);
	deferredFramebuffer = engine->renderer->createFramebuffer(deferredRenderPass, {deferredOutputView}, gbufferDim.x, gbufferDim.y, 1);

	DescriptorImageInfo gbufferRoughnessAlbedoImageInfo = {};
	gbufferRoughnessAlbedoImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	gbufferRoughnessAlbedoImageInfo.view = gbuffer_AlbedoRoughnessView;

	DescriptorImageInfo gbufferNormalsMetalnessImageInfo = {};
	gbufferNormalsMetalnessImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	gbufferNormalsMetalnessImageInfo.view = gbuffer_NormalsMetalnessView;

	DescriptorImageInfo gbufferDepthImageInfo = {};
	gbufferDepthImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	gbufferDepthImageInfo.view = gbuffer_DepthView;

	DescriptorWriteInfo garWrite = {}, gnmWrite = {}, dwrite = {};
	garWrite.descriptorCount = 1;
	garWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	garWrite.dstBinding = 1;
	garWrite.dstSet = deferredInputDescriptorSet;
	garWrite.imageInfo =
	{	gbufferRoughnessAlbedoImageInfo};

	gnmWrite.descriptorCount = 1;
	gnmWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	gnmWrite.dstBinding = 2;
	gnmWrite.dstSet = deferredInputDescriptorSet;
	gnmWrite.imageInfo =
	{	gbufferNormalsMetalnessImageInfo};

	dwrite.descriptorCount = 1;
	dwrite.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	dwrite.dstBinding = 3;
	dwrite.dstSet = deferredInputDescriptorSet;
	dwrite.imageInfo =
	{	gbufferDepthImageInfo};

	engine->renderer->writeDescriptorSets({garWrite, gnmWrite, dwrite});

	engine->renderer->setObjectDebugName(deferredOutput, OBJECT_TYPE_TEXTURE, "Deferred Output");
}

void DeferredRenderer::destroy ()
{
	destroyed = true;

	engine->renderer->destroyDescriptorPool(deferredInputsDescriptorPool);
	engine->renderer->destroySampler(deferredInputsSampler);
	engine->renderer->destroySampler(atmosphereTextureSampler);
	engine->renderer->destroyTexture(deferredOutput);
	engine->renderer->destroyTextureView(deferredOutputView);

	engine->renderer->destroyFramebuffer(deferredFramebuffer);
	engine->renderer->destroyCommandPool(deferredCommandPool);

	engine->renderer->destroyPipeline(deferredPipeline);
	engine->renderer->destroyRenderPass(deferredRenderPass);

	atmosphere->destroy();

	delete atmosphere;
}

void DeferredRenderer::createDeferredLightingRenderPass ()
{
	AttachmentDescription deferredOutputAttachment = {};
	deferredOutputAttachment.format = RESOURCE_FORMAT_R16G16B16A16_SFLOAT;
	deferredOutputAttachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
	deferredOutputAttachment.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	deferredOutputAttachment.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
	deferredOutputAttachment.storeOp = ATTACHMENT_STORE_OP_STORE;

	AttachmentReference subpass0_deferredOutputRef = {};
	subpass0_deferredOutputRef.attachment = 0;
	subpass0_deferredOutputRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	SubpassDescription subpass0 = {};
	subpass0.bindPoint = PIPELINE_BIND_POINT_GRAPHICS;
	subpass0.colorAttachments = {subpass0_deferredOutputRef};
	subpass0.depthStencilAttachment = nullptr;

	deferredRenderPass = engine->renderer->createRenderPass({deferredOutputAttachment}, {subpass0}, {});
}

void DeferredRenderer::createDeferredLightingPipeline ()
{
	const std::string insertMarker = "#SE_BUILTIN_INCLUDE_ATMOSPHERE_LIB";
	const std::string shaderSourceFile = engine->getWorkingDir() + "GameData/shaders/vulkan/lighting.glsl";

	std::string lightingSource = readFileStr(shaderSourceFile);
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

	ShaderModule vertShader = engine->renderer->createShaderModuleFromSource(lightingSource, shaderSourceFile, SHADER_STAGE_VERTEX_BIT);
	ShaderModule fragShader = engine->renderer->createShaderModuleFromSource(lightingSource, shaderSourceFile, SHADER_STAGE_FRAGMENT_BIT);

	PipelineShaderStage vertShaderStage = {};
	vertShaderStage.entry = "main";
	vertShaderStage.module = vertShader;

	PipelineShaderStage fragShaderStage = {};
	fragShaderStage.entry = "main";
	fragShaderStage.module = fragShader;

	PipelineVertexInputInfo vertexInput = {};
	vertexInput.vertexInputAttribs =
	{};
	vertexInput.vertexInputBindings =
	{};

	PipelineInputAssemblyInfo inputAssembly = {};
	inputAssembly.primitiveRestart = false;
	inputAssembly.topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	PipelineViewportInfo viewportInfo = {};
	viewportInfo.scissors =
	{
		{	0, 0, 1920, 1080}};
	viewportInfo.viewports =
	{
		{	0, 0, 1920, 1080}};

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
	colorBlend.attachments =
	{	colorBlendAttachment};
	colorBlend.logicOpEnable = false;
	colorBlend.logicOp = LOGIC_OP_COPY;
	colorBlend.blendConstants[0] = 1.0f;
	colorBlend.blendConstants[1] = 1.0f;
	colorBlend.blendConstants[2] = 1.0f;
	colorBlend.blendConstants[3] = 1.0f;

	PipelineDynamicStateInfo dynamicState = {};
	dynamicState.dynamicStates =
	{	DYNAMIC_STATE_VIEWPORT, DYNAMIC_STATE_SCISSOR};

	PipelineInfo info = {};
	info.stages =
	{	vertShaderStage, fragShaderStage};
	info.vertexInputInfo = vertexInput;
	info.inputAssemblyInfo = inputAssembly;
	info.viewportInfo = viewportInfo;
	info.rasterizationInfo = rastInfo;
	info.depthStencilInfo = depthInfo;
	info.colorBlendInfo = colorBlend;
	info.dynamicStateInfo = dynamicState;

	info.inputPushConstantRanges = {{0, lightingPcSize, SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_FRAGMENT_BIT}};
	info.inputSetLayouts = {{
			{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
			{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
			{2, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
			{3, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
			{4, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
			{5, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
			{6, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
			{7, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT}
	}};

	deferredPipeline = engine->renderer->createGraphicsPipeline(info, deferredRenderPass, 0);

	engine->renderer->destroyShaderModule(vertShaderStage.module);
	engine->renderer->destroyShaderModule(fragShaderStage.module);
}

TextureView DeferredRenderer::getGBuffer_AlbedoRoughness ()
{
	return gbuffer_AlbedoRoughnessView;
}

TextureView DeferredRenderer::getGBuffer_NormalsMetalness ()
{
	return gbuffer_NormalsMetalnessView;
}
