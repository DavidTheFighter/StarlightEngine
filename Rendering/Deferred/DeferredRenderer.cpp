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

#include <Rendering/Renderer/Renderer.h>

DeferredRenderer::DeferredRenderer (StarlightEngine *enginePtr)
{
	engine = enginePtr;
	gbuffer_AlbedoRoughnessView = nullptr;
	gbuffer_NormalsMetalnessView = nullptr;
	gbufferDirty = false;
	destroyed = false;

	deferredRenderPass = nullptr;
	deferredPipeline = nullptr;
	deferredPipelineInputLayout = nullptr;
	deferredFramebuffer = nullptr;

	deferredCommandPool = nullptr;
	deferredCommandBuffer = nullptr;

	deferredOutput = nullptr;
	deferredOutputView = nullptr;
	deferredInputsSampler = nullptr;

	deferredInputsDescriptorPool = nullptr;
	deferredInputDescriptorSet = nullptr;
}

DeferredRenderer::~DeferredRenderer ()
{
	if (!destroyed)
		destroy();
}

void DeferredRenderer::renderDeferredLighting ()
{
	std::vector<ClearValue> clearValues = std::vector<ClearValue>(1);
	clearValues[0].color =
	{	0, 0, 0, 0};

	CommandBuffer cmdBuffer = deferredCommandPool->allocateCommandBuffer(COMMAND_BUFFER_LEVEL_PRIMARY);
	cmdBuffer->beginCommands(COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	cmdBuffer->beginRenderPass(deferredRenderPass, deferredFramebuffer, {0, 0, (uint32_t) gbufferSize.x, (uint32_t) gbufferSize.y}, clearValues, SUBPASS_CONTENTS_INLINE);
	cmdBuffer->setScissors(0, {{0, 0, (uint32_t) gbufferSize.x, (uint32_t) gbufferSize.y}});
	cmdBuffer->setViewports(0, {{0, 0, gbufferSize.x, gbufferSize.y, 0.0f, 1.0f}});
	cmdBuffer->beginDebugRegion("Deferred Lighting", glm::vec4(0.9f, 0.85f, 0.1f, 1.0f));

	cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, deferredPipeline);
	cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, deferredPipelineInputLayout, 0, {deferredInputDescriptorSet});

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
	createDeferredLightingRenderPass();
	createDeferredLightingPipeline();

	deferredCommandPool = engine->renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, 0);
	deferredInputsSampler = engine->renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SAMPLER_FILTER_NEAREST, SAMPLER_FILTER_NEAREST, 1);

	deferredInputsDescriptorPool = engine->renderer->createDescriptorPool({{
		{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{2, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT}
	}}, 1);

	deferredInputDescriptorSet = deferredInputsDescriptorPool->allocateDescriptorSet();

	DescriptorImageInfo deferredInputSamplerImageInfo = {};
	deferredInputSamplerImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	deferredInputSamplerImageInfo.sampler = deferredInputsSampler;

	DescriptorWriteInfo swrite = {};
	swrite.descriptorCount = 1;
	swrite.descriptorType = DESCRIPTOR_TYPE_SAMPLER;
	swrite.dstBinding = 0;
	swrite.dstSet = deferredInputDescriptorSet;
	swrite.imageInfo =
	{	deferredInputSamplerImageInfo};

	engine->renderer->writeDescriptorSets({swrite});
}

void DeferredRenderer::setGBuffer (TextureView gbuffer_AlbedoRoughnessView, TextureView gbuffer_NormalsMetalnessView, suvec2 gbufferDim)
{
	gbufferDirty = true;

	this->gbufferSize = {(float) gbufferDim.x, (float) gbufferDim.y};
	this->gbuffer_AlbedoRoughnessView = gbuffer_AlbedoRoughnessView;
	this->gbuffer_NormalsMetalnessView = gbuffer_NormalsMetalnessView;

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

	DescriptorWriteInfo garWrite = {}, gnmWrite = {};
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

	engine->renderer->writeDescriptorSets({garWrite, gnmWrite});

	engine->renderer->setObjectDebugName(deferredOutput, OBJECT_TYPE_TEXTURE, "Deferred Output");
}

void DeferredRenderer::destroy ()
{
	destroyed = true;

	engine->renderer->destroyDescriptorPool(deferredInputsDescriptorPool);
	engine->renderer->destroySampler(deferredInputsSampler);
	engine->renderer->destroyTexture(deferredOutput);
	engine->renderer->destroyTextureView(deferredOutputView);

	engine->renderer->destroyFramebuffer(deferredFramebuffer);
	engine->renderer->destroyCommandPool(deferredCommandPool);

	engine->renderer->destroyPipelineInputLayout(deferredPipelineInputLayout);
	engine->renderer->destroyPipeline(deferredPipeline);
	engine->renderer->destroyRenderPass(deferredRenderPass);
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
	deferredPipelineInputLayout = engine->renderer->createPipelineInputLayout({}, {{
			{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
			{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
			{2, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT}
	}});

	ShaderModule vertShader = engine->renderer->createShaderModule(engine->getWorkingDir() + "GameData/shaders/vulkan/lighting.glsl", SHADER_STAGE_VERTEX_BIT);
	ShaderModule fragShader = engine->renderer->createShaderModule(engine->getWorkingDir() + "GameData/shaders/vulkan/lighting.glsl", SHADER_STAGE_FRAGMENT_BIT);

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

	deferredPipeline = engine->renderer->createGraphicsPipeline(info, deferredPipelineInputLayout, deferredRenderPass, 0);

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
