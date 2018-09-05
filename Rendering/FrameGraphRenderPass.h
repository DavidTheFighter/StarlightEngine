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
* FrameGraphRenderPass.h
*
* Created on: Jul 17, 2018
*     Author: david
*/

#ifndef RENDERING_FRAMEGRAPHRENDERPASS_H_
#define RENDERING_FRAMEGRAPHRENDERPASS_H_

#include <common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

typedef struct RenderPassAttachment
{
	bool sizeSwapchainRelative = true; // Makes the attachment have the same size as the window, and the size members now scale that window size for this attachment
	ResourceFormat format;
	float sizeX = 1;
	float sizeY = 1;
	float sizeZ = 1;
	uint32_t mipLevels = 1;
	uint32_t arrayLayers = 1;
	TextureViewType viewType = TEXTURE_VIEW_TYPE_2D;

	bool operator==(const RenderPassAttachment &other) const
	{
		return sizeSwapchainRelative == other.sizeSwapchainRelative && format == other.format && sizeX == other.sizeX && sizeY == other.sizeY && sizeZ == other.sizeZ && mipLevels == other.mipLevels && arrayLayers == other.arrayLayers && viewType == other.viewType;
	}

	bool operator<(const RenderPassAttachment &other) const
	{
		return sizeSwapchainRelative < other.sizeSwapchainRelative || format < other.format || sizeX < other.sizeX || sizeY < other.sizeY || sizeZ < other.sizeZ || mipLevels < other.mipLevels || arrayLayers < other.arrayLayers || viewType < other.viewType;
	}

} RenderPassAttachment;

typedef enum
{
	RP_MIP_GEN_POST_BLIT = 0,
	RP_MIP_GEN_NONE = 1,
	RP_MIP_GEN_MAX_ENUM
} RenderPassAttachmentMipGenMethod;

typedef enum
{
	RP_LAYER_RENDER_IN_ONE_SUBPASS = 0,
	RP_LAYER_RENDER_IN_MULTIPLE_SUBPASSES = 1,
	RP_LAYER_RENDER_IN_MULTIPLE_RENDERPASSES = 2,
	RP_LAYER_RENDER_MAX_ENUM
} RenderPassAttachmentLayerRenderMethod;

class FrameGraphRenderPass
{
	public:

	FrameGraphRenderPass(const std::string &passName, uint32_t passPipelineType);
	virtual ~FrameGraphRenderPass();

	void addColorOutput(const std::string &name, const RenderPassAttachment &info, bool clearAttachment = true, ClearValue outputClearValue = {0, 0, 0, 0});
	void setDepthStencilOutput(const std::string &name, const RenderPassAttachment &info, bool clearAttachment = true, ClearValue outputClearValue = {1, 0});

	/*
	Adds a color input to this render pass. By using this method, you're stating that this render pass can sample anywhere in the input texture. If you only need it as a
	subpass input attachment (only need to sample one texel corresponding to the texel you're rendering, google it), then use the addColorInputAttachment(..)
	*/
	void addColorInput(const std::string &name);

	/*
	Adds a depth/stencil input to this render pass. By using this method, you're stating that this render pass can sample anywhere in the input texture. If you only need it as a
	subpass input attachment (only need to sample one texel corresponding to the texel you're rendering, google it), then use the addDepthStencilInput(..)
	*/
	void addDepthStencilInput(const std::string &name);

	/*
	Adds a color input attachment to this render pass. Different than a color input, in that you can only sample one texel corresponding to the fragment you're rendering. Also known
	as a subpass input attachment. If you need to sample more than one pixel or in a different location, such as a blur, then use the addColorInput(..)
	*/
	void addColorInputAttachment(const std::string &name);

	/*
	Adds a depth/stencil input attachment to this render pass. Different than a depth/stencil input, in that you can only sample one texel corresponding to the fragment you're rendering. Also known
	as a subpass input attachment. If you need to sample more than one pixel or in a different location, such as a blur, then use the addDepthStencilInput(..)
	*/
	void addDepthStencilInputAttachment(const std::string &name);

	/*
	Sets how the mipmap generation of a particular color output should happen, by default RP_MIP_GEN_NONE. You can specify to allow automagic generation after the pass if finished using
	blits (RP_MIP_GEN_POST_BLIT).
	*/
	void setColorOutputMipGenMethod(const std::string &name, RenderPassAttachmentMipGenMethod mipGenMethod);
	RenderPassAttachmentMipGenMethod getColorOutputMipGenMethod(const std::string &name);

	/*
	Specifies whether the rendering of a color attachment with multiple array layers should happen all in one subpass or in multiple subpasses (one per layer). By default it happens
	all in one.
	*/
	void setColorOutputRenderLayerMethod(const std::string &name, RenderPassAttachmentLayerRenderMethod method);
	RenderPassAttachmentLayerRenderMethod getColorOutputRenderLayerMethod(const std::string &name);

	ClearValue getColorOutputClearValue(const std::string &name);
	ClearValue getDepthStencilOutputClearValue();

	bool shouldClearColorOutput(const std::string &name);
	bool shouldClearDepthStencilOutput();

	void setInitFunction(std::function<void(RenderPass, uint32_t)> callbackFunc);
	void setDescriptorUpdateFunction(std::function<void(std::map<std::string, TextureView>, suvec3)> updateFunc);
	void setRenderFunction(std::function<void(CommandBuffer, uint32_t)> renderFunc);

	std::string getNodeName();
	uint32_t getPipelineType();

	std::function<void(CommandBuffer, uint32_t)> getRenderFunction();
	std::function<void(RenderPass, uint32_t)> getInitFunction();
	std::function<void(std::map<std::string, TextureView>, suvec3)> getDescriptorUpdateFunction();

	bool hasDepthStencilOutput();

	std::vector<std::pair<std::string, RenderPassAttachment>> getColorOutputs();
	std::pair<std::string, RenderPassAttachment> getDepthStencilOutput();

	const std::vector<std::string> &getColorInputs();
	const std::vector<std::string> &getColorInputAttachments();
	const std::vector<std::string> &getDepthStencilInputs();
	const std::vector<std::string> &getDepthStencilInputAttachments();

	RenderPassAttachment getOutputAttachment(const std::string &name);

	private:

	std::string nodeName;
	uint32_t pipelineType;

	ClearValue depthStencilClearValue;
	std::map<std::string, ClearValue> colorOutputClearValues;

	bool shouldClearDepthStencil;
	std::map<std::string, bool> shouldClearColorOutputs;

	std::vector<std::pair<std::string, RenderPassAttachment>> colorOutputs;
	std::pair<std::string, RenderPassAttachment> depthStencilOutput;
	std::vector<std::string> colorInputs;
	std::vector<std::string> depthStencilInputs;
	std::vector<std::string> colorInputAttachments;
	std::vector<std::string> depthStencilInputAttachments;

	std::map<std::string, RenderPassAttachmentMipGenMethod> colorOutputMipGenMethods;
	std::map<std::string, RenderPassAttachmentLayerRenderMethod> colorOutputLayerRenderMethods;

	std::function<void(CommandBuffer, uint32_t)> renderFunction;
	std::function<void(RenderPass, uint32_t)> initFunction;
	std::function<void(std::map<std::string, TextureView>, suvec3)> descriptorUpdateFunction;
};

#endif /* RENDERING_FRAMEGRAPHRENDERPASS_H_ */