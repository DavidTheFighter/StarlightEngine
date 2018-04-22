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
 * TerrainRenderer.h
 * 
 * Created on: Oct 28, 2017
 *     Author: david
 */

#ifndef RENDERING_WORLD_TERRAINRENDERER_H_
#define RENDERING_WORLD_TERRAINRENDERER_H_

#include <common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>
#include <Resources/ResourceManager.h>

class StarlightEngine;
class WorldHandler;
class WorldRenderer;

class TerrainRenderer
{
	public:

		StarlightEngine *engine;
		WorldHandler *world;
		WorldRenderer *worldRenderer;

		DescriptorPool heightmapDescriptorPool;
		DescriptorSet heightmapDescriptorSet;

		bool clipmapUpdated;
		Semaphore clipmapUpdateSemaphore;
		CommandPool clipmapUpdateCommandPool;
		CommandBuffer clipmapUpdateCommandBuffer;

		Pipeline terrainPipeline;

		uint32_t terrainCellMeshVertexCount;
		Buffer terrainCellMesh;

		Texture transferClipmap_Elevation;
		Texture terrainClipmap_Elevation;

		TextureView terrainClipmapView_Elevation;

		Sampler terrainClipmapSampler;
		Sampler terrainTextureSampler;

		sivec2 clipmap0Regions[2][2];
		sivec2 clipmap1Regions[4][4];
		sivec2 clipmap2Regions[8][8];
		sivec2 clipmap3Regions[16][16];
		sivec2 clipmap4Regions[32][32];

		TerrainRenderer (StarlightEngine *enginePtr, WorldHandler *worldHandlerPtr, WorldRenderer *worldRendererPtr);
		virtual ~TerrainRenderer ();

		void update ();

		void renderTerrain (CommandBuffer &cmdBuffer);

		void init ();
		void destroy ();

	private:

		std::vector<StagingBuffer> clipmapStagingBuffersToDelete;

		void buildTerrainCellGrids ();
		void createGraphicsPipeline ();
};

#endif /* RENDERING_WORLD_TERRAINRENDERER_H_ */
