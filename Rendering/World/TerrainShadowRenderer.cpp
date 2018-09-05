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
* TerrainShadowRenderer.cpp
*
* Created on: Jun 22, 2018
*     Author: david
*/

#include "TerrainShadowRenderer.h"

#include <Rendering/Renderer/Renderer.h>

TerrainShadowRenderer::TerrainShadowRenderer(Renderer *rendererPtr)
{
	renderer = rendererPtr;

	destroyed = false;

	shadowTerrainMeshVertexCount = 0;
}

TerrainShadowRenderer::~TerrainShadowRenderer()
{
	if (!destroyed)
		destroy();
}

typedef struct
{
	glm::mat4 mvp;
	float clipmapArrayLayer;
} TerrainShadowRendererPushConsts;

void TerrainShadowRenderer::renderTerrainShadowmap(CommandBuffer &cmdBuffer, glm::vec3 lightDir)
{
	glm::mat4 proj = glm::ortho<float>(-250, 1250, -512, 512, 1250, -250);
	proj[1][1] *= -1;
	glm::mat4 view = glm::lookAt(-lightDir, glm::vec3(0), glm::vec3(0, 1, 0));
	glm::mat4 mvp = proj * view;

	std::vector<ClearValue> clearValues = std::vector<ClearValue>(1);
	clearValues[0].depthStencil = {1, 0};

	cmdBuffer->beginDebugRegion("Terrain Shadows", glm::vec4(0.5f, 0.5f, 0.5f, 1));
	
	for (int i = 0; i < 5; i++)
	{
		TerrainShadowRendererPushConsts pushConsts = {mvp, (float) i};

		cmdBuffer->beginRenderPass(depthRenderPass, terrainDepthsFB[i], {0, 0, 2048, 2048}, clearValues, SUBPASS_CONTENTS_INLINE);
		cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, depthPipeline);
		cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT, 0, sizeof(TerrainShadowRendererPushConsts), &pushConsts);
		cmdBuffer->bindVertexBuffers(0, {shadowTerrainMesh}, {0});
		cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {depthClipmapDescSet});

		cmdBuffer->draw(shadowTerrainMeshVertexCount);

		cmdBuffer->endRenderPass();
	}

	cmdBuffer->endDebugRegion();
}

void TerrainShadowRenderer::init()
{
	terrainShadowCmdPool = renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, COMMAND_POOL_RESET_COMMAND_BUFFER_BIT);

	terrainDepths = renderer->createTexture({2048, 2048, 1}, RESOURCE_FORMAT_D16_UNORM, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true, 1, 5);
	terrainDepthsView = renderer->createTextureView(terrainDepths, TEXTURE_VIEW_TYPE_2D_ARRAY, {0, 1, 0, 5});
	terrainDepthSlicesView[0] = renderer->createTextureView(terrainDepths, TEXTURE_VIEW_TYPE_2D, {0, 1, 0, 1});
	terrainDepthSlicesView[1] = renderer->createTextureView(terrainDepths, TEXTURE_VIEW_TYPE_2D, {0, 1, 1, 1});
	terrainDepthSlicesView[2] = renderer->createTextureView(terrainDepths, TEXTURE_VIEW_TYPE_2D, {0, 1, 2, 1});
	terrainDepthSlicesView[3] = renderer->createTextureView(terrainDepths, TEXTURE_VIEW_TYPE_2D, {0, 1, 3, 1});
	terrainDepthSlicesView[4] = renderer->createTextureView(terrainDepths, TEXTURE_VIEW_TYPE_2D, {0, 1, 4, 1});


	depthmapSampler = renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	depthClipmapDescriptorPool = renderer->createDescriptorPool({{
		{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_VERTEX_BIT},
		{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_VERTEX_BIT}
		}}, 1);

	depthClipmapDescSet = depthClipmapDescriptorPool->allocateDescriptorSet();

	createTerrainMesh();

	createDepthRenderPass();
	createDepthPipeline();

	for (int i = 0; i < 5; i++)
		terrainDepthsFB[i] = renderer->createFramebuffer(depthRenderPass, {terrainDepthSlicesView[i]}, 2048, 2048);
}

