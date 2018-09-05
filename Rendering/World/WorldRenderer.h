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
 * WorldRenderer.h
 * 
 * Created on: Oct 10, 2017
 *     Author: david
 */

#ifndef RENDERING_WORLD_WORLDRENDERER_H_
#define RENDERING_WORLD_WORLDRENDERER_H_

#define STATIC_OBJECT_STREAMING_BUFFER_SIZE (4 * 1024 * 1024)

#include <common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>
#include <World/SortedOctree.h>

#include <Rendering/World/CSM.h>

class WorldHandler;
class StarlightEngine;
class TerrainRenderer;
class TerrainShadowRenderer;

struct LevelStaticObject;
struct LevelStaticObjectType;

typedef std::map<size_t, std::map<size_t, std::vector<std::vector<LevelStaticObject> > > > LevelStaticObjectStreamingDataHierarchy;

typedef struct LevelStaticObjectStreamingData
{
		/*
		 * The data that will be rendered. It's a little complex and long winded, and might change.
		 *
		 * In order, it goes:
		 * 						map of material hashes
		 * 							map of mesh hashes
		 * 								vector of mesh lods (size() == mesh lod number, always)
		 * 									vector of objs using this data
		 */
		LevelStaticObjectStreamingDataHierarchy data;
} LevelStaticObjectStreamingData;

class WorldRenderer
{
	public:

	Sampler testSampler;

	CSM *sunCSM;

	StarlightEngine *engine;
	WorldHandler *world;

	TerrainRenderer *terrainRenderer;
	TerrainShadowRenderer *terrainShadowRenderer;

	RenderPass gbufferRenderPass;
	RenderPass shadowsRenderPass;

	WorldRenderer (StarlightEngine *enginePtr, WorldHandler *worldHandlerPtr);
	virtual ~WorldRenderer ();

	void update ();

	void renderWorldStaticMeshes (CommandBuffer &cmdBuffer, glm::mat4 mvp, bool renderDepth);

	void gbufferPassInit(RenderPass renderPass, uint32_t baseSubpass);
	void gbufferPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size);
	void gbufferPassRender(CommandBuffer cmdBuffer, uint32_t counter);

	void sunShadowsPassInit(RenderPass renderPass, uint32_t baseSubpass);
	void sunShadowsPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size);
	void sunShadowsPassRender(CommandBuffer cmdBuffer, uint32_t counter);

	private:

	suvec3 gbufferRenderSize;
	suvec3 sunShadowsRenderSize;

	Pipeline physxDebugPipeline;

	void traverseOctreeNode (SortedOctree<LevelStaticObjectType, LevelStaticObject> &node, LevelStaticObjectStreamingData &data, const glm::vec4 (&frustum)[6]);

	void createPipelines(RenderPass renderPass, uint32_t baseSubpass);

	LevelStaticObjectStreamingData getStaticObjStreamingData (const glm::vec4 (&frustum)[6]);

	size_t worldStreamingBufferOffset;
	Buffer worldStreamingBuffer;
	Buffer physxDebugStreamingBuffer;
	void *worldStreamingBufferData;
};

#endif /* RENDERING_WORLD_WORLDRENDERER_H_ */
