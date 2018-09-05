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

class Renderer;

class PostProcess
{
	public:


	PostProcess (Renderer *rendererPtr);
	virtual ~PostProcess ();

	void combineTonemapPassInit(RenderPass renderPass, uint32_t baseSubpass);
	void combineTonemapPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size);
	void combineTonemapPassRender(CommandBuffer cmdBuffer, uint32_t counter);

	private:

	Renderer *renderer;

	Pipeline combinePipeline;

	DescriptorPool combineDescriptorPool;
	DescriptorSet combineDescriptorSet;

	Sampler inputSampler;

	suvec3 renderSize;

	void createCombinePipeline(RenderPass renderPass, uint32_t baseSubpass);
};

#endif /* RENDERING_POSTPROCESS_POSTPROCESS_H_ */
