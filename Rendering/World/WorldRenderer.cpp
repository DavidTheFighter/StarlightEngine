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

#include <Game/API/SEAPI.h>

#include <World/WorldHandler.h>

#include <Resources/ResourceManager.h>

#include <frustum.h>

WorldRenderer::WorldRenderer (StarlightEngine *enginePtr, WorldHandler *worldHandlerPtr)
{
	engine = enginePtr;
	world = worldHandlerPtr;
	terrainRenderer = nullptr;

	isDestroyed = false;

	gbufferRenderPass = nullptr;
	gbufferFramebuffer = nullptr;

	renderWorldCommandPool = nullptr;

	worldStreamingBuffer = nullptr;
	worldStreamingBufferOffset = 0;
	worldStreamingBufferData = nullptr;

	testSampler = nullptr;

	sunCSM = nullptr;

	shadowsRenderPass = nullptr;

	cmdBufferIndex = 0;
}

WorldRenderer::~WorldRenderer ()
{
	if (!isDestroyed)
		destroy();
}

void WorldRenderer::update ()
{
	terrainRenderer->update();
	sunCSM->update(camViewMat, glm::vec2(60 * (M_PI / 180.0f), 60 * (M_PI / 180.0f)) / (getGBufferDimensions().x / float(getGBufferDimensions().y)), {1, 20, 100, 500}, engine->api->getSunDirection());
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

	cmdBufferIndex ++;
	cmdBufferIndex %= gbufferFillCmdBuffers.size();

	gbufferFillCmdBuffers[cmdBufferIndex]->beginCommands(COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	gbufferFillCmdBuffers[cmdBufferIndex]->beginRenderPass(gbufferRenderPass, gbufferFramebuffer, {0, 0, gbufferRenderDimensions.x, gbufferRenderDimensions.y}, clearValues, SUBPASS_CONTENTS_INLINE);
	gbufferFillCmdBuffers[cmdBufferIndex]->setScissors(0, {{0, 0, gbufferRenderDimensions.x, gbufferRenderDimensions.y}});
	gbufferFillCmdBuffers[cmdBufferIndex]->setViewports(0, {{0, 0, (float) gbufferRenderDimensions.x, (float) gbufferRenderDimensions.y, 0.0f, 1.0f}});

	gbufferFillCmdBuffers[cmdBufferIndex]->beginDebugRegion("GBuffer Fill", glm::vec4(0.196f, 0.698f, 1.0f, 1.0f));

	renderWorldStaticMeshes(gbufferFillCmdBuffers[cmdBufferIndex], camProjMat * camViewMat, false);
	terrainRenderer->renderTerrain(gbufferFillCmdBuffers[cmdBufferIndex]);

	gbufferFillCmdBuffers[cmdBufferIndex]->endDebugRegion();
	gbufferFillCmdBuffers[cmdBufferIndex]->endRenderPass();
	gbufferFillCmdBuffers[cmdBufferIndex]->endCommands();

	shadowmapCmdBuffers[cmdBufferIndex]->beginCommands(COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	clearValues[2].depthStencil = {1, 0};

	shadowmapCmdBuffers[cmdBufferIndex]->beginDebugRegion("Render Sun CSM", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
	for (uint32_t i = 0; i < sunCSM->getCascadeCount(); i ++)
	{
		shadowmapCmdBuffers[cmdBufferIndex]->beginDebugRegion("Cascade #" + toString(i), glm::vec4(1, 1, 0, 1));
		shadowmapCmdBuffers[cmdBufferIndex]->beginRenderPass(shadowsRenderPass, sunCSMFramebuffers[i], {0, 0, sunCSM->getShadowSize(), sunCSM->getShadowSize()}, {clearValues[2]}, SUBPASS_CONTENTS_INLINE);
		shadowmapCmdBuffers[cmdBufferIndex]->setScissors(0, {{0, 0, sunCSM->getShadowSize(), sunCSM->getShadowSize()}});
		shadowmapCmdBuffers[cmdBufferIndex]->setViewports(0, {{0, 0, (float) sunCSM->getShadowSize(), (float) sunCSM->getShadowSize(), 0.0f, 1.0f}});

		renderWorldStaticMeshes(shadowmapCmdBuffers[cmdBufferIndex], sunCSM->getCamProjMat(i) * sunCSM->getCamViewMat(), true);

		shadowmapCmdBuffers[cmdBufferIndex]->endRenderPass ();
		shadowmapCmdBuffers[cmdBufferIndex]->endDebugRegion();
	}

	shadowmapCmdBuffers[cmdBufferIndex]->endDebugRegion();
	shadowmapCmdBuffers[cmdBufferIndex]->endCommands();

	std::vector<Semaphore> waitSems = {};
	std::vector<PipelineStageFlags> waitSemStages = {};

	if (terrainRenderer->clipmapUpdated)
	{
		waitSems.push_back(terrainRenderer->clipmapUpdateSemaphore);
		waitSemStages.push_back(PIPELINE_STAGE_VERTEX_SHADER_BIT);
	}

	engine->renderer->submitToQueue(QUEUE_TYPE_GRAPHICS, {gbufferFillCmdBuffers[cmdBufferIndex]}, waitSems, waitSemStages, {gbufferFillSemaphores[cmdBufferIndex]});
	engine->renderer->submitToQueue(QUEUE_TYPE_GRAPHICS, {shadowmapCmdBuffers[cmdBufferIndex]}, {}, {}, {shadowmapsSemaphores[cmdBufferIndex]});
}

void WorldRenderer::renderWorldStaticMeshes (CommandBuffer &cmdBuffer, glm::mat4 camMVPMat, bool renderDepth)
{
	//double sT = engine->getTime();
	glm::vec4 frustum[6];
	getFrustum(camMVPMat, frustum);
	LevelStaticObjectStreamingData streamData = getStaticObjStreamingData(frustum);
	//printf("Stream took: %fms\n", (engine->getTime() - sT) * 1000.0);

	std::map<size_t, LevelStaticObjectStreamingDataHierarchy> streamDataByPipeline;

	for (auto mat = streamData.data.begin(); mat != streamData.data.end(); mat ++)
	{
		ResourceMaterial material = engine->resources->findMaterial(mat->first);

		streamDataByPipeline[material->pipelineHash][mat->first] = mat->second;
	}

	cmdBuffer->beginDebugRegion("Level Static Objects", glm::vec4(1.0f, 0.984f, 0.059f, 1.0f));

	for (auto pipeIt = streamDataByPipeline.begin(); pipeIt != streamDataByPipeline.end(); pipeIt ++)
	{
		ResourcePipeline materialPipeline = engine->resources->findPipeline(pipeIt->first);

		cmdBuffer->beginDebugRegion("For pipeline: " + materialPipeline->defUniqueName, glm::vec4(0.18f, 0.94f, 0.94f, 1.0f));
		cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, renderDepth ? materialPipeline->depthPipeline : materialPipeline->pipeline);
		cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &camMVPMat[0][0]);
		cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(glm::vec3), &cameraPosition.x);

		uint32_t drawCallCount = 0;
		for (auto mat = pipeIt->second.begin(); mat != pipeIt->second.end(); mat ++)
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

void WorldRenderer::traverseOctreeNode (SortedOctree<LevelStaticObjectType, LevelStaticObject> &node, LevelStaticObjectStreamingData &data, const glm::vec4 (&frustum)[6])
{
	if (!sphereVisible(node.cellBB.getCenter(), LEVEL_CELL_SIZE * M_SQRT_2 / 2.0f, frustum))
		return;

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
			continue;

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
			traverseOctreeNode(*node.children[a], data, frustum);
		}
	}
}

