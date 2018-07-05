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
 * PostProcess.cpp
 *
 *  Created on: Apr 21, 2018
 *      Author: David
 */

#include "Rendering/PostProcess/PostProcess.h"

#include <Engine/StarlightEngine.h>

#include <Rendering/Renderer/Renderer.h>

PostProcess::PostProcess (StarlightEngine *enginePtr)
{
	engine = enginePtr;
	destroyed = false;

	postprocessRenderPass = nullptr;
	postprocessFramebuffer = nullptr;

	combinePipeline = nullptr;

	gbuffer_AlbedoRoughness = nullptr;
	gbuffer_NormalsMetalness = nullptr;
	gbuffer_Depth = nullptr;
	deferredLightingOutput = nullptr;

	postprocessOutputTexture = nullptr;
	postprocessOutputTextureView = nullptr;

	combineDescriptorPool = nullptr;
	combineDescriptorSet = nullptr;

	inputSampler = nullptr;

	postprocessCommandPool = nullptr;

	cmdBufferIndex = 0;
}

PostProcess::~PostProcess ()
{
	if (!destroyed)
		destroy();
}

void PostProcess::renderPostProcessing (Semaphore deferredLightingOutputSemaphore)
{
	std::vector<ClearValue> clearValues = std::vector<ClearValue>(1);
	clearValues[0].color =
	{	0, 0, 0, 0};

	cmdBufferIndex ++;
	cmdBufferIndex %= finalCombineCmdBuffers.size();

	finalCombineCmdBuffers[cmdBufferIndex]->beginCommands(COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	finalCombineCmdBuffers[cmdBufferIndex]->beginDebugRegion("Post Processing", glm::vec4(1.0f, 0.5f, 1.0f, 1.0f));
	finalCombineCmdBuffers[cmdBufferIndex]->beginRenderPass(postprocessRenderPass, postprocessFramebuffer, {0, 0, (uint32_t) gbufferSize.x, (uint32_t) gbufferSize.y}, clearValues, SUBPASS_CONTENTS_INLINE);
	finalCombineCmdBuffers[cmdBufferIndex]->setScissors(0, {{0, 0, (uint32_t) gbufferSize.x, (uint32_t) gbufferSize.y}});
	finalCombineCmdBuffers[cmdBufferIndex]->setViewports(0, {{0, 0, gbufferSize.x, gbufferSize.y, 0.0f, 1.0f}});

	finalCombineCmdBuffers[cmdBufferIndex]->beginDebugRegion("Final/Combine", glm::vec4(0.8f, 0, 0, 1.0f));
	finalCombineCmdBuffers[cmdBufferIndex]->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, combinePipeline);
	finalCombineCmdBuffers[cmdBufferIndex]->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {combineDescriptorSet});

	// Vertex positions contained in the shader as constants
	finalCombineCmdBuffers[cmdBufferIndex]->draw(6);
	finalCombineCmdBuffers[cmdBufferIndex]->endDebugRegion();

	finalCombineCmdBuffers[cmdBufferIndex]->endRenderPass();
	finalCombineCmdBuffers[cmdBufferIndex]->endDebugRegion();
	finalCombineCmdBuffers[cmdBufferIndex]->endCommands();
	std::vector<Semaphore> waitSems = {deferredLightingOutputSemaphore};
	std::vector<PipelineStageFlags> waitSemStages = {PIPELINE_STAGE_FRAGMENT_SHADER_BIT};

	engine->renderer->submitToQueue(QUEUE_TYPE_GRAPHICS, {finalCombineCmdBuffers[cmdBufferIndex]}, waitSems, waitSemStages);
}

void PostProcess::init ()
{
	createPostProcessRenderPass();
	createCombinePipeline();

	combineDescriptorPool = engine->renderer->createDescriptorPool({{
		{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT}
	}}, 1);

	combineDescriptorSet = combineDescriptorPool->allocateDescriptorSet();
	inputSampler = engine->renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SAMPLER_FILTER_NEAREST, SAMPLER_FILTER_NEAREST, 1);

	postprocessCommandPool = engine->renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, COMMAND_POOL_RESET_COMMAND_BUFFER_BIT);
	finalCombineCmdBuffers = postprocessCommandPool->allocateCommandBuffers(COMMAND_BUFFER_LEVEL_PRIMARY, 3);

	finalCombineSemaphores = engine->renderer->createSemaphores(3);
}

void PostProcess::destroy ()
{
	destroyed = true;

	engine->renderer->destroyDescriptorPool(combineDescriptorPool);

	for (size_t i = 0; i < finalCombineSemaphores.size(); i ++)
		engine->renderer->destroySemaphore(finalCombineSemaphores[i]);

	engine->renderer->destroyFramebuffer(postprocessFramebuffer);

	engine->renderer->destroyPipeline(combinePipeline);
	engine->renderer->destroyRenderPass(postprocessRenderPass);

	engine->renderer->destroyTextureView(postprocessOutputTextureView);
	engine->renderer->destroyTexture(postprocessOutputTexture);

	engine->renderer->destroySampler(inputSampler);

	engine->renderer->destroyCommandPool(postprocessCommandPool);
}

