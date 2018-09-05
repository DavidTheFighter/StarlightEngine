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
#include <Rendering/World/TerrainShadowRenderer.h>

#include <Game/Game.h>
#include <Game/API/SEAPI.h>

#include <World/WorldHandler.h>

#include <Resources/ResourceManager.h>

#include <frustum.h>

WorldRenderer::WorldRenderer (StarlightEngine *enginePtr, WorldHandler *worldHandlerPtr)
{
	engine = enginePtr;
	world = worldHandlerPtr;
	terrainRenderer = nullptr;

	worldStreamingBuffer = nullptr;
	worldStreamingBufferOffset = 0;
	worldStreamingBufferData = nullptr;

	gbufferRenderPass = nullptr;
	shadowsRenderPass = nullptr;

	testSampler = nullptr;

	sunCSM = nullptr;

	physxDebugStreamingBuffer = nullptr;
}

WorldRenderer::~WorldRenderer ()
{
	terrainRenderer->destroy();
	delete terrainRenderer;

	terrainShadowRenderer->destroy();
	delete terrainShadowRenderer;

	delete sunCSM;

	engine->renderer->destroyPipeline(physxDebugPipeline);

	engine->renderer->destroySampler(testSampler);

	engine->renderer->unmapBuffer(worldStreamingBuffer);
	engine->renderer->destroyBuffer(worldStreamingBuffer);
}

void WorldRenderer::update ()
{
	terrainRenderer->update();
	sunCSM->update(engine->api->getMainCameraViewMat(), glm::vec2(60 * (M_PI / 180.0f), 60 * (M_PI / 180.0f)) / (gbufferRenderSize.x / float(gbufferRenderSize.y)), {1, 10, 75, 400}, engine->api->getSunDirection());
}

void WorldRenderer::gbufferPassInit(RenderPass renderPass, uint32_t baseSubpass)
{
	gbufferRenderPass = renderPass;

	testSampler = engine->renderer->createSampler();

	worldStreamingBuffer = engine->renderer->createBuffer(STATIC_OBJECT_STREAMING_BUFFER_SIZE, BUFFER_USAGE_VERTEX_BUFFER, false, false, MEMORY_USAGE_CPU_TO_GPU, true);
	worldStreamingBufferData = engine->renderer->mapBuffer(worldStreamingBuffer);

	engine->renderer->setObjectDebugName(worldStreamingBuffer, OBJECT_TYPE_BUFFER, "LevelStaticObj Streaming Buffer");

	createPipelines(renderPass, baseSubpass);

	sunCSM = new CSM(engine->renderer, 4096, 3);

	terrainRenderer = new TerrainRenderer(engine, world, this);
	terrainRenderer->init();

	terrainShadowRenderer = new TerrainShadowRenderer(engine->renderer);
	terrainShadowRenderer->init();
	terrainShadowRenderer->setClipmap(terrainRenderer->terrainClipmapView_Elevation, terrainRenderer->terrainClipmapSampler);
}

void WorldRenderer::gbufferPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size)
{
	gbufferRenderSize = size;
}