LevelStaticObjectStreamingData WorldRenderer::getStaticObjStreamingData (const glm::vec4 (&frustum)[6])
{
	LevelStaticObjectStreamingData data = {};

	for (size_t i = 0; i < world->getActiveLevelData()->activeStaticObjectCells.size(); i ++)
	{
		traverseOctreeNode(world->getActiveLevelData()->activeStaticObjectCells[i], data, frustum);
	}

	return data;
}

void WorldRenderer::init (suvec2 gbufferDimensions)
{
	gbufferRenderDimensions = gbufferDimensions;

	renderWorldCommandPool = engine->renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, COMMAND_POOL_RESET_COMMAND_BUFFER_BIT);
	gbufferFillCmdBuffers = renderWorldCommandPool->allocateCommandBuffers(COMMAND_BUFFER_LEVEL_PRIMARY, 3);
	shadowmapCmdBuffers = renderWorldCommandPool->allocateCommandBuffers(COMMAND_BUFFER_LEVEL_PRIMARY, 3);;

	gbufferFillSemaphores = engine->renderer->createSemaphores(3);
	shadowmapsSemaphores = engine->renderer->createSemaphores(3);

	testSampler = engine->renderer->createSampler();

	worldStreamingBuffer = engine->renderer->createBuffer(STATIC_OBJECT_STREAMING_BUFFER_SIZE, BUFFER_USAGE_VERTEX_BUFFER_BIT, MEMORY_USAGE_CPU_TO_GPU, true);
	worldStreamingBufferData = engine->renderer->mapBuffer(worldStreamingBuffer);

	engine->renderer->setObjectDebugName(worldStreamingBuffer, OBJECT_TYPE_BUFFER, "LevelStaticObj Streaming Buffer");

	createRenderPasses();
	createGBuffer();

	sunCSM = new CSM(engine->renderer, 4096, 3);

	for (uint32_t i = 0; i < sunCSM->getCascadeCount(); i ++)
		sunCSMFramebuffers.push_back(engine->renderer->createFramebuffer(shadowsRenderPass, {sunCSM->getCSMTextureSliceView(i)}, sunCSM->getShadowSize(), sunCSM->getShadowSize()));

	terrainRenderer = new TerrainRenderer(engine, world, this);

	terrainRenderer->init();
}

