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
 * WorldRenderer.cpp
 * 
 * Created on: Oct 10, 2017
 *     Author: david
 */

#include "Rendering/World/WorldRenderer.h"

#include <Engine/StarlightEngine.h>

#include <Rendering/Renderer/Renderer.h>
#include <Rendering/World/TerrainRenderer.h>

#include <World/WorldHandler.h>

#include <Resources/ResourceManager.h>

WorldRenderer::WorldRenderer (StarlightEngine *enginePtr, WorldHandler *worldHandlerPtr)
{
	engine = enginePtr;
	world = worldHandlerPtr;
	terrainRenderer = nullptr;

	isDestroyed = false;

	gbufferRenderPass = nullptr;
	gbufferFramebuffer = nullptr;

	defaultMaterialPipeline = nullptr;

	testCommandPool = nullptr;

	gbuffer_AlbedoRoughness = nullptr;
	gbuffer_AlbedoRoughnessView = nullptr;
	gbuffer_NormalMetalness = nullptr;
	gbuffer_NormalMetalnessView = nullptr;
	gbuffer_Depth = nullptr;
	gbuffer_DepthView = nullptr;

	worldStreamingBuffer = nullptr;
	worldStreamingBufferOffset = 0;
	worldStreamingBufferData = nullptr;

	testSampler = nullptr;
}

WorldRenderer::~WorldRenderer ()
{
	if (!isDestroyed)
		destroy();
}

void WorldRenderer::update ()
{
	terrainRenderer->update();
}

void WorldRenderer::render3DWorld ()
{
	std::vector<ClearValue> clearValues = std::vector<ClearValue>(3);
	clearValues[0].color =
	{	0, 0, 0, 0};
	clearValues[1].color =
	{	0, 0, 0, 0};
	clearValues[2].depthStencil =
	{	0, 0};

	CommandBuffer cmdBuffer = testCommandPool->allocateCommandBuffer(COMMAND_BUFFER_LEVEL_PRIMARY);
	cmdBuffer->beginCommands(COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	cmdBuffer->beginRenderPass(gbufferRenderPass, gbufferFramebuffer, {0, 0, gbufferRenderDimensions.x, gbufferRenderDimensions.y}, clearValues, SUBPASS_CONTENTS_INLINE);
	cmdBuffer->setScissors(0, {{0, 0, gbufferRenderDimensions.x, gbufferRenderDimensions.y}});
	cmdBuffer->setViewports(0, {{0, 0, (float) gbufferRenderDimensions.x, (float) gbufferRenderDimensions.y, 0.0f, 1.0f}});

	renderWorldStaticMeshes(cmdBuffer);
	terrainRenderer->renderTerrain(cmdBuffer);

	cmdBuffer->endRenderPass();

	cmdBuffer->endCommands();

	std::vector<Semaphore> waitSems = {};
	std::vector<PipelineStageFlags> waitSemStages = {};

	if (terrainRenderer->clipmapUpdated)
	{
		waitSems.push_back(terrainRenderer->clipmapUpdateSemaphore);
		waitSemStages.push_back(PIPELINE_STAGE_VERTEX_SHADER_BIT);
	}

	engine->renderer->submitToQueue(QUEUE_TYPE_GRAPHICS, {cmdBuffer}, waitSems, waitSemStages);
	engine->renderer->waitForQueueIdle(QUEUE_TYPE_GRAPHICS);
	testCommandPool->freeCommandBuffer(cmdBuffer);
}

void WorldRenderer::renderWorldStaticMeshes (CommandBuffer &cmdBuffer)
{
	glm::mat4 camMVP = camProjMat * camViewMat;

	cmdBuffer->beginDebugRegion("GBuffer Fill", glm::vec4(0.196f, 0.698f, 1.0f, 1.0f));

	{
		cmdBuffer->beginDebugRegion("Level Static Objects", glm::vec4(1.0f, 0.984f, 0.059f, 1.0f));
		cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, defaultMaterialPipeline);
		cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &camMVP[0][0]);
		cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(glm::vec3), &cameraPosition.x);

		//double sT = engine->getTime();
		LevelStaticObjectStreamingData streamData = getStaticObjStreamingData();
		//printf("Stream took: %fms\n", (engine->getTime() - sT) * 1000.0);

		uint32_t drawCallCount = 0;
		for (auto mat = streamData.data.begin(); mat != streamData.data.end(); mat ++)
		{
			ResourceMaterial material = engine->resources->findMaterial(mat->first);

			cmdBuffer->beginDebugRegion("For material: " + material->defUniqueName, glm::vec4(1.0f, 0.467f, 0.02f, 1.0f));
			cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {material->descriptorSet});

			for (auto mesh = mat->second.begin(); mesh != mat->second.end(); mesh ++)
			{
				ResourceStaticMesh staticMesh = engine->resources->findStaticMesh(mesh->first);

				cmdBuffer->insertDebugMarker("For mesh: " + staticMesh->defUniqueName, glm::vec4(1.0f, 0.039f, 0.439f, 1.0f));

				for (uint32_t lod = 0; lod < staticMesh->meshLODs.size(); lod ++)
				{
					std::vector<LevelStaticObject> &dataList = mesh->second[lod];

					if (dataList.size() == 0)
						continue;

					ResourceMesh staticMeshLOD = staticMesh->meshLODs[lod].second;

					size_t meshInstanceDataSize = dataList.size() * sizeof(dataList[0]);

					if (worldStreamingBufferOffset * sizeof(dataList[0]) + meshInstanceDataSize > STATIC_OBJECT_STREAMING_BUFFER_SIZE)
					{
						worldStreamingBufferOffset = 0;
					}

					cmdBuffer->bindIndexBuffer(staticMeshLOD->meshBuffer);
					cmdBuffer->bindVertexBuffers(0, {staticMeshLOD->meshBuffer, worldStreamingBuffer}, {staticMeshLOD->indexChunkSize, worldStreamingBufferOffset * sizeof(dataList[0])});

					memcpy(static_cast<LevelStaticObject*>(worldStreamingBufferData) + worldStreamingBufferOffset, dataList.data(), meshInstanceDataSize);
					worldStreamingBufferOffset += dataList.size();

					drawCallCount ++;

					cmdBuffer->drawIndexed(staticMeshLOD->faceCount * 3, (uint32_t) dataList.size());
				}
			}

			cmdBuffer->endDebugRegion();
		}
		//printf("Draw calls: %u\n", drawCallCount);

		cmdBuffer->endDebugRegion();
	}

	cmdBuffer->endDebugRegion();
}

