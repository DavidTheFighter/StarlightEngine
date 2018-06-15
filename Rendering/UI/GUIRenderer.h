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
 * GUIRenderer.h
 * 
 * Created on: Oct 1, 2017
 *     Author: david
 */

#ifndef RENDERING_UI_GUIRENDERER_H_
#define RENDERING_UI_GUIRENDERER_H_

#include <common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>
#include <Resources/Resources.h>

struct nk_vertex
{
		svec2 vertex;
		svec2 uv;
		svec4 color;
};

class Renderer;
class StarlightEngine;

class GUIRenderer
{
	public:

		CommandPool testGUICommandPool;

		StarlightEngine *temp_engine;

		Sampler testSampler;

		Texture fontAtlas;
		TextureView fontAtlasView;
		DescriptorSet fontAtlasDescriptor;

		DescriptorPool guiTextureDescriptorPool;
		ResourceTexture whiteTexture;
		TextureView whiteTextureView;
		DescriptorSet whiteTextureDescriptor;

		Buffer guiVertexStreamBuffer;
		Buffer guiIndexStreamBuffer;

		GUIRenderer (Renderer *engineRenderer);
		virtual ~GUIRenderer ();

		void init (struct nk_context &ctx);
		void destroy ();

		void writeTestGUI (struct nk_context &context);
		void recordGUIRenderCommandList (CommandBuffer cmdBuffer, struct nk_context &nkctx, uint32_t renderWidth, uint32_t renderHeight);

	private:

		Renderer *renderer;

		Pipeline guiGraphicsPipeline;

		void createGraphicsPipeline ();
};

#endif /* RENDERING_UI_GUIRENDERER_H_ */