void PostProcess::setInputs (TextureView gbuffer_AlbedoRoughnessTV, TextureView gbuffer_NormalsMetalnessTV, TextureView gbuffer_DepthTV, TextureView deferredLightingOutputTV, suvec2 gbufferSize)
{
	if (postprocessOutputTexture != nullptr)
		engine->renderer->destroyTexture(postprocessOutputTexture);

	if (postprocessOutputTextureView != nullptr)
		engine->renderer->destroyTextureView(postprocessOutputTextureView);

	if (postprocessFramebuffer != nullptr)
		engine->renderer->destroyFramebuffer(postprocessFramebuffer);

	gbuffer_AlbedoRoughness = gbuffer_AlbedoRoughnessTV;
	gbuffer_NormalsMetalness = gbuffer_NormalsMetalnessTV;
	gbuffer_Depth = gbuffer_DepthTV;
	deferredLightingOutput = deferredLightingOutputTV;
	this->gbufferSize = {(float) gbufferSize.x, (float) gbufferSize.y};

	postprocessOutputTexture = engine->renderer->createTexture({(float) gbufferSize.x, (float) gbufferSize.y, 1}, RESOURCE_FORMAT_R8G8B8A8_UNORM,
			TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_COLOR_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true);
	postprocessOutputTextureView = engine->renderer->createTextureView(postprocessOutputTexture);

	postprocessFramebuffer = engine->renderer->createFramebuffer(postprocessRenderPass, {postprocessOutputTextureView}, gbufferSize.x, gbufferSize.y);

	DescriptorImageInfo inputSamplerInfo = {};
	inputSamplerInfo.sampler = inputSampler;

	DescriptorImageInfo deferredOutputInfo = {};
	deferredOutputInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	deferredOutputInfo.view = deferredLightingOutput;

	DescriptorWriteInfo inputSamplerWrite = {}, deferredOutputWrite = {};

	inputSamplerWrite.descriptorCount = 1;
	inputSamplerWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLER;
	inputSamplerWrite.dstBinding = 0;
	inputSamplerWrite.dstSet = combineDescriptorSet;
	inputSamplerWrite.imageInfo =
	{	inputSamplerInfo};

	deferredOutputWrite.descriptorCount = 1;
	deferredOutputWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	deferredOutputWrite.dstBinding = 1;
	deferredOutputWrite.dstSet = combineDescriptorSet;
	deferredOutputWrite.imageInfo =
	{	deferredOutputInfo};

	engine->renderer->writeDescriptorSets({inputSamplerWrite, deferredOutputWrite});

	engine->renderer->setObjectDebugName(postprocessOutputTexture, OBJECT_TYPE_TEXTURE, "Postprocess Output");
}

void PostProcess::createPostProcessRenderPass ()
{
	AttachmentDescription postprocessOutputDesc = {};
	postprocessOutputDesc.format = RESOURCE_FORMAT_R8G8B8A8_UNORM;
	postprocessOutputDesc.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
	postprocessOutputDesc.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	postprocessOutputDesc.loadOp = ATTACHMENT_LOAD_OP_DONT_CARE;
	postprocessOutputDesc.storeOp = ATTACHMENT_STORE_OP_STORE;

	AttachmentReference finalsubpass_postprocessOutputRef = {};
	finalsubpass_postprocessOutputRef.attachment = 0;
	finalsubpass_postprocessOutputRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	SubpassDescription finalsubpass = {};
	finalsubpass.bindPoint = PIPELINE_BIND_POINT_GRAPHICS;
	finalsubpass.colorAttachments = {finalsubpass_postprocessOutputRef};
	finalsubpass.depthStencilAttachment = nullptr;

	postprocessRenderPass = engine->renderer->createRenderPass({postprocessOutputDesc}, {finalsubpass}, {});
}

void PostProcess::createCombinePipeline ()
{
	ShaderModule vertShader = engine->renderer->createShaderModule("GameData/shaders/vulkan/combine.glsl", SHADER_STAGE_VERTEX_BIT);
	ShaderModule fragShader = engine->renderer->createShaderModule("GameData/shaders/vulkan/combine.glsl", SHADER_STAGE_FRAGMENT_BIT);

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

	info.inputPushConstantRanges = {};
	info.inputSetLayouts = {{
			{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
			{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT}
	}};

	combinePipeline = engine->renderer->createGraphicsPipeline(info, postprocessRenderPass, 0);

	engine->renderer->destroyShaderModule(vertShaderStage.module);
	engine->renderer->destroyShaderModule(fragShaderStage.module);
}