void TerrainShadowRenderer::destroy()
{
	destroyed = true;

	renderer->destroyCommandPool(terrainShadowCmdPool);

	renderer->destroyBuffer(shadowTerrainMesh);

	renderer->destroyTexture(terrainDepths);
	renderer->destroyTextureView(terrainDepthsView);
	
	for (int i = 0; i < 5; i++)
		renderer->destroyTextureView(terrainDepthSlicesView[i]);

	for (int i = 0; i < 5; i++)
		renderer->destroyFramebuffer(terrainDepthsFB[i]);

	renderer->destroySampler(depthmapSampler);

	renderer->destroyDescriptorPool(depthClipmapDescriptorPool);

	renderer->destroyPipeline(depthPipeline);

	renderer->destroyRenderPass(depthRenderPass);
}

void TerrainShadowRenderer::setClipmap(TextureView clipmap, Sampler sampler)
{
	DescriptorImageInfo clipmapImageInfo = {};
	clipmapImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	clipmapImageInfo.sampler = sampler;
	clipmapImageInfo.view = clipmap;

	DescriptorWriteInfo clipmapSamplerWrite_depth = {}, clipmapTextureWrite_depth = {};
	clipmapSamplerWrite_depth.descriptorCount = 1;
	clipmapSamplerWrite_depth.descriptorType = DESCRIPTOR_TYPE_SAMPLER;
	clipmapSamplerWrite_depth.dstBinding = 0;
	clipmapSamplerWrite_depth.dstSet = depthClipmapDescSet;
	clipmapSamplerWrite_depth.imageInfo = {clipmapImageInfo};

	clipmapTextureWrite_depth.descriptorCount = 1;
	clipmapTextureWrite_depth.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	clipmapTextureWrite_depth.dstBinding = 1;
	clipmapTextureWrite_depth.dstSet = depthClipmapDescSet;
	clipmapTextureWrite_depth.imageInfo = {clipmapImageInfo};

	//

	renderer->writeDescriptorSets({clipmapSamplerWrite_depth, clipmapTextureWrite_depth});
}

void TerrainShadowRenderer::createTerrainMesh()
{
	std::vector<svec2> vertices;
	float increment = 1024 / 128.0f;

	for (float x = 0; x < 1024; x += increment)
	{
		for (float z = 0; z < 1024; z += increment)
		{
			vertices.push_back({x, z});
			vertices.push_back({x, z + increment});
			vertices.push_back({x + increment, z + increment});

			vertices.push_back({x + increment, z + increment});
			vertices.push_back({x + increment, z});
			vertices.push_back({x, z});

			shadowTerrainMeshVertexCount += 6;
		}
	}

	StagingBuffer stagBuf = renderer->createAndFillStagingBuffer(vertices.size() * sizeof(vertices[0]), vertices.data());
	shadowTerrainMesh = renderer->createBuffer(vertices.size() * sizeof(vertices[0]), BUFFER_USAGE_VERTEX_BUFFER, true, false, MEMORY_USAGE_GPU_ONLY, false);

	CommandBuffer cmdBuffer = renderer->beginSingleTimeCommand(terrainShadowCmdPool);
	cmdBuffer->stageBuffer(stagBuf, shadowTerrainMesh);
	renderer->endSingleTimeCommand(cmdBuffer, terrainShadowCmdPool, QUEUE_TYPE_GRAPHICS);

	renderer->destroyStagingBuffer(stagBuf);
}