void WorldRenderer::gbufferPassRender(CommandBuffer cmdBuffer, uint32_t counter)
{
	bool renderPhysicsDebug = bool(engine->api->getDebugVariable("physics"));
	uint32_t physxDebugVertexCount = 0;

	if (physxDebugStreamingBuffer != nullptr)
	{
		engine->renderer->destroyBuffer(physxDebugStreamingBuffer);

		physxDebugStreamingBuffer = nullptr;
	}

	if (renderPhysicsDebug)
	{
		auto physDat = world->getDebugRenderData(world->getActiveLevelData());

		if (physDat.numLines > 0)
		{
			physxDebugStreamingBuffer = engine->renderer->createBuffer(physDat.numLines * sizeof(PhysicsDebugLine), BUFFER_USAGE_VERTEX_BUFFER, true, false, MEMORY_USAGE_CPU_ONLY);
			void *bufDat = engine->renderer->mapBuffer(physxDebugStreamingBuffer);
			memcpy(bufDat, physDat.lines.data(), physDat.numLines * sizeof(PhysicsDebugLine));
			engine->renderer->unmapBuffer(physxDebugStreamingBuffer);

			physxDebugVertexCount = physDat.numLines * 2;
		}
	}

	glm::mat4 camMVPMat = engine->api->getMainCameraProjMat() * engine->api->getMainCameraViewMat();

	renderWorldStaticMeshes(cmdBuffer, camMVPMat, false);
	terrainRenderer->renderTerrain(cmdBuffer);

	if (renderPhysicsDebug && physxDebugVertexCount > 0)
	{
		glm::vec3 cameraCellOffset = glm::floor(Game::instance()->mainCamera.position / float(LEVEL_CELL_SIZE)) * float(LEVEL_CELL_SIZE);

		cmdBuffer->beginDebugRegion("Physics Debug Visualization", glm::vec4(0, 1, 1, 1));

		glm::vec3 cameraPosition = engine->api->getMainCameraPosition();

		cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, physxDebugPipeline);
		cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &camMVPMat[0][0]);
		cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(glm::vec3), &cameraPosition.x);
		cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4) + sizeof(glm::vec4), sizeof(glm::vec3), &cameraCellOffset.x);

		cmdBuffer->bindVertexBuffers(0, {physxDebugStreamingBuffer}, {0});
		cmdBuffer->draw(physxDebugVertexCount);

		cmdBuffer->endDebugRegion();
	}
}

void WorldRenderer::sunShadowsPassInit(RenderPass renderPass, uint32_t baseSubpass)
{
	shadowsRenderPass = renderPass;
}

void WorldRenderer::sunShadowsPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size)
{
	sunShadowsRenderSize = size;
}

void WorldRenderer::sunShadowsPassRender(CommandBuffer cmdBuffer, uint32_t counter)
{
	cmdBuffer->setScissors(0, {{0, 0, sunCSM->getShadowSize(), sunCSM->getShadowSize()}});
	cmdBuffer->setViewports(0, {{0, 0, (float) sunCSM->getShadowSize(), (float) sunCSM->getShadowSize(), 0.0f, 1.0f}});

	renderWorldStaticMeshes(cmdBuffer, sunCSM->getCamProjMat(counter) * sunCSM->getCamViewMat(), true);
}

void WorldRenderer::renderWorldStaticMeshes (CommandBuffer &cmdBuffer, glm::mat4 camMVPMat, bool renderDepth)
{
	glm::vec3 cameraCellOffset = glm::floor(Game::instance()->mainCamera.position / float(LEVEL_CELL_SIZE)) * float(LEVEL_CELL_SIZE);

	//double sT = engine->getTime();
	glm::vec4 frustum[6];
	getFrustum(camMVPMat * glm::translate(glm::mat4(1), -cameraCellOffset), frustum);
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

		glm::vec3 cameraPosition = engine->api->getMainCameraPosition();

		cmdBuffer->beginDebugRegion("For pipeline: " + materialPipeline->defUniqueName, glm::vec4(0.18f, 0.94f, 0.94f, 1.0f));
		cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, renderDepth ? materialPipeline->depthPipeline : materialPipeline->pipeline);
		cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &camMVPMat[0][0]);
		cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(glm::vec3), &cameraPosition.x);
		cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4) + sizeof(glm::vec4), sizeof(glm::vec3), &cameraCellOffset.x);

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

					cmdBuffer->bindIndexBuffer(staticMeshLOD->meshIndexBuffer, 0, staticMeshLOD->uses32bitIndices);
					cmdBuffer->bindVertexBuffers(0, {staticMeshLOD->meshVertexBuffer, worldStreamingBuffer}, {0, worldStreamingBufferOffset * sizeof(dataList[0])});

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
		float cellDistance = glm::distance(engine->api->getMainCameraPosition(), node.cellBB.getCenter());

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
			materialList[objType.meshDefUniqueNameHash] = std::vector<std::vector<LevelStaticObject> >(mesh->meshLODs.size(), std::vector<LevelStaticObject>());
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

