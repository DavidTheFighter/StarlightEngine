/*
 * Renderer.h
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#ifndef RENDERING_RENDERER_RENDERER_H_
#define RENDERING_RENDERER_RENDERER_H_

#include <common.h>
#include <Rendering/Renderer/RendererBackends.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

class Window;

typedef struct RendererAllocInfo
{
		RendererBackend backend;
		std::vector<std::string> launchArgs;
		Window* mainWindow;
} RendererAllocInfo;

class Renderer
{
	public:

		std::string workingDir;

		Renderer ();
		virtual ~Renderer ();

		/*
		 * Initializes the renderer.
		 */
		virtual void initRenderer () = 0;

		virtual CommandPool createCommandPool (QueueType queue, CommandPoolFlags flags) = 0;

		virtual void submitToQueue (QueueType queue, const std::vector<CommandBuffer> &cmdBuffers, const std::vector<Semaphore> &waitSemaphores = {}, const std::vector<PipelineStageFlags> &waitSemaphoreStages = {}, const std::vector<Semaphore> &signalSemaphores = {}, Fence fence = nullptr) = 0;
		virtual void waitForQueueIdle (QueueType queue) = 0;
		virtual void waitForDeviceIdle () = 0;

		virtual bool getFenceStatus (Fence fence) = 0;
		virtual void resetFence (Fence fence) = 0;
		virtual void resetFences (const std::vector<Fence> &fences) = 0;
		virtual void waitForFence (Fence fence, double timeoutInSeconds) = 0;
		virtual void waitForFences (const std::vector<Fence> &fences, bool waitForAll, double timeoutInSeconds) = 0;

		virtual void writeDescriptorSets (const std::vector<DescriptorWriteInfo> &writes) = 0;

		virtual RenderPass createRenderPass (const std::vector<AttachmentDescription> &attachments, const std::vector<SubpassDescription> &subpasses, const std::vector<SubpassDependency> &dependencies) = 0;
		virtual Framebuffer createFramebuffer (RenderPass renderPass, const std::vector<TextureView> &attachments, uint32_t width, uint32_t height, uint32_t layers = 1) = 0;
		virtual ShaderModule createShaderModule (std::string file, ShaderStageFlagBits stage) = 0;
		virtual PipelineInputLayout createPipelineInputLayout (const std::vector<PushConstantRange> &pushConstantRanges, const std::vector<std::vector<DescriptorSetLayoutBinding> > &setLayouts) = 0;
		virtual Pipeline createGraphicsPipeline (const PipelineInfo &pipelineInfo, PipelineInputLayout inputLayout, RenderPass renderPass, uint32_t subpass) = 0;
		virtual DescriptorPool createDescriptorPool (const std::vector<DescriptorSetLayoutBinding> &layoutBindings, uint32_t poolBlockAllocSize) = 0;

		virtual Fence createFence (bool createAsSignaled = false) = 0;
		virtual Semaphore createSemaphore () = 0;

		virtual Texture createTexture (svec3 extent, ResourceFormat format, TextureUsageFlags usage, MemoryUsage memUsage, bool ownMemory = false, uint32_t mipLevelCount = 1, TextureType type = TEXTURE_TYPE_2D) = 0;
		virtual TextureView createTextureView (Texture texture, TextureViewType viewType = TEXTURE_VIEW_TYPE_2D, TextureSubresourceRange subresourceRange = {0, 1, 0, 1}, ResourceFormat viewFormat = RESOURCE_FORMAT_UNDEFINED) = 0;
		virtual Sampler createSampler (SamplerAddressMode addressMode = SAMPLER_ADDRESS_MODE_REPEAT, SamplerFilter minFilter = SAMPLER_FILTER_LINEAR, SamplerFilter magFilter = SAMPLER_FILTER_LINEAR, float anisotropy = 1.0f, svec3 min_max_biasLod = {0, 0, 0}, SamplerMipmapMode mipmapMode =
				SAMPLER_MIPMAP_MODE_LINEAR) = 0;

		virtual Buffer createBuffer (size_t size, BufferUsageFlags usage, MemoryUsage memUsage, bool ownMemory = false) = 0;
		virtual void mapBuffer (Buffer buffer, size_t dataSize, const void *data) = 0;

		virtual StagingBuffer createStagingBuffer (size_t dataSize) = 0;
		virtual StagingBuffer createAndMapStagingBuffer (size_t dataSize, const void *data) = 0;
		virtual void mapStagingBuffer (StagingBuffer stagingBuffer, size_t dataSize, const void *data) = 0;

		virtual CommandBuffer beginSingleTimeCommand (CommandPool pool);
		virtual void endSingleTimeCommand (CommandBuffer cmdBuffer, CommandPool pool, QueueType queue);

		virtual void destroyCommandPool (CommandPool pool) = 0;
		virtual void destroyRenderPass (RenderPass renderPass) = 0;
		virtual void destroyFramebuffer (Framebuffer framebuffer) = 0;
		virtual void destroyPipelineInputLayout (PipelineInputLayout layout) = 0;
		virtual void destroyPipeline (Pipeline pipeline) = 0;
		virtual void destroyShaderModule (ShaderModule module) = 0;
		virtual void destroyDescriptorPool (DescriptorPool pool) = 0;
		virtual void destroyTexture (Texture texture) = 0;
		virtual void destroyTextureView (TextureView textureView) = 0;
		virtual void destroySampler (Sampler sampler) = 0;
		virtual void destroyBuffer (Buffer buffer) = 0;
		virtual void destroyStagingBuffer (StagingBuffer stagingBuffer) = 0;

		virtual void destroyFence (Fence fence) = 0;
		virtual void destroySemaphore (Semaphore sem) = 0;

#if SE_RENDER_DEBUG_MARKERS
		virtual void setObjectDebugName (void *obj, RendererObjectType objType, const std::string &name) = 0;
#else
		inline void setObjectDebugName (void *obj, RendererObjectType objType, const std::string &name) {};
#endif

		/*
		 * Initializes the swapchain. Note that this should only be called once, in the event you
		 * want to explicitly recreate the swapchain, call recreateSwapchain(). However in most cases
		 * you should just notify the renderer that the window was resized. That's is the preferred way to
		 * do it.
		 */
		virtual void initSwapchain (Window *wnd) = 0;
		virtual void presentToSwapchain (Window *wnd) = 0;
		virtual void recreateSwapchain (Window *wnd) = 0;
		virtual void setSwapchainTexture (Window *wnd, TextureView texView, Sampler sampler, TextureLayout layout) = 0;

		static RendererBackend chooseRendererBackend (const std::vector<std::string>& launchArgs);
		static Renderer* allocateRenderer (const RendererAllocInfo& allocInfo);
};

#endif /* RENDERING_RENDERER_H_ */
