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
* FrameGraphRenderPass.cpp
*
* Created on: Jul 19, 2018
*     Author: david
*/

#include "Rendering/FrameGraphRenderPass.h"

FrameGraphRenderPass::FrameGraphRenderPass(const std::string &passName, uint32_t passPipelineType)
{
	nodeName = passName;
	pipelineType = passPipelineType;

	depthStencilOutput = std::make_pair<std::string, RenderPassAttachment>(std::string(""), {});
}

FrameGraphRenderPass::~FrameGraphRenderPass()
{

}

void FrameGraphRenderPass::addColorOutput(const std::string &name, const RenderPassAttachment &info, bool clearAttachment, ClearValue outputClearValue)
{
	colorOutputs.push_back(std::make_pair(name, info));

	colorOutputMipGenMethods[name] = RP_MIP_GEN_NONE;
	shouldClearColorOutputs[name] = clearAttachment;
	colorOutputClearValues[name] = outputClearValue;
	colorOutputLayerRenderMethods[name] = RP_LAYER_RENDER_IN_ONE_SUBPASS;
}

void FrameGraphRenderPass::setDepthStencilOutput(const std::string &name, const RenderPassAttachment &info, bool clearAttachment, ClearValue outputClearValue)
{
	depthStencilOutput = std::make_pair(name, info);
	shouldClearDepthStencil = clearAttachment;
	depthStencilClearValue = outputClearValue;
}

void FrameGraphRenderPass::addColorInput(const std::string &name)
{
	colorInputs.push_back(name);
}

void FrameGraphRenderPass::addDepthStencilInput(const std::string &name)
{
	depthStencilInputs.push_back(name);
}

void FrameGraphRenderPass::addColorInputAttachment(const std::string &name)
{
	colorInputAttachments.push_back(name);
}

// TODO Make sure color inputs attachments and depth input attachments have ordering guarantees, not just all colors then all depths (for shader input indexes)

void FrameGraphRenderPass::addDepthStencilInputAttachment(const std::string &name)
{
	depthStencilInputAttachments.push_back(name);
}

void FrameGraphRenderPass::setColorOutputMipGenMethod(const std::string &name, RenderPassAttachmentMipGenMethod mipGenMethod)
{
	colorOutputMipGenMethods[name] = mipGenMethod;
}

void FrameGraphRenderPass::setColorOutputRenderLayerMethod(const std::string &name, RenderPassAttachmentLayerRenderMethod method)
{
	colorOutputLayerRenderMethods[name] = method;
}

RenderPassAttachmentLayerRenderMethod FrameGraphRenderPass::getColorOutputRenderLayerMethod(const std::string &name)
{
	return colorOutputLayerRenderMethods[name];
}

void FrameGraphRenderPass::setRenderFunction(std::function<void(CommandBuffer, uint32_t)> renderFunc)
{
	renderFunction = renderFunc;
}

void FrameGraphRenderPass::setInitFunction(std::function<void(RenderPass, uint32_t)> windowCallback)
{
	initFunction = windowCallback;
}

void FrameGraphRenderPass::setDescriptorUpdateFunction(std::function<void(std::map<std::string, TextureView>, suvec3)> updateFunc)
{
	descriptorUpdateFunction = updateFunc;
}

std::string FrameGraphRenderPass::getNodeName()
{
	return nodeName;
}

uint32_t FrameGraphRenderPass::getPipelineType()
{
	return pipelineType;
}

std::function<void(CommandBuffer, uint32_t)> FrameGraphRenderPass::getRenderFunction()
{
	return renderFunction;
}

std::function<void(RenderPass, uint32_t)> FrameGraphRenderPass::getInitFunction()
{
	return initFunction;
}

std::function<void(std::map<std::string, TextureView>, suvec3)> FrameGraphRenderPass::getDescriptorUpdateFunction()
{
	return descriptorUpdateFunction;
}

bool FrameGraphRenderPass::hasDepthStencilOutput()
{
	return !depthStencilOutput.first.empty();
}

std::vector<std::pair<std::string, RenderPassAttachment>> FrameGraphRenderPass::getColorOutputs()
{
	return colorOutputs;
}

std::pair<std::string, RenderPassAttachment> FrameGraphRenderPass::getDepthStencilOutput()
{
	return depthStencilOutput;
}

const std::vector<std::string> &FrameGraphRenderPass::getColorInputs()
{
	return colorInputs;
}

const std::vector<std::string> &FrameGraphRenderPass::getColorInputAttachments()
{
	return colorInputAttachments;
}

const std::vector<std::string> &FrameGraphRenderPass::getDepthStencilInputs()
{
	return depthStencilInputs;
}

const std::vector<std::string> &FrameGraphRenderPass::getDepthStencilInputAttachments()
{
	return depthStencilInputAttachments;
}

ClearValue FrameGraphRenderPass::getColorOutputClearValue(const std::string &name)
{
	return colorOutputClearValues[name];
}

ClearValue FrameGraphRenderPass::getDepthStencilOutputClearValue()
{
	return depthStencilClearValue;
}

bool FrameGraphRenderPass::shouldClearColorOutput(const std::string &name)
{
	return shouldClearColorOutputs[name];
}

bool FrameGraphRenderPass::shouldClearDepthStencilOutput()
{
	return shouldClearDepthStencil;
}

RenderPassAttachmentMipGenMethod FrameGraphRenderPass::getColorOutputMipGenMethod(const std::string &name)
{
	if (colorOutputMipGenMethods.count(name) == 0)
		return RP_MIP_GEN_NONE;

	return colorOutputMipGenMethods[name];
}

RenderPassAttachment FrameGraphRenderPass::getOutputAttachment(const std::string &name)
{
	for (size_t i = 0; i < colorOutputs.size(); i++)
		if (colorOutputs[i].first == name)
			return colorOutputs[i].second;

	if (hasDepthStencilOutput() && depthStencilOutput.first == name)
		return depthStencilOutput.second;

	return {};
}