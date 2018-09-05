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
* FrameGraph.h
*
* Created on: Jul 17, 2018
*     Author: david
*/

#ifndef RENDERING_FRAMEGRAPH_H_
#define RENDERING_FRAMEGRAPH_H_

#include <common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>
#include <Rendering/FrameGraphRenderPass.h>

typedef enum
{
	FG_PIPELINE_GRAPHICS = 0,
	FG_PIPELINE_COMPUTE = 1,
	FG_PIPELINE_TRANSFER = 2,
	FG_PIPELINE_MAX_ENUM
} FrameGraphPipelineType;

typedef struct
{
	RenderPassAttachment base;
	Texture attTex;
	std::vector<suvec2> usageLifetimes; // A list of periods in the pass stack that this is being used in, used for aliasing

} FrameGraphTexture;

typedef enum
{
	FG_OPCODE_BEGIN_RENDER_PASS,
	FG_OPCODE_END_RENDER_PASS,
	FG_OPCODE_NEXT_SUBPASS,
	FG_OPCODE_POST_BLIT,
	FG_OPCODE_CALL_RENDER_FUNC,
	FG_OPCODE_MAX_ENUM
} FrameGraphOpCode;

typedef struct
{
	RenderPass renderPass;
	Framebuffer framebuffer;
	suvec3 size; // {width, height, arrayLayers}
	std::vector<ClearValue> clearValues;
	SubpassContents subpassContents;

} FrameGraphRenderPassData;

typedef struct
{
	std::string attachmentName;
} PostBlitOpData;

typedef struct
{
	size_t passIndex;
	uint32_t counter;
} CallRenderFuncOpData;

class Renderer;

class FrameGraph
{
	public:

	FrameGraph(Renderer *rendererPtr, suvec2 swapchainDimensions);
	virtual ~FrameGraph();

	void setBackbufferSource(const std::string &name);
	FrameGraphRenderPass &addRenderPass(const std::string &name, FrameGraphPipelineType pipelineType);

	/*
	Validates the render passes currently specified and spits out any errors found. Will throw exceptions if an error is found.
	*/
	void validate();

	/*
	Builds/compiles/bakes the frame graph.
	*/
	void build();

	void execute();

	TextureView getBackbufferSourceTextureView();

	private:

	Renderer *renderer;
	suvec2 swapchainSize;

	uint32_t execCounter; // A counter incremented every time the execute() method is called, used for n-buffering of the command buffers

	std::string backbufferSource;
	std::vector<std::unique_ptr<FrameGraphRenderPass>> passes;

	std::vector<FrameGraphTexture> graphTextures; // A list of all physical resources used by the graph, note that some of these could be aliased multiple times
	std::map<std::string, std::pair<TextureView, size_t>> attachmentViews; // A list of views that each attachment, by name, gets that indexes to a texture in "graphTextures"

	std::vector<std::pair<FrameGraphOpCode, void*>> execCodes;

	std::vector<std::unique_ptr<FrameGraphRenderPassData>> beginRenderPassOpsData;
	std::vector<std::unique_ptr<PostBlitOpData>> postBlitOpsData;
	std::vector<std::unique_ptr<CallRenderFuncOpData>> callRenderFuncOpsData;

	std::map<std::string, RenderPassAttachment> allAttachments;
	std::map<std::string, TextureUsageFlags> attachmentUsages;
	std::map<std::string, suvec2> attachmentLifetimes;

	std::vector<CommandPool> gfxCommandPools;

	/*
	Creates actual render passes and the execution code for when it becomes time to actually render.
	*/
	void buildRenderPassesAndExecCodes(const std::vector<size_t> &passStack);

	/*
	Goes through a pass stack and allocates physical resources, also does aliasing.
	*/
	void assignPhysicalResources(const std::vector<size_t> &passStack);

	/*
	Attempts to merge any passes together that can be merged into one render pass, returns a vector that has each old pass
	per new pass.
	*/
	std::vector<size_t> optimizePassOrder(const std::vector<size_t> &passStack);

	/*
	Adds all passes that writes to a resource, then recursively adds the passes that write to those passes inputs, etc, etc, 
	basically traveres the dependency graph. Note it does not check for cyclic dependencies, so check beforehand for those.
	*/
	void recursiveFindWritePasses(const std::string &name, std::vector<size_t> &passStack);

	// Gets a list of all passes that contain a resource as one of their outputs
	std::vector<size_t> getPassesThatWriteToResource(const std::string &name);

	bool passAttachmentsHaveSameSize(const RenderPassAttachment& att0, const RenderPassAttachment &att1);

	bool checkIsMergeValid(size_t pass, size_t writePass);
	bool attachmentContainsPassInDependencyChain(const std::string &name, size_t pass);
	bool passContainsPassInDependencyChain(size_t pass, size_t writePass);
	bool inputAttachmentHasInvalidSubpassChainToPass(const std::string &name, size_t pass);
};

#endif /* RENDERING_FRAMEGRAPH_H_ */