void WorldRenderer::createGBuffer ()
{
	gbuffer[0] = engine->renderer->createTexture({(float) gbufferRenderDimensions.x, (float) gbufferRenderDimensions.y, 1.0f}, RESOURCE_FORMAT_R8G8B8A8_UNORM, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_COLOR_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true);
	gbuffer[1] = engine->renderer->createTexture({(float) gbufferRenderDimensions.x, (float) gbufferRenderDimensions.y, 1.0f}, RESOURCE_FORMAT_A2R10G10B10_UNORM_PACK32, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_COLOR_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true);
	gbuffer[2] = engine->renderer->createTexture({(float) gbufferRenderDimensions.x, (float) gbufferRenderDimensions.y, 1.0f}, RESOURCE_FORMAT_D32_SFLOAT, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true);

	gbufferView[0] = engine->renderer->createTextureView(gbuffer[0]);
	gbufferView[1] = engine->renderer->createTextureView(gbuffer[1]);
	gbufferView[2] = engine->renderer->createTextureView(gbuffer[2]);

	gbufferFramebuffer = engine->renderer->createFramebuffer(gbufferRenderPass, {gbufferView[0], gbufferView[1], gbufferView[2]}, gbufferRenderDimensions.x, gbufferRenderDimensions.y, 1);

	engine->renderer->setObjectDebugName(gbuffer[0], OBJECT_TYPE_TEXTURE, "GBuffer: Albedo,Roughness");
	engine->renderer->setObjectDebugName(gbuffer[1], OBJECT_TYPE_TEXTURE, "GBuffer: Normals,Metalness,AO");
	engine->renderer->setObjectDebugName(gbuffer[2], OBJECT_TYPE_TEXTURE, "GBuffer: Depth");
}

void WorldRenderer::destroyGBuffer ()
{
	engine->renderer->destroyFramebuffer(gbufferFramebuffer);

	engine->renderer->destroyTextureView(gbufferView[0]);
	engine->renderer->destroyTextureView(gbufferView[1]);
	engine->renderer->destroyTextureView(gbufferView[2]);

	engine->renderer->destroyTexture(gbuffer[0]);
	engine->renderer->destroyTexture(gbuffer[1]);
	engine->renderer->destroyTexture(gbuffer[2]);
}

void WorldRenderer::destroy ()
{
	isDestroyed = true;

	terrainRenderer->destroy();
	delete terrainRenderer;

	destroyGBuffer();

	delete sunCSM;

	for (size_t i = 0; i < sunCSMFramebuffers.size(); i ++)
		engine->renderer->destroyFramebuffer(sunCSMFramebuffers[i]);

	for (size_t i = 0; i < gbufferFillSemaphores.size(); i ++)
		engine->renderer->destroySemaphore(gbufferFillSemaphores[i]);

	for (size_t i = 0; i < shadowmapsSemaphores.size(); i ++)
		engine->renderer->destroySemaphore(shadowmapsSemaphores[i]);

	engine->renderer->destroyRenderPass(gbufferRenderPass);
	engine->renderer->destroyRenderPass(shadowsRenderPass);

	engine->renderer->destroyCommandPool(renderWorldCommandPool);
	engine->renderer->destroySampler(testSampler);

	engine->renderer->unmapBuffer(worldStreamingBuffer);
	engine->renderer->destroyBuffer(worldStreamingBuffer);
}

void WorldRenderer::createRenderPasses ()
{
	// GBuffer Render Pass
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

	// Shadowmaps Render Pass
	{
		AttachmentDescription shadowsDepthAttachment = {};
		shadowsDepthAttachment.format = RESOURCE_FORMAT_D16_UNORM;
		shadowsDepthAttachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
		shadowsDepthAttachment.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		shadowsDepthAttachment.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
		shadowsDepthAttachment.storeOp = ATTACHMENT_STORE_OP_STORE;

		AttachmentReference subpass0_shadowsDepthRef = {};
		subpass0_shadowsDepthRef.attachment = 0;
		subpass0_shadowsDepthRef.layout = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		SubpassDescription subpass0 = {};
		subpass0.bindPoint = PIPELINE_BIND_POINT_GRAPHICS;
		subpass0.depthStencilAttachment = &subpass0_shadowsDepthRef;

		shadowsRenderPass = engine->renderer->createRenderPass({shadowsDepthAttachment}, {subpass0}, {});
	}
}

void WorldRenderer::setGBufferDimensions (suvec2 gbufferDimensions)
{
	engine->renderer->waitForDeviceIdle();

	gbufferRenderDimensions = gbufferDimensions;

	destroyGBuffer();
	createGBuffer();

	camProjMat = glm::perspective<float>(60 * (M_PI / 180.0f), gbufferDimensions.x / float(gbufferDimensions.y), 100000.0f, 0.1f);

	float hfov = 2.0f * std::atan(1 / camProjMat[1][1]);
	float ar = camProjMat[1][1] / (camProjMat[0][0]);

	printf("%f %f\n", hfov, ar);

	camProjMat[1][1] *= -1;
}

suvec2 WorldRenderer::getGBufferDimensions ()
{
	return gbufferRenderDimensions;
}
