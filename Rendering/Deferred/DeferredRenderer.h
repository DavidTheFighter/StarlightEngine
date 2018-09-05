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
 * DeferredRenderer.h
 * 
 * Created on: Mar 24, 2018
 *     Author: david
 */

#ifndef RENDERING_DEFERRED_DEFERREDRENDERER_H_
#define RENDERING_DEFERRED_DEFERREDRENDERER_H_

#include <common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>
#include <Resources/ResourceManager.h>

class StarlightEngine;
class AtmosphereRenderer;
class Game;

typedef struct
{
	glm::mat4 invCamMVPMat;
	glm::vec3 cameraPosition;
	float padding0;
	glm::vec4 prjMat_aspectRatio_tanHalfFOV;

} DeferredRendererLightingPushConsts;

class DeferredRenderer
{
	public:

		Game *game;

		DeferredRenderer (StarlightEngine *enginePtr, AtmosphereRenderer *atmospherePtr);
		virtual ~DeferredRenderer ();

		void lightingPassInit(RenderPass renderPass, uint32_t baseSubpass);
		void lightingPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size);
		void lightingPassRender(CommandBuffer cmdBuffer, uint32_t counter);

	private:

		StarlightEngine *engine;
		AtmosphereRenderer *atmosphere;

		Pipeline deferredPipeline;

		DescriptorPool deferredInputsDescriptorPool;
		DescriptorSet deferredInputDescriptorSet;

		Sampler linearSampler;
		Sampler atmosphereTextureSampler;
		Sampler shadowsSampler;
		Sampler ditherSampler;

		ResourceTexture brdfLUT;

		suvec3 renderSize;

		void createDeferredLightingPipeline (RenderPass renderPass, uint32_t baseSubpass);
};

#endif /* RENDERING_DEFERRED_DEFERREDRENDERER_H_ */
