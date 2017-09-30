/*
 * RenderGame.h
 *
 *  Created on: Sep 15, 2017
 *      Author: david
 */

#ifndef RENDERING_RENDERGAME_H_
#define RENDERING_RENDERGAME_H_

#include <common.h>
#include <Rendering/Renderer/Renderer.h>
#include <Resources/ResourceManager.h>

class RenderGame
{
	public:

		Renderer *renderer;
		ResourceManager *resources;

		RenderGame (Renderer *rendererBackend, ResourceManager *rendererResourceManager);
		virtual ~RenderGame ();

		void init ();
		void renderGame ();

	private:

		ResourceMesh testMesh;

		RenderPass testGBufferRenderPass;
		Pipeline defaultMaterialPipeline;
		DescriptorSet testMaterialDescriptorSet;
		Framebuffer testGBufferFramebuffer;

		CommandPool testGBufferCommandPool;
		CommandBuffer testGBufferCommandBuffer;

		Texture gbuffer_AlbedoRoughness;
		Texture gbuffer_NormalMetalness;
		Texture gbuffer_Depth;

		TextureView gbuffer_AlbedoRoughnessView;
		TextureView gbuffer_NormalMetalnessView;
		TextureView gbuffer_DepthView;

		Sampler gbufferSampler;

		void createTestCommandBuffer ();
		void createTestMesh ();
		void createGBuffer ();
		void createRenderPass ();
		void createGraphicsPipeline ();
		void createDescriptorSets ();
};

#endif /* RENDERING_RENDERERS_RENDERGAME_H_ */