void WorldRenderer::createPipelines(RenderPass renderPass, uint32_t baseSubpass)
{
	ShaderModule vertShader = engine->renderer->createShaderModule("GameData/shaders/vulkan/physx-debug-lines.glsl", SHADER_STAGE_VERTEX_BIT, SHADER_LANGUAGE_GLSL);
	ShaderModule fragShader = engine->renderer->createShaderModule("GameData/shaders/vulkan/physx-debug-lines.glsl", SHADER_STAGE_FRAGMENT_BIT, SHADER_LANGUAGE_GLSL);

	VertexInputBinding meshVertexBindingDesc = {};
	meshVertexBindingDesc.binding = 0;
	meshVertexBindingDesc.stride = sizeof(svec3) + sizeof(uint32_t);
	meshVertexBindingDesc.inputRate = VERTEX_INPUT_RATE_VERTEX;

	std::vector<VertexInputAttrib> attribDesc = std::vector<VertexInputAttrib>(2);
	attribDesc[0].binding = 0;
	attribDesc[0].location = 0;
	attribDesc[0].format = RESOURCE_FORMAT_R32G32B32_SFLOAT;
	attribDesc[0].offset = 0;

	attribDesc[1].binding = 0;
	attribDesc[1].location = 1;
	attribDesc[1].format = RESOURCE_FORMAT_R32_UINT;
	attribDesc[1].offset = sizeof(svec3);

	PipelineVertexInputInfo vertexInput = {};
	vertexInput.vertexInputAttribs = attribDesc;
	vertexInput.vertexInputBindings =
	{meshVertexBindingDesc};

	PipelineInputAssemblyInfo inputAssembly = {};
	inputAssembly.primitiveRestart = false;
	inputAssembly.topology = PRIMITIVE_TOPOLOGY_LINE_LIST;

	PipelineViewportInfo viewportInfo = {};
	viewportInfo.scissors =
	{
		{0, 0, 1920, 1080}};
	viewportInfo.viewports =
	{
		{0, 0, 1920, 1080}};

	PipelineRasterizationInfo rastInfo = {};
	rastInfo.clockwiseFrontFace = false;
	rastInfo.cullMode = CULL_MODE_NONE;
	rastInfo.lineWidth = 3;
	rastInfo.polygonMode = POLYGON_MODE_LINE;
	rastInfo.rasterizerDiscardEnable = false;
	rastInfo.enableOutOfOrderRasterization = true;

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
	{colorBlendAttachment, colorBlendAttachment};
	colorBlend.logicOpEnable = false;
	colorBlend.logicOp = LOGIC_OP_COPY;
	colorBlend.blendConstants[0] = 1.0f;
	colorBlend.blendConstants[1] = 1.0f;
	colorBlend.blendConstants[2] = 1.0f;
	colorBlend.blendConstants[3] = 1.0f;

	PipelineDynamicStateInfo dynamicState = {};
	dynamicState.dynamicStates =
	{DYNAMIC_STATE_VIEWPORT, DYNAMIC_STATE_SCISSOR};

	GraphicsPipelineInfo info = {};

	PipelineShaderStage vertShaderStage = {};
	vertShaderStage.entry = "main";
	vertShaderStage.module = vertShader;

	PipelineShaderStage fragShaderStage = {};
	fragShaderStage.entry = "main";
	fragShaderStage.module = fragShader;

	info.stages =
	{vertShaderStage, fragShaderStage};

	info.vertexInputInfo = vertexInput;
	info.inputAssemblyInfo = inputAssembly;
	info.viewportInfo = viewportInfo;
	info.rasterizationInfo = rastInfo;
	info.depthStencilInfo = depthInfo;
	info.colorBlendInfo = colorBlend;
	info.dynamicStateInfo = dynamicState;

	std::vector<DescriptorSetLayoutBinding> layoutBindings;

	info.inputPushConstantRanges = {{0, sizeof(glm::mat4) + sizeof(glm::vec4) + sizeof(glm::vec4), SHADER_STAGE_VERTEX_BIT}};
	info.inputSetLayouts = {layoutBindings};

	physxDebugPipeline = engine->renderer->createGraphicsPipeline(info, renderPass, baseSubpass);

	engine->renderer->destroyShaderModule(vertShader);
	engine->renderer->destroyShaderModule(fragShader);
}