void WorldRenderer::traverseOctreeNode (SortedOctree<LevelStaticObjectType, LevelStaticObject> &node, LevelStaticObjectStreamingData &data)
{
	for (size_t i = 0; i < node.objectList.size(); i ++)
	{
		const LevelStaticObjectType &objType = node.objectList[i].first;
		std::vector<LevelStaticObject> &objList = node.objectList[i].second;

		ResourceStaticMesh mesh = engine->resources->findStaticMesh(objType.meshDefUniqueNameHash);

		int32_t lod = 0;
		float cellDistance = glm::distance(cameraPosition, node.cellBB.getCenter());

		for (uint32_t meshLod = 0; meshLod < mesh->meshLODs.size() + 1; meshLod ++)
		{
			if (meshLod == mesh->meshLODs.size())
			{
				lod = -1;
				break;
			}

			lod = (int32_t) meshLod;

			if (cellDistance < mesh->meshLODs[meshLod].first)
				break;
		}

		if (lod == -1)
		{
			continue;
		}

		// If there's no mesh data for this material, we need to do some special setup for it
		auto &materialList = data.data[objType.materialDefUniqueNameHash];
		if (materialList.find(objType.meshDefUniqueNameHash) == materialList.end())
		{
			std::vector<std::vector<LevelStaticObject> > dataList;

			for (uint32_t l = 0; l < mesh->meshLODs.size(); l ++)
			{
				dataList.push_back(std::vector<LevelStaticObject>());
			}

			materialList[objType.meshDefUniqueNameHash] = dataList;
		}

		std::vector<LevelStaticObject> &dataList = materialList[objType.meshDefUniqueNameHash][lod];

		dataList.insert(dataList.end(), objList.begin(), objList.end());
	}

	for (int a = 0; a < 8; a ++)
	{
		if (node.activeChildren & (1 << a))
		{
			traverseOctreeNode(*node.children[a], data);
		}
	}
}

LevelStaticObjectStreamingData WorldRenderer::getStaticObjStreamingData ()
{
	LevelStaticObjectStreamingData data = {};

	for (size_t i = 0; i < world->getActiveLevelData()->activeStaticObjectCells.size(); i ++)
	{
		traverseOctreeNode(world->getActiveLevelData()->activeStaticObjectCells[i], data);
	}

	return data;
}

void WorldRenderer::init (suvec2 gbufferDimensions)
{
	gbufferRenderDimensions = gbufferDimensions;

	testCommandPool = engine->renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, 0);

	testSampler = engine->renderer->createSampler();

	worldStreamingBuffer = engine->renderer->createBuffer(STATIC_OBJECT_STREAMING_BUFFER_SIZE, BUFFER_USAGE_VERTEX_BUFFER_BIT, MEMORY_USAGE_CPU_TO_GPU, true);
	worldStreamingBufferData = engine->renderer->mapBuffer(worldStreamingBuffer);

	engine->renderer->setObjectDebugName(worldStreamingBuffer, OBJECT_TYPE_BUFFER, "LevelStaticObj Streaming Buffer");

	createRenderPasses();
	createTestMaterialPipeline();
	createGBuffer();

	terrainRenderer = new TerrainRenderer(engine, world, this);

	terrainRenderer->init();
}

