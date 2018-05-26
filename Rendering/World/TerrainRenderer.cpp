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

#include <Game/Game.h>

TerrainRenderer::TerrainRenderer (StarlightEngine *enginePtr, WorldHandler *worldHandlerPtr, WorldRenderer *worldRendererPtr)
{
	engine = enginePtr;
	world = worldHandlerPtr;
	worldRenderer = worldRendererPtr;

	clipmapUpdated = false;
	clipmapUpdateSemaphore = nullptr;
	clipmapUpdateCommandPool = nullptr;
	clipmapUpdateCommandBuffer = nullptr;

	terrainPipeline = nullptr;

	terrainCellMeshVertexCount = 0;
	terrainCellMesh = nullptr;

	heightmapDescriptorPool = nullptr;
	heightmapDescriptorSet = nullptr;

	transferClipmap_Elevation = nullptr;
	terrainClipmap_Elevation = nullptr;

	terrainClipmapView_Elevation = nullptr;
	terrainClipmapSampler = nullptr;
	terrainTextureSampler = nullptr;

	for (int i = 0; i < 4; i ++)
	{
		clipmap0Regions[i % 2][i / 2] =
		{	std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
	}

	for (int i = 0; i < 16; i ++)
	{
		clipmap1Regions[i % 4][i / 4] =
		{	std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
	}

	for (int i = 0; i < 64; i ++)
	{
		clipmap2Regions[i % 8][i / 8] =
		{	std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
	}

	for (int i = 0; i < 256; i ++)
	{
		clipmap3Regions[i % 16][i / 16] =
		{	std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
	}

	for (int i = 0; i < 1024; i ++)
	{
		clipmap3Regions[i % 32][i / 32] =
		{	std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
	}
}

TerrainRenderer::~TerrainRenderer ()
{

}

struct ClipmapUpdate
{
		sivec2 localCoord;
		StagingBuffer stagingData;
};

void TerrainRenderer::update ()
{
	clipmapUpdated = false;
	/*
	 CommandBuffer cmdBuffer = engine->renderer->beginSingleTimeCommand(testCommandPool);

	 cmdBuffer->setTextureLayout(terrainClipmap_Elevation, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_TOP_OF_PIPE_BIT, PIPELINE_STAGE_TRANSFER_BIT);


	 engine->renderer->endSingleTimeCommand(cmdBuffer, testCommandPool, QUEUE_TYPE_GRAPHICS);
	 */

	float cameraCellXNorm = ((worldRenderer->cameraPosition.x - LEVEL_CELL_SIZE * 0.5f) / float(LEVEL_CELL_SIZE));
	float cameraCellZNorm = ((worldRenderer->cameraPosition.z - LEVEL_CELL_SIZE * 0.5f) / float(LEVEL_CELL_SIZE));
	int32_t cameraCellX = int32_t(floor(cameraCellXNorm));
	int32_t cameraCellZ = int32_t(floor(cameraCellZNorm));

	std::vector<ClipmapUpdate> dirtyClipmap0Regions, dirtyClipmap1Regions, dirtyClipmap2Regions, dirtyClipmap3Regions, dirtyClipmap4Regions;

	// Clipmap 0 checks
	{
		for (int x = 0; x < 2; x ++)
		{
			for (int y = 0; y < 2; y ++)
			{
				if (cameraCellX + x != clipmap0Regions[x][y].x || cameraCellZ + y != clipmap0Regions[x][y].y)
				{
					std::unique_ptr<uint16_t> elevationData(world->getCellHeightmapMipData(cameraCellZ + y, cameraCellX + x, 0));

					if (elevationData.get() == nullptr)
					{
						continue;
					}

					StagingBuffer stagBuf = engine->renderer->createAndMapStagingBuffer(513 * 513 * 2, elevationData.get());

					dirtyClipmap0Regions.push_back({{x, y}, stagBuf});

					clipmap0Regions[x][y] =
					{	cameraCellX + x, cameraCellZ + y};
				}
			}
		}
	}

	// Clipmap 1 checks
	{
		for (int x = 0; x < 4; x ++)
		{
			for (int y = 0; y < 4; y ++)
			{
				if (cameraCellX + x - 1 != clipmap1Regions[x][y].x || cameraCellZ + y - 1 != clipmap1Regions[x][y].y)
				{
					std::unique_ptr<uint16_t> elevationData(world->getCellHeightmapMipData(cameraCellZ + y - 1, cameraCellX + x - 1, 1));

					if (elevationData.get() == nullptr)
					{
						continue;
					}

					StagingBuffer stagBuf = engine->renderer->createAndMapStagingBuffer(257 * 257 * 2, elevationData.get());

					dirtyClipmap1Regions.push_back({{x, y}, stagBuf});

					clipmap1Regions[x][y] =
					{	cameraCellX + x - 1, cameraCellZ + y - 1};
				}
			}
		}
	}

	// Clipmap 2 checks
	{
		for (int x = 0; x < 8; x ++)
		{
			for (int y = 0; y < 8; y ++)
			{
				if (cameraCellX + x - 3 != clipmap2Regions[x][y].x || cameraCellZ + y - 3 != clipmap2Regions[x][y].y)
				{
					std::unique_ptr<uint16_t> elevationData(world->getCellHeightmapMipData(cameraCellZ + y - 3, cameraCellX + x - 3, 2));

					if (elevationData.get() == nullptr)
					{
						continue;
					}

					StagingBuffer stagBuf = engine->renderer->createAndMapStagingBuffer(129 * 129 * 2, elevationData.get());

					dirtyClipmap2Regions.push_back({{x, y}, stagBuf});

					clipmap2Regions[x][y] =
					{	cameraCellX + x - 3, cameraCellZ + y - 3};
				}
			}
		}
	}

	// Clipmap 3 checks
	{
		for (int x = 0; x < 16; x ++)
		{
			for (int y = 0; y < 16; y ++)
			{
				if (cameraCellX + x - 7 != clipmap3Regions[x][y].x || cameraCellZ + y - 7 != clipmap3Regions[x][y].y)
				{
					std::unique_ptr<uint16_t> elevationData(world->getCellHeightmapMipData(cameraCellZ + y - 7, cameraCellX + x - 7, 3));

					if (elevationData.get() == nullptr)
					{
						continue;
					}

					StagingBuffer stagBuf = engine->renderer->createAndMapStagingBuffer(65 * 65 * 2, elevationData.get());

					dirtyClipmap3Regions.push_back({{x, y}, stagBuf});

					clipmap3Regions[x][y] =
					{	cameraCellX + x - 7, cameraCellZ + y - 7};
				}
			}
		}
	}

	// Clipmap 4 checks
	{
		for (int x = 0; x < 32; x ++)
		{
			for (int y = 0; y < 32; y ++)
			{
				if (cameraCellX + x - 15 != clipmap4Regions[x][y].x || cameraCellZ + y - 15 != clipmap4Regions[x][y].y)
				{
					std::unique_ptr<uint16_t> elevationData(world->getCellHeightmapMipData(cameraCellZ + y - 15, cameraCellX + x - 15, 4));

					if (elevationData.get() == nullptr)
					{
						continue;
					}

					StagingBuffer stagBuf = engine->renderer->createAndMapStagingBuffer(33 * 33 * 2, elevationData.get());

					dirtyClipmap4Regions.push_back({{x, y}, stagBuf});

					clipmap4Regions[x][y] =
					{	cameraCellX + x - 15, cameraCellZ + y - 15};
				}
			}
		}
	}

	if ((dirtyClipmap0Regions.size() + dirtyClipmap1Regions.size() + dirtyClipmap2Regions.size() + dirtyClipmap3Regions.size() + dirtyClipmap4Regions.size()) > 0)
	{
		clipmapUpdateCommandBuffer->beginCommands(COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		clipmapUpdateCommandBuffer->setTextureLayout(terrainClipmap_Elevation, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {0, 1, 0, 5}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_TRANSFER_BIT);

		if (dirtyClipmap0Regions.size() > 0)
		{
			for (size_t i = 0; i < dirtyClipmap0Regions.size(); i ++)
			{
				sivec2 regionCoord = dirtyClipmap0Regions[i].localCoord;

				TextureBlitInfo blitInfo = {};
				blitInfo.srcSubresource =
				{	0, 0, 1};
				blitInfo.dstSubresource =
				{	0, 0, 1};
				blitInfo.srcOffsets[0] =
				{	0, 0, 0};
				blitInfo.dstOffsets[0] =
				{	regionCoord.x * 512, regionCoord.y * 512, 0};
				blitInfo.srcOffsets[1] =
				{	513, 513, 1};
				blitInfo.dstOffsets[1] =
				{	regionCoord.x * 512 + 513, regionCoord.y * 512 + 513, 1};

				clipmapUpdateCommandBuffer->setTextureLayout(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_TRANSFER_BIT);
				clipmapUpdateCommandBuffer->stageBuffer(dirtyClipmap0Regions[i].stagingData, transferClipmap_Elevation);
				clipmapUpdateCommandBuffer->setTextureLayout(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_TRANSFER_BIT);

				clipmapUpdateCommandBuffer->blitTexture(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, terrainClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {blitInfo}, SAMPLER_FILTER_NEAREST);
			}
		}

		if (dirtyClipmap1Regions.size() > 0)
		{
			for (size_t i = 0; i < dirtyClipmap1Regions.size(); i ++)
			{
				sivec2 regionCoord = dirtyClipmap1Regions[i].localCoord;

				TextureBlitInfo blitInfo = {};
				blitInfo.srcSubresource =
				{	0, 0, 1};
				blitInfo.dstSubresource =
				{	0, 1, 1};
				blitInfo.srcOffsets[0] =
				{	0, 0, 0};
				blitInfo.dstOffsets[0] =
				{	regionCoord.x * 256, regionCoord.y * 256, 0};
				blitInfo.srcOffsets[1] =
				{	257, 257, 1};
				blitInfo.dstOffsets[1] =
				{	regionCoord.x * 256 + 257, regionCoord.y * 256 + 257, 1};

				clipmapUpdateCommandBuffer->setTextureLayout(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_TRANSFER_BIT);
				clipmapUpdateCommandBuffer->stageBuffer(dirtyClipmap1Regions[i].stagingData, transferClipmap_Elevation, {0, 0, 1}, {0, 0, 0}, {257, 257, 1});
				clipmapUpdateCommandBuffer->setTextureLayout(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_TRANSFER_BIT);

				clipmapUpdateCommandBuffer->blitTexture(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, terrainClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {blitInfo}, SAMPLER_FILTER_NEAREST);
			}
		}

		if (dirtyClipmap2Regions.size() > 0)
		{
			for (size_t i = 0; i < dirtyClipmap2Regions.size(); i ++)
			{
				sivec2 regionCoord = dirtyClipmap2Regions[i].localCoord;

				TextureBlitInfo blitInfo = {};
				blitInfo.srcSubresource =
				{	0, 0, 1};
				blitInfo.dstSubresource =
				{	0, 2, 1};
				blitInfo.srcOffsets[0] =
				{	0, 0, 0};
				blitInfo.dstOffsets[0] =
				{	regionCoord.x * 128, regionCoord.y * 128, 0};
				blitInfo.srcOffsets[1] =
				{	129, 129, 1};
				blitInfo.dstOffsets[1] =
				{	regionCoord.x * 128 + 129, regionCoord.y * 128 + 129, 1};

				clipmapUpdateCommandBuffer->setTextureLayout(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_TRANSFER_BIT);
				clipmapUpdateCommandBuffer->stageBuffer(dirtyClipmap2Regions[i].stagingData, transferClipmap_Elevation, {0, 0, 1}, {0, 0, 0}, {129, 129, 1});
				clipmapUpdateCommandBuffer->setTextureLayout(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_TRANSFER_BIT);

				clipmapUpdateCommandBuffer->blitTexture(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, terrainClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {blitInfo}, SAMPLER_FILTER_NEAREST);
			}
		}

		if (dirtyClipmap3Regions.size() > 0)
		{
			for (size_t i = 0; i < dirtyClipmap3Regions.size(); i ++)
			{
				sivec2 regionCoord = dirtyClipmap3Regions[i].localCoord;

				TextureBlitInfo blitInfo = {};
				blitInfo.srcSubresource =
				{	0, 0, 1};
				blitInfo.dstSubresource =
				{	0, 3, 1};
				blitInfo.srcOffsets[0] =
				{	0, 0, 0};
				blitInfo.dstOffsets[0] =
				{	regionCoord.x * 64, regionCoord.y * 64, 0};
				blitInfo.srcOffsets[1] =
				{	65, 65, 1};
				blitInfo.dstOffsets[1] =
				{	regionCoord.x * 64 + 65, regionCoord.y * 64 + 65, 1};

				clipmapUpdateCommandBuffer->setTextureLayout(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_TRANSFER_BIT);
				clipmapUpdateCommandBuffer->stageBuffer(dirtyClipmap3Regions[i].stagingData, transferClipmap_Elevation, {0, 0, 1}, {0, 0, 0}, {65, 65, 1});
				clipmapUpdateCommandBuffer->setTextureLayout(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_TRANSFER_BIT);

				clipmapUpdateCommandBuffer->blitTexture(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, terrainClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {blitInfo}, SAMPLER_FILTER_NEAREST);
			}
		}

		if (dirtyClipmap4Regions.size() > 0)
		{
			for (size_t i = 0; i < dirtyClipmap4Regions.size(); i ++)
			{
				sivec2 regionCoord = dirtyClipmap4Regions[i].localCoord;

				TextureBlitInfo blitInfo = {};
				blitInfo.srcSubresource =
				{	0, 0, 1};
				blitInfo.dstSubresource =
				{	0, 4, 1};
				blitInfo.srcOffsets[0] =
				{	0, 0, 0};
				blitInfo.dstOffsets[0] =
				{	regionCoord.x * 32, regionCoord.y * 32, 0};
				blitInfo.srcOffsets[1] =
				{	33, 33, 1};
				blitInfo.dstOffsets[1] =
				{	regionCoord.x * 32 + 33, regionCoord.y * 32 + 33, 1};

				clipmapUpdateCommandBuffer->setTextureLayout(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_TRANSFER_BIT);
				clipmapUpdateCommandBuffer->stageBuffer(dirtyClipmap4Regions[i].stagingData, transferClipmap_Elevation, {0, 0, 1}, {0, 0, 0}, {33, 33, 1});
				clipmapUpdateCommandBuffer->setTextureLayout(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_TRANSFER_BIT);

				clipmapUpdateCommandBuffer->blitTexture(transferClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, terrainClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {blitInfo}, SAMPLER_FILTER_NEAREST);
			}
		}

		clipmapUpdateCommandBuffer->setTextureLayout(terrainClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 5}, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
		clipmapUpdateCommandBuffer->endCommands();

		engine->renderer->submitToQueue(QUEUE_TYPE_GRAPHICS, {clipmapUpdateCommandBuffer}, {}, {}, {clipmapUpdateSemaphore});
		clipmapUpdated = true;
	}

	for (size_t i = 0; i < clipmapStagingBuffersToDelete.size(); i ++)
	{
		engine->renderer->destroyStagingBuffer(clipmapStagingBuffersToDelete[i]);
	}

	clipmapStagingBuffersToDelete.clear();

	for (size_t i = 0; i < dirtyClipmap0Regions.size(); i ++)
		clipmapStagingBuffersToDelete.push_back(dirtyClipmap0Regions[i].stagingData);

	for (size_t i = 0; i < dirtyClipmap1Regions.size(); i ++)
		clipmapStagingBuffersToDelete.push_back(dirtyClipmap1Regions[i].stagingData);

	for (size_t i = 0; i < dirtyClipmap2Regions.size(); i ++)
		clipmapStagingBuffersToDelete.push_back(dirtyClipmap2Regions[i].stagingData);

	for (size_t i = 0; i < dirtyClipmap3Regions.size(); i ++)
		clipmapStagingBuffersToDelete.push_back(dirtyClipmap3Regions[i].stagingData);

	for (size_t i = 0; i < dirtyClipmap4Regions.size(); i ++)
		clipmapStagingBuffersToDelete.push_back(dirtyClipmap4Regions[i].stagingData);
}

const size_t pcSize = sizeof(glm::mat4) + sizeof(svec2) + sizeof(svec2) + sizeof(glm::vec4) + sizeof(glm::vec4) + sizeof(int32_t);
ResourceMaterial testTerrainGraniteMaterial = nullptr;
ResourceMaterial testTerrainGrassMaterial = nullptr;

void TerrainRenderer::renderTerrain (CommandBuffer &cmdBuffer)
{
	svec2 cameraCellCoords = {(float) floor((worldRenderer->cameraPosition.x - LEVEL_CELL_SIZE * 0.5f) / float(LEVEL_CELL_SIZE)), (float) floor((worldRenderer->cameraPosition.z - LEVEL_CELL_SIZE * 0.5f) / float(LEVEL_CELL_SIZE))};
	svec2 cellCoordStart = {0, 0};
	int32_t instanceCountWidth = 16;
	glm::vec3 cameraCellOffset = glm::floor(Game::instance()->mainCamera.position / float(LEVEL_CELL_SIZE)) * float(LEVEL_CELL_SIZE);

	glm::mat4 camMVP = worldRenderer->camProjMat * worldRenderer->camViewMat;

	char pushConstData[pcSize];
	size_t seqOffset = 0;
	memset(pushConstData, 0, sizeof(pushConstData));

	seqmemcpy(pushConstData, &camMVP[0][0], sizeof(glm::mat4), seqOffset);
	seqmemcpy(pushConstData, &cameraCellCoords.x, sizeof(svec2), seqOffset);
	seqmemcpy(pushConstData, &cellCoordStart.x, sizeof(svec2), seqOffset);
	seqmemcpy(pushConstData, &worldRenderer->cameraPosition.x, sizeof(glm::vec4), seqOffset);
	seqmemcpy(pushConstData, &cameraCellOffset.x, sizeof(glm::vec4), seqOffset);
	seqmemcpy(pushConstData, &instanceCountWidth, sizeof(int32_t), seqOffset);

	//printf("%f - %f - %f\n", worldRenderer->cameraPosition.x, worldRenderer->cameraPosition.y, worldRenderer->cameraPosition.z);

	cmdBuffer->beginDebugRegion("Terrain", glm::vec4(0, 0.5f, 0, 1));
	cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, terrainPipeline);
	cmdBuffer->pushConstants(SHADER_STAGE_TESSELLATION_EVALUATION_BIT | SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, pcSize, pushConstData);

	cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {heightmapDescriptorSet});

	//cmdBuffer->bindIndexBuffer(terrainGrid->meshBuffer, 0, false);
	//cmdBuffer->bindVertexBuffers(0, {terrainGrid->meshBuffer}, {terrainGrid->indexChunkSize});

	//cmdBuffer->drawIndexed(terrainGrid->faceCount * 3, 256);

	cmdBuffer->bindVertexBuffers(0, {terrainCellMesh}, {0});
	cmdBuffer->draw(terrainCellMeshVertexCount, 256);

	cmdBuffer->endDebugRegion();
}

void TerrainRenderer::init ()
{
	auto heightmapData = world->getCellHeightmapMipData(0, 0, 0);

	clipmapUpdateSemaphore = engine->renderer->createSemaphore();

	clipmapUpdateCommandPool = engine->renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, COMMAND_POOL_RESET_COMMAND_BUFFER_BIT);
	clipmapUpdateCommandBuffer = clipmapUpdateCommandPool->allocateCommandBuffer(COMMAND_BUFFER_LEVEL_PRIMARY);

	terrainClipmapSampler = engine->renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	terrainTextureSampler = engine->renderer->createSampler();

	transferClipmap_Elevation = engine->renderer->createTexture({513, 513, 1}, RESOURCE_FORMAT_R16_UNORM, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_TRANSFER_DST_BIT | TEXTURE_USAGE_TRANSFER_SRC_BIT, MEMORY_USAGE_GPU_ONLY, true, 1, 1);
	terrainClipmap_Elevation = engine->renderer->createTexture({1025, 1025, 1}, RESOURCE_FORMAT_R16_UNORM, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_TRANSFER_DST_BIT, MEMORY_USAGE_GPU_ONLY, true, 1, 5);

	terrainClipmapView_Elevation = engine->renderer->createTextureView(terrainClipmap_Elevation, TEXTURE_VIEW_TYPE_2D_ARRAY, {0, 1, 0, 5});

	StagingBuffer st = engine->renderer->createStagingBuffer(513 * 513 * 2);
	engine->renderer->mapStagingBuffer(st, 513 * 513 * 2, heightmapData.get());

	CommandBuffer buf = engine->renderer->beginSingleTimeCommand(clipmapUpdateCommandPool);

	buf->setTextureLayout(transferClipmap_Elevation, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
	buf->setTextureLayout(terrainClipmap_Elevation, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 5}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);

	engine->renderer->endSingleTimeCommand(buf, clipmapUpdateCommandPool, QUEUE_TYPE_GRAPHICS);

	{
		MaterialDef granite = {};
		strcpy(granite.uniqueName, "granite1");
		strcpy(granite.pipelineUniqueName, "engine.defaultMaterial");
		strcpy(granite.textureFiles[0], "GameData/textures/terrain/granite1/granite-albedo.png");
		strcpy(granite.textureFiles[1], "GameData/textures/terrain/granite1/granite-normals.png");
		strcpy(granite.textureFiles[2], "GameData/textures/terrain/granite1/granite-roughness.png");
		strcpy(granite.textureFiles[3], "GameData/textures/terrain/granite1/granite-metalness.png");
		strcpy(granite.textureFiles[4], "");

		granite.enableAnisotropy = true;
		granite.linearFiltering = true;
		granite.linearMipmapFiltering = true;
		granite.addressMode = SAMPLER_ADDRESS_MODE_REPEAT;

		engine->resources->addMaterialDef(granite);

		MaterialDef grass = {};
		strcpy(grass.uniqueName, "grass1");
		strcpy(grass.pipelineUniqueName, "engine.defaultMaterial");
		strcpy(grass.textureFiles[0], "GameData/textures/terrain/grass1/grass-albedo.png");
		strcpy(grass.textureFiles[1], "GameData/textures/terrain/grass1/grass-normals.png");
		strcpy(grass.textureFiles[2], "GameData/textures/terrain/grass1/grass-roughness.png");
		strcpy(grass.textureFiles[3], "GameData/textures/terrain/grass1/grass-metalness.png");
		strcpy(grass.textureFiles[4], "");

		grass.enableAnisotropy = true;
		grass.linearFiltering = true;
		grass.linearMipmapFiltering = true;
		grass.addressMode = SAMPLER_ADDRESS_MODE_REPEAT;

		engine->resources->addMaterialDef(grass);

		testTerrainGraniteMaterial = engine->resources->loadMaterialImmediate(granite.uniqueName);
		testTerrainGrassMaterial = engine->resources->loadMaterialImmediate(grass.uniqueName);
	}

	buildTerrainCellGrids();

	createGraphicsPipeline();

	heightmapDescriptorPool = engine->renderer->createDescriptorPool({
		{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_TESSELLATION_EVALUATION_BIT | SHADER_STAGE_FRAGMENT_BIT},
		{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_TESSELLATION_EVALUATION_BIT | SHADER_STAGE_FRAGMENT_BIT},
		{2, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{3, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 32, SHADER_STAGE_FRAGMENT_BIT}
	}, 8);
	heightmapDescriptorSet = heightmapDescriptorPool->allocateDescriptorSet();

	DescriptorImageInfo heightmapDescriptorImageInfo = {};
	heightmapDescriptorImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	heightmapDescriptorImageInfo.sampler = terrainClipmapSampler;
	heightmapDescriptorImageInfo.view = terrainClipmapView_Elevation;

	DescriptorImageInfo testTerrainGraniteMaterialDescriptorImageInfo = {};
	testTerrainGraniteMaterialDescriptorImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	testTerrainGraniteMaterialDescriptorImageInfo.sampler = testTerrainGraniteMaterial->sampler;
	testTerrainGraniteMaterialDescriptorImageInfo.view = testTerrainGraniteMaterial->textures->textureView;

	DescriptorImageInfo testTerrainGrassMaterialDescriptorImageInfo = {};
	testTerrainGrassMaterialDescriptorImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	testTerrainGrassMaterialDescriptorImageInfo.sampler = testTerrainGrassMaterial->sampler;
	testTerrainGrassMaterialDescriptorImageInfo.view = testTerrainGrassMaterial->textures->textureView;
	
	DescriptorWriteInfo swrite = {}, iwrite = {}, s1write = {}, i1write = {};
	swrite.descriptorCount = 1;
	swrite.descriptorType = DESCRIPTOR_TYPE_SAMPLER;
	swrite.dstBinding = 0;
	swrite.dstSet = heightmapDescriptorSet;
	swrite.imageInfo =
	{	heightmapDescriptorImageInfo};

	iwrite.descriptorCount = 1;
	iwrite.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	iwrite.dstBinding = 1;
	iwrite.dstSet = heightmapDescriptorSet;
	iwrite.imageInfo =
	{	heightmapDescriptorImageInfo};

	s1write.descriptorCount = 1;
	s1write.descriptorType = DESCRIPTOR_TYPE_SAMPLER;
	s1write.dstBinding = 2;
	s1write.dstSet = heightmapDescriptorSet;
	s1write.imageInfo =
	{	testTerrainGraniteMaterialDescriptorImageInfo};

	i1write.descriptorCount = 32;
	i1write.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	i1write.dstBinding = 3;
	i1write.dstSet = heightmapDescriptorSet;
	i1write.imageInfo =
	{	testTerrainGraniteMaterialDescriptorImageInfo, testTerrainGrassMaterialDescriptorImageInfo};

	std::vector<DescriptorImageInfo> testFiller(30, testTerrainGrassMaterialDescriptorImageInfo);
	i1write.imageInfo.insert(i1write.imageInfo.end(), testFiller.begin(), testFiller.end());

	engine->renderer->writeDescriptorSets({swrite, iwrite, s1write, i1write});

	TextureBlitInfo blitInfo = {};
	blitInfo.srcSubresource =
	{	0, 0, 1};
	blitInfo.dstSubresource =
	{	0, 0, 1};
	blitInfo.srcOffsets[0] =
	{	0, 0, 0};
	blitInfo.dstOffsets[0] =
	{	0, 0, 0};
	blitInfo.srcOffsets[1] =
	{	int32_t(513), int32_t(513), 1};
	blitInfo.dstOffsets[1] =
	{	int32_t(513), int32_t(513), 1};

	/*
	 CommandBuffer clipmapTextureLayoutCmdBuffer = engine->renderer->beginSingleTimeCommand(testCommandPool);

	 //clipmapTextureLayoutCmdBuffer->setTextureLayout(terrainClipmap_Elevation, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 5}, PIPELINE_STAGE_TOP_OF_PIPE_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
	 clipmapTextureLayoutCmdBuffer->setTextureLayout(testHeightmap, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);

	 clipmapTextureLayoutCmdBuffer->transitionTextureLayout(terrainClipmap_Elevation, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {0, 1, 0, 5});
	 //clipmapTextureLayoutCmdBuffer->transitionTextureLayout(testHeightmap, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	 clipmapTextureLayoutCmdBuffer->blitTexture(testHeightmap, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, terrainClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {blitInfo}, SAMPLER_FILTER_NEAREST);

	 //clipmapTextureLayoutCmdBuffer->transitionTextureLayout(testHeightmap, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	 clipmapTextureLayoutCmdBuffer->setTextureLayout(testHeightmap, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 1}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
	 clipmapTextureLayoutCmdBuffer->setTextureLayout(terrainClipmap_Elevation, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 5}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);

	 engine->renderer->endSingleTimeCommand(clipmapTextureLayoutCmdBuffer, testCommandPool, QUEUE_TYPE_GRAPHICS);
	 */
}

void TerrainRenderer::destroy ()
{
	engine->resources->returnMaterial(testTerrainGraniteMaterial->defUniqueName);
	engine->resources->returnMaterial(testTerrainGrassMaterial->defUniqueName);

	engine->renderer->destroySampler(terrainClipmapSampler);
	engine->renderer->destroySampler(terrainTextureSampler);

	engine->renderer->destroyTexture(transferClipmap_Elevation);
	engine->renderer->destroyTexture(terrainClipmap_Elevation);

	engine->renderer->destroyTextureView(terrainClipmapView_Elevation);

	engine->renderer->destroyBuffer(terrainCellMesh);

	engine->renderer->destroyPipeline(terrainPipeline);
	engine->renderer->destroyDescriptorPool(heightmapDescriptorPool);

	engine->renderer->destroySemaphore(clipmapUpdateSemaphore);
	engine->renderer->destroyCommandPool(clipmapUpdateCommandPool);
}

const uint32_t ivunt_vertexFormatSize = 44;

void TerrainRenderer::buildTerrainCellGrids()
{
	{
		std::vector<svec2> vertices;
		float increment = 256 / 4.0f;

		for (float x = 0; x < 256; x += increment)
		{
			for (float z = 0; z < 256; z += increment)
			{
				vertices.push_back({x, z});
				vertices.push_back({x, z + increment});
				vertices.push_back({x + increment, z + increment});
				vertices.push_back({x + increment, z});

				terrainCellMeshVertexCount += 4;
			}
		}

		StagingBuffer stagBuf = engine->renderer->createAndMapStagingBuffer(vertices.size() * sizeof(vertices[0]), vertices.data());
		terrainCellMesh = engine->renderer->createBuffer(vertices.size() * sizeof(vertices[0]), BUFFER_USAGE_VERTEX_BUFFER_BIT | BUFFER_USAGE_TRANSFER_DST_BIT, MEMORY_USAGE_GPU_ONLY, false);

		CommandBuffer cmdBuffer = engine->renderer->beginSingleTimeCommand(clipmapUpdateCommandPool);
		cmdBuffer->stageBuffer(stagBuf, terrainCellMesh);
		engine->renderer->endSingleTimeCommand(cmdBuffer, clipmapUpdateCommandPool, QUEUE_TYPE_GRAPHICS);

		engine->renderer->destroyStagingBuffer(stagBuf);
	}
}

void TerrainRenderer::createGraphicsPipeline ()
{
	ShaderModule vertShader = engine->renderer->createShaderModule(engine->getWorkingDir() + "GameData/shaders/vulkan/terrain.glsl", SHADER_STAGE_VERTEX_BIT);
	ShaderModule tessCtrlShader = engine->renderer->createShaderModule(engine->getWorkingDir() + "GameData/shaders/vulkan/terrain.glsl", SHADER_STAGE_TESSELLATION_CONTROL_BIT);
	ShaderModule tessEvalShader = engine->renderer->createShaderModule(engine->getWorkingDir() + "GameData/shaders/vulkan/terrain.glsl", SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
	ShaderModule fragShader = engine->renderer->createShaderModule(engine->getWorkingDir() + "GameData/shaders/vulkan/terrain.glsl", SHADER_STAGE_FRAGMENT_BIT);

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

	PipelineShaderStage tessCtrlShaderStage = {};
	tessCtrlShaderStage.entry = "main";
	tessCtrlShaderStage.module = tessCtrlShader;

	PipelineShaderStage tessEvalShaderStage = {};
	tessEvalShaderStage.entry = "main";
	tessEvalShaderStage.module = tessEvalShader;

	std::vector<VertexInputAttrib> attribDesc = std::vector<VertexInputAttrib>(1);
	attribDesc[0].binding = 0;
	attribDesc[0].location = 0;
	attribDesc[0].format = RESOURCE_FORMAT_R32G32_SFLOAT;
	attribDesc[0].offset = 0;

	PipelineVertexInputInfo vertexInput = {};
	vertexInput.vertexInputAttribs = attribDesc;
	vertexInput.vertexInputBindings =
	{	meshVertexBindingDesc};

	PipelineInputAssemblyInfo inputAssembly = {};
	inputAssembly.primitiveRestart = false;
	inputAssembly.topology = PRIMITIVE_TOPOLOGY_PATCH_LIST;

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

	PipelineTessellationInfo tessInfo = {};
	tessInfo.patchControlPoints = 4;

	PipelineInfo info = {};
	info.stages =
	{	vertShaderStage, tessCtrlShaderStage, tessEvalShaderStage, fragShaderStage};
	info.vertexInputInfo = vertexInput;
	info.inputAssemblyInfo = inputAssembly;
	info.viewportInfo = viewportInfo;
	info.rasterizationInfo = rastInfo;
	info.depthStencilInfo = depthInfo;
	info.colorBlendInfo = colorBlend;
	info.dynamicStateInfo = dynamicState;
	info.tessellationInfo = tessInfo;

	info.inputPushConstantRanges = {{0, pcSize, SHADER_STAGE_TESSELLATION_EVALUATION_BIT | SHADER_STAGE_TESSELLATION_CONTROL_BIT}};
	info.inputSetLayouts = {{
			{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_TESSELLATION_EVALUATION_BIT | SHADER_STAGE_FRAGMENT_BIT},
			{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_TESSELLATION_EVALUATION_BIT | SHADER_STAGE_FRAGMENT_BIT},
			{2, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
			{3, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 32, SHADER_STAGE_FRAGMENT_BIT}
	}};

	terrainPipeline = engine->renderer->createGraphicsPipeline(info, worldRenderer->gbufferRenderPass, 0);

	engine->renderer->destroyShaderModule(vertShaderStage.module);
	engine->renderer->destroyShaderModule(fragShaderStage.module);
	engine->renderer->destroyShaderModule(tessCtrlShaderStage.module);
	engine->renderer->destroyShaderModule(tessEvalShaderStage.module);
}

