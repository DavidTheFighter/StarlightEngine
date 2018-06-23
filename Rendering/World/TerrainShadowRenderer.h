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
* TerrainShadowRenderer.h
*
* Created on: Jun 22, 2018
*     Author: david
*/

#ifndef RENDERING_WORLD_TERRAINSHADOWRENDERER_H_
#define RENDERING_WORLD_TERRAINSHADOWRENDERER_H_

#include <common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

class Renderer;

class TerrainShadowRenderer
{
	public:

	TextureView terrainDepthsView;

	std::string temp_workingDir;

	TerrainShadowRenderer(Renderer *rendererPtr);
	virtual ~TerrainShadowRenderer();

	void init();
	void destroy();

	void renderTerrainShadowmap(CommandBuffer &cmdBuffer, glm::vec3 lightDir);
	void setClipmap(TextureView clipmap, Sampler clipmapSampler);

	private:

	CommandPool terrainShadowCmdPool;

	bool destroyed;

	Renderer *renderer;

	Pipeline depthPipeline;

	RenderPass depthRenderPass;

	uint32_t shadowTerrainMeshVertexCount;
	Buffer shadowTerrainMesh;

	Sampler depthmapSampler;

	DescriptorPool depthClipmapDescriptorPool;
	DescriptorSet depthClipmapDescSet;

	TextureView clipmapTexView;

	Texture terrainDepths;
	TextureView terrainDepthSlicesView[5];
	Framebuffer terrainDepthsFB[5];

	void createTerrainMesh();

	void createDepthPipeline();
	void createDepthRenderPass();
	
};

#endif /* RENDERING_WORLD_TERRAINSHADOWRENDERER_H_ */