void TerrainShadowRenderer::createDepthPipeline()
{
	ShaderModule vertShader = renderer->createShaderModule("GameData/shaders/vulkan/terrain-depth.glsl", SHADER_STAGE_VERTEX_BIT, SHADER_LANGUAGE_GLSL);
	ShaderModule fragShader = renderer->createShaderModule("GameData/shaders/vulkan/terrain-depth.glsl", SHADER_STAGE_FRAGMENT_BIT, SHADER_LANGUAGE_GLSL);

	VertexInputBinding meshVertexBindingDesc = {};
	meshVertexBindingDesc.binding = 0;
	meshVertexBindingDesc.stride = sizeof(svec2);
	meshVertexBindingDesc.inputRate = VERTEX_INPUT_RATE_VERTEX;

	PipelineShaderStage vertShaderStage = {};
	vertShaderStage.entry = "main";
	vertShaderStage.module = vertShader;

	PipelineShaderStage fragShaderStage = {};
	fragShaderStage.entry = "main";
	fragShaderStage.module = fragShader;

	std::vector<VertexInputAttrib> attribDesc = std::vector<VertexInputAttrib>(1);
	attribDesc[0].binding = 0;
	attribDesc[0].location = 0;
	attribDesc[0].format = RESOURCE_FORMAT_R32G32_SFLOAT;
	attribDesc[0].offset = 0;

	PipelineVertexInputInfo vertexInput = {};
	vertexInput.vertexInputAttribs = attribDesc;
	vertexInput.vertexInputBindings =
	{meshVertexBindingDesc};

	PipelineInputAssemblyInfo inputAssembly = {};
	inputAssembly.primitiveRestart = false;
	inputAssembly.topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	PipelineViewportInfo viewportInfo = {};
	viewportInfo.scissors =
	{
		{0, 0, 2048, 2048}};
	viewportInfo.viewports =
	{
		{0, 0, 2048, 2048, 0, 1}};

	PipelineRasterizationInfo rastInfo = {};
	rastInfo.clockwiseFrontFace = false;
	rastInfo.cullMode = 0;
	rastInfo.lineWidth = 3;
	rastInfo.polygonMode = POLYGON_MODE_FILL;
	rastInfo.rasterizerDiscardEnable = false;
	rastInfo.enableOutOfOrderRasterization = true;

	PipelineDepthStencilInfo depthInfo = {};
	depthInfo.enableDepthTest = true;
	depthInfo.enableDepthWrite = true;
	depthInfo.minDepthBounds = 0;
	depthInfo.maxDepthBounds = 1;
	depthInfo.depthCompareOp = COMPARE_OP_LESS;

	PipelineColorBlendAttachment colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = false;
	colorBlendAttachment.colorWriteMask = COLOR_COMPONENT_R_BIT | COLOR_COMPONENT_G_BIT | COLOR_COMPONENT_B_BIT | COLOR_COMPONENT_A_BIT;

	PipelineColorBlendInfo colorBlend = {};
	colorBlend.attachments = {};
	colorBlend.logicOpEnable = false;
	colorBlend.logicOp = LOGIC_OP_COPY;
	colorBlend.blendConstants[0] = 1.0f;
	colorBlend.blendConstants[1] = 1.0f;
	colorBlend.blendConstants[2] = 1.0f;
	colorBlend.blendConstants[3] = 1.0f;

	PipelineDynamicStateInfo dynamicState = {};
	dynamicState.dynamicStates =
	{};


	GraphicsPipelineInfo info = {};
	info.stages =
	{vertShaderStage, fragShaderStage};
	info.vertexInputInfo = vertexInput;
	info.inputAssemblyInfo = inputAssembly;
	info.viewportInfo = viewportInfo;
	info.rasterizationInfo = rastInfo;
	info.depthStencilInfo = depthInfo;
	info.colorBlendInfo = colorBlend;
	info.dynamicStateInfo = dynamicState;

	info.inputPushConstantRanges = {{0, sizeof(TerrainShadowRendererPushConsts), SHADER_STAGE_VERTEX_BIT}};
	info.inputSetLayouts = {{
		{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_VERTEX_BIT},
		{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_VERTEX_BIT}
		}};

	depthPipeline = renderer->createGraphicsPipeline(info, depthRenderPass, 0);

	renderer->destroyShaderModule(vertShaderStage.module);
	renderer->destroyShaderModule(fragShaderStage.module);
}

void TerrainShadowRenderer::createDepthRenderPass()
{
	AttachmentDescription heightAttachment = {};
	heightAttachment.format = RESOURCE_FORMAT_D16_UNORM;
	heightAttachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
	heightAttachment.finalLayout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	heightAttachment.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
	heightAttachment.storeOp = ATTACHMENT_STORE_OP_STORE;

	AttachmentReference subpass0_heightRef = {};
	subpass0_heightRef.attachment = 0;
	subpass0_heightRef.layout = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	SubpassDescription subpass0 = {};
	subpass0.bindPoint = PIPELINE_BIND_POINT_GRAPHICS;
	subpass0.depthStencilAttachment = &subpass0_heightRef;

	depthRenderPass = renderer->createRenderPass({heightAttachment}, {subpass0}, {});
}