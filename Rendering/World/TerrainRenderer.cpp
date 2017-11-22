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
 * TerrainRenderer.cpp
 * 
 * Created on: Oct 28, 2017
 *     Author: david
 */

#include "Rendering/World/TerrainRenderer.h"

#include <Engine/StarlightEngine.h>

#include <Rendering/Renderer/Renderer.h>
#include <Rendering/World/WorldRenderer.h>

#include <World/WorldHandler.h>

#include <Resources/ResourceManager.h>

TerrainRenderer::TerrainRenderer (StarlightEngine *enginePtr, WorldHandler *worldHandlerPtr, WorldRenderer *worldRendererPtr)
{
	engine = enginePtr;
	world = worldHandlerPtr;
	worldRenderer = worldRendererPtr;

	testHeightmap = nullptr;
	testHeightmapView = nullptr;
	testSampler = nullptr;
	testCommandPool = nullptr;

	terrainPipeline = nullptr;
	terrainPipelineInput = nullptr;
}

TerrainRenderer::~TerrainRenderer ()
{

}

void TerrainRenderer::update ()
{

}

void TerrainRenderer::renderTerrain (CommandBuffer &buf)
{
	buf->beginDebugRegion("Terrain", glm::vec4(0, 0.5f, 0, 1));

	buf->endDebugRegion();
}

ResourceMesh terrainGrid;

void TerrainRenderer::init ()
{
	auto heightmapData = world->getCellHeightmapMipData(0, 0, 0);

	testCommandPool = engine->renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, 0);

	testHeightmap = engine->renderer->createTexture({513, 513, 1}, RESOURCE_FORMAT_R16_UNORM, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_TRANSFER_DST_BIT, MEMORY_USAGE_GPU_ONLY, false);
	testHeightmapView = engine->renderer->createTextureView(testHeightmap);
	testSampler = engine->renderer->createSampler();

	StagingBuffer st = engine->renderer->createStagingBuffer(513 * 513 * 2);
	engine->renderer->mapStagingBuffer(st, 513 * 513 * 2, heightmapData.get());

	CommandBuffer buf = engine->renderer->beginSingleTimeCommand(testCommandPool);

	buf->transitionTextureLayout(testHeightmap, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL);
	buf->stageBuffer(st, testHeightmap);
	buf->transitionTextureLayout(testHeightmap, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	engine->renderer->endSingleTimeCommand(buf, testCommandPool, QUEUE_TYPE_GRAPHICS);

	terrainGrid = engine->resources->loadMeshImmediate("GameData/meshes/test-terrain.dae", "cell_terrain_grid");

	createGraphicsPipeline();
}

void TerrainRenderer::destroy ()
{
	engine->renderer->destroyPipeline(terrainPipeline);
	engine->renderer->destroyPipelineInputLayout(terrainPipelineInput);
}

const uint32_t ivunt_vertexFormatSize = 44;

void TerrainRenderer::createGraphicsPipeline ()
{
	terrainPipelineInput = engine->renderer->createPipelineInputLayout({{0, sizeof(glm::mat4), PIPELINE_STAGE_VERTEX_SHADER_BIT}}, {{{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_VERTEX_BIT}}});

	ShaderModule vertShader = engine->renderer->createShaderModule(engine->getWorkingDir() + "GameData/shaders/vulkan/terrain.glsl", SHADER_STAGE_VERTEX_BIT);
	ShaderModule fragShader = engine->renderer->createShaderModule(engine->getWorkingDir() + "GameData/shaders/vulkan/terrain.glsl", SHADER_STAGE_FRAGMENT_BIT);

	VertexInputBinding meshVertexBindingDesc = {};
	meshVertexBindingDesc.binding = 0;
	meshVertexBindingDesc.stride = ivunt_vertexFormatSize;
	meshVertexBindingDesc.inputRate = VERTEX_INPUT_RATE_VERTEX;

	PipelineShaderStage vertShaderStage = {};
	vertShaderStage.entry = "main";
	vertShaderStage.module = vertShader;

	PipelineShaderStage fragShaderStage = {};
	fragShaderStage.entry = "main";
	fragShaderStage.module = fragShader;

	std::vector<VertexInputAttrib> attribDesc = std::vector<VertexInputAttrib>(4);
	attribDesc[0].binding = 0;
	attribDesc[0].location = 0;
	attribDesc[0].format = RESOURCE_FORMAT_R32G32B32_SFLOAT;
	attribDesc[0].offset = 0;

	attribDesc[1].binding = 0;
	attribDesc[1].location = 1;
	attribDesc[1].format = RESOURCE_FORMAT_R32G32_SFLOAT;
	attribDesc[1].offset = sizeof(glm::vec3);

	attribDesc[2].binding = 0;
	attribDesc[2].location = 2;
	attribDesc[2].format = RESOURCE_FORMAT_R32G32B32_SFLOAT;
	attribDesc[2].offset = sizeof(glm::vec3) + sizeof(glm::vec2);

	attribDesc[3].binding = 0;
	attribDesc[3].location = 3;
	attribDesc[3].format = RESOURCE_FORMAT_R32G32B32_SFLOAT;
	attribDesc[3].offset = sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3);

	PipelineVertexInputInfo vertexInput = {};
	vertexInput.vertexInputAttribs = attribDesc;
	vertexInput.vertexInputBindings =
	{	meshVertexBindingDesc};

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
	rastInfo.cullMode = CULL_MODE_BACK_BIT;
	rastInfo.lineWidth = 1;
	rastInfo.polygonMode = POLYGON_MODE_FILL;
	rastInfo.rasterizerDiscardEnable = false;

	PipelineDepthStencilInfo depthInfo = {};
	depthInfo.enableDepthTest = true;
	depthInfo.enableDepthWrite = true;
	depthInfo.minDepthBounds = 0;
	depthInfo.maxDepthBounds = 1;
	depthInfo.depthCompareOp = COMPARE_OP_GREATER;

	PipelineColorBlendAttachment colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = false;
	colorBlendAttachment.colorWriteMask = COLOR_COMPONENT_R_BIT | COLOR_COMPONENT_G_BIT | COLOR_COMPONENT_B_BIT | COLOR_COMPONENT_A_BIT;

	PipelineColorBlendInfo colorBlend = {};
	colorBlend.attachments =
	{	colorBlendAttachment, colorBlendAttachment};
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

	terrainPipeline = engine->renderer->createGraphicsPipeline(info, terrainPipelineInput, worldRenderer->gbufferRenderPass, 0);

	engine->renderer->destroyShaderModule(vertShaderStage.module);
	engine->renderer->destroyShaderModule(fragShaderStage.module);
}

