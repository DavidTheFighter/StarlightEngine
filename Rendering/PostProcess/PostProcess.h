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
 * PostProcess.h
 *
 *  Created on: Apr 21, 2018
 *      Author: David
 */

#ifndef RENDERING_POSTPROCESS_POSTPROCESS_H_
#define RENDERING_POSTPROCESS_POSTPROCESS_H_

#include <common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>
#include <Resources/ResourceManager.h>

class StarlightEngine;

class PostProcess
{
	public:

		TextureView postprocessOutputTextureView;

		uint32_t cmdBufferIndex;
		std::vector<Semaphore> finalCombineSemaphores;

		PostProcess (StarlightEngine *enginePtr);
		virtual ~PostProcess ();

		void renderPostProcessing (Semaphore deferredLightingOutputSemaphore);

		void init ();
		void destroy ();

		void setInputs (TextureView gbuffer_AlbedoRoughnessTV, TextureView gbuffer_NormalsMetalnessTV, TextureView gbuffer_DepthTV, TextureView deferredLightingOutputTV, suvec2 gbufferSize);

	private:

		bool destroyed;

		StarlightEngine *engine;

		RenderPass postprocessRenderPass;
		Framebuffer postprocessFramebuffer;

		Pipeline combinePipeline;

		CommandPool postprocessCommandPool;
		std::vector<CommandBuffer> finalCombineCmdBuffers;

		DescriptorPool combineDescriptorPool;
		DescriptorSet combineDescriptorSet;

		Sampler inputSampler;

		Texture postprocessOutputTexture;

		TextureView gbuffer_AlbedoRoughness;
		TextureView gbuffer_NormalsMetalness;
		TextureView gbuffer_Depth;
		TextureView deferredLightingOutput;

		svec2 gbufferSize;

		void createCombinePipeline ();

		void createPostProcessRenderPass ();
};

#endif /* RENDERING_POSTPROCESS_POSTPROCESS_H_ */