void WorldRenderer::createGBuffer ()
{
	gbuffer_AlbedoRoughness = engine->renderer->createTexture({(float) gbufferRenderDimensions.x, (float) gbufferRenderDimensions.y, 1.0f}, RESOURCE_FORMAT_R8G8B8A8_UNORM, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_COLOR_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true);
	gbuffer_NormalMetalness = engine->renderer->createTexture({(float) gbufferRenderDimensions.x, (float) gbufferRenderDimensions.y, 1.0f}, RESOURCE_FORMAT_A2R10G10B10_UNORM_PACK32, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_COLOR_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true);
	gbuffer_Depth = engine->renderer->createTexture({(float) gbufferRenderDimensions.x, (float) gbufferRenderDimensions.y, 1.0f}, RESOURCE_FORMAT_D32_SFLOAT, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true);

	gbuffer_AlbedoRoughnessView = engine->renderer->createTextureView(gbuffer_AlbedoRoughness);
	gbuffer_NormalMetalnessView = engine->renderer->createTextureView(gbuffer_NormalMetalness);
	gbuffer_DepthView = engine->renderer->createTextureView(gbuffer_Depth);

	gbufferFramebuffer = engine->renderer->createFramebuffer(gbufferRenderPass, {gbuffer_AlbedoRoughnessView, gbuffer_NormalMetalnessView, gbuffer_DepthView}, gbufferRenderDimensions.x, gbufferRenderDimensions.y, 1);

	engine->renderer->setObjectDebugName(gbuffer_AlbedoRoughness, OBJECT_TYPE_TEXTURE, "GBuffer: Albedo & Roughness");
	engine->renderer->setObjectDebugName(gbuffer_NormalMetalness, OBJECT_TYPE_TEXTURE, "GBuffer: Normals & Metalness");
	engine->renderer->setObjectDebugName(gbuffer_Depth, OBJECT_TYPE_TEXTURE, "GBuffer: Depth");
}

void WorldRenderer::destroyGBuffer ()
{
	engine->renderer->destroyFramebuffer(gbufferFramebuffer);

	engine->renderer->destroyTextureView(gbuffer_AlbedoRoughnessView);
	engine->renderer->destroyTextureView(gbuffer_NormalMetalnessView);
	engine->renderer->destroyTextureView(gbuffer_DepthView);

	engine->renderer->destroyTexture(gbuffer_AlbedoRoughness);
	engine->renderer->destroyTexture(gbuffer_NormalMetalness);
	engine->renderer->destroyTexture(gbuffer_Depth);
}

void WorldRenderer::destroy ()
{
	isDestroyed = true;

	terrainRenderer->destroy();
	delete terrainRenderer;

	destroyGBuffer();

	engine->renderer->destroyPipeline(defaultMaterialPipeline);

	engine->renderer->destroyRenderPass(gbufferRenderPass);

	engine->renderer->destroyCommandPool(testCommandPool);
	engine->renderer->destroySampler(testSampler);

	engine->renderer->unmapBuffer(worldStreamingBuffer);
	engine->renderer->destroyBuffer(worldStreamingBuffer);
}

void WorldRenderer::createRenderPasses ()
{
	AttachmentDescription gbufferAlbedoRoughnessAttachment = {}, gbufferNormalMetalnessAttachment = {}, gbufferDepthAttachment = {};
	gbufferAlbedoRoughnessAttachment.format = RESOURCE_FORMAT_R8G8B8A8_UNORM;
	gbufferAlbedoRoughnessAttachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
	gbufferAlbedoRoughnessAttachment.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	gbufferAlbedoRoughnessAttachment.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
	gbufferAlbedoRoughnessAttachment.storeOp = ATTACHMENT_STORE_OP_STORE;

	gbufferNormalMetalnessAttachment.format = RESOURCE_FORMAT_A2R10G10B10_UNORM_PACK32;
	gbufferNormalMetalnessAttachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
	gbufferNormalMetalnessAttachment.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	gbufferNormalMetalnessAttachment.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
	gbufferNormalMetalnessAttachment.storeOp = ATTACHMENT_STORE_OP_STORE;

	gbufferDepthAttachment.format = RESOURCE_FORMAT_D32_SFLOAT;
	gbufferDepthAttachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
	gbufferDepthAttachment.finalLayout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	gbufferDepthAttachment.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
	gbufferDepthAttachment.storeOp = ATTACHMENT_STORE_OP_STORE;

	AttachmentReference subpass0_gbufferAlbedoRoughnessRef = {}, subpass0_gbufferNormalMetalnessRef = {}, subpass0_gbufferDepthRef = {};
	subpass0_gbufferAlbedoRoughnessRef.attachment = 0;
	subpass0_gbufferAlbedoRoughnessRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	subpass0_gbufferNormalMetalnessRef.attachment = 1;
	subpass0_gbufferNormalMetalnessRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	subpass0_gbufferDepthRef.attachment = 2;
	subpass0_gbufferDepthRef.layout = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	SubpassDescription subpass0 = {};
	subpass0.bindPoint = PIPELINE_BIND_POINT_GRAPHICS;
	subpass0.colorAttachments =
	{	subpass0_gbufferAlbedoRoughnessRef, subpass0_gbufferNormalMetalnessRef};
	subpass0.depthStencilAttachment = &subpass0_gbufferDepthRef;

	gbufferRenderPass = engine->renderer->createRenderPass({gbufferAlbedoRoughnessAttachment, gbufferNormalMetalnessAttachment, gbufferDepthAttachment}, {subpass0}, {});
}

const uint32_t ivunt_vertexFormatSize = 44;

void WorldRenderer::createTestMaterialPipeline ()
{
	ShaderModule vertShader = engine->renderer->createShaderModule(engine->getWorkingDir() + "GameData/shaders/vulkan/defaultMaterial.glsl", SHADER_STAGE_VERTEX_BIT);
	ShaderModule fragShader = engine->renderer->createShaderModule(engine->getWorkingDir() + "GameData/shaders/vulkan/defaultMaterial.glsl", SHADER_STAGE_FRAGMENT_BIT);

	VertexInputBinding meshVertexBindingDesc = {}, instanceVertexBindingDesc = {};
	meshVertexBindingDesc.binding = 0;
	meshVertexBindingDesc.stride = ivunt_vertexFormatSize;
	meshVertexBindingDesc.inputRate = VERTEX_INPUT_RATE_VERTEX;

	instanceVertexBindingDesc.binding = 1;
	instanceVertexBindingDesc.stride = sizeof(svec4) * 2;
	instanceVertexBindingDesc.inputRate = VERTEX_INPUT_RATE_INSTANCE;

	PipelineShaderStage vertShaderStage = {};
	vertShaderStage.entry = "main";
	vertShaderStage.module = vertShader;

	PipelineShaderStage fragShaderStage = {};
	fragShaderStage.entry = "main";
	fragShaderStage.module = fragShader;

	std::vector<VertexInputAttrib> attribDesc = std::vector<VertexInputAttrib>(6);
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

	attribDesc[4].binding = 1;
	attribDesc[4].location = 4;
	attribDesc[4].format = RESOURCE_FORMAT_R32G32B32A32_SFLOAT;
	attribDesc[4].offset = 0;

	attribDesc[5].binding = 1;
	attribDesc[5].location = 5;
	attribDesc[5].format = RESOURCE_FORMAT_R32G32B32A32_SFLOAT;
	attribDesc[5].offset = sizeof(svec4);

	PipelineVertexInputInfo vertexInput = {};
	vertexInput.vertexInputAttribs = attribDesc;
	vertexInput.vertexInputBindings =
	{	meshVertexBindingDesc, instanceVertexBindingDesc};

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

	std::vector<DescriptorSetLayoutBinding> layoutBindings;
	layoutBindings.push_back({0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT});
	layoutBindings.push_back({1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT});

	info.inputPushConstantRanges = {{0, sizeof(glm::mat4) + sizeof(glm::vec3), SHADER_STAGE_VERTEX_BIT}};
	info.inputSetLayouts = {layoutBindings};

	defaultMaterialPipeline = engine->renderer->createGraphicsPipeline(info, gbufferRenderPass, 0);

	engine->renderer->destroyShaderModule(vertShaderStage.module);
	engine->renderer->destroyShaderModule(fragShaderStage.module);
}

void WorldRenderer::setGBufferDimensions (suvec2 gbufferDimensions)
{
	engine->renderer->waitForDeviceIdle();

	gbufferRenderDimensions = gbufferDimensions;

	destroyGBuffer();
	createGBuffer();

	camProjMat = glm::perspective<float>(60 * (M_PI / 180.0f), gbufferDimensions.x / float(gbufferDimensions.y), 100000.0f, 0.1f);
	camProjMat[1][1] *= -1;
}

suvec2 WorldRenderer::getGBufferDimensions ()
{
	return gbufferRenderDimensions;
}
