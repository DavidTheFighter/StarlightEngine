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
* FrameGraph.cpp
*
* Created on: Jul 17, 2018
*     Author: david
*/

#include "Rendering/FrameGraph.h"

#include <Rendering/Renderer/Renderer.h>

const std::string viewSelectMipPostfix = "_fg_miplvl";
const std::string viewSelectLayerPostfix = "_fg_layer";

FrameGraph::FrameGraph(Renderer *rendererPtr, suvec2 swapchainDimensions)
{
	backbufferSource = "";
	renderer = rendererPtr;
	swapchainSize = swapchainDimensions;
	execCounter = 0;

	const uint32_t bufferCount = 3;
	for (uint32_t i = 0; i < bufferCount; i++)
		gfxCommandPools.push_back(renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, COMMAND_POOL_TRANSIENT_BIT));
}

FrameGraph::~FrameGraph()
{
	for (size_t i = 0; i < gfxCommandPools.size(); i++)
		renderer->destroyCommandPool(gfxCommandPools[i]);

	for (size_t i = 0; i < graphTextures.size(); i++)
		renderer->destroyTexture(graphTextures[i].attTex);

	for (std::pair<std::string, std::pair<TextureView, size_t>> attView : attachmentViews)
		renderer->destroyTextureView(attView.second.first);

	for (size_t i = 0; i < beginRenderPassOpsData.size(); i++)
	{
		renderer->destroyFramebuffer(beginRenderPassOpsData[i]->framebuffer);
		renderer->destroyRenderPass(beginRenderPassOpsData[i]->renderPass);
	}
}

void FrameGraph::build()
{
	validate();

	std::vector<size_t> passStack;

	auto sT = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch());
	recursiveFindWritePasses(backbufferSource, passStack);
	printf("Recursive took: %fms\n", (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()) - sT).count() / 1000.0f);

	std::reverse(passStack.begin(), passStack.end());

	printf("\nInitial pass stack:\n");
	for (size_t i = 0; i < passStack.size(); i++)
	{
		printf("%s\n", passes[passStack[i]]->getNodeName().c_str());
	}
	printf("\n");

	sT = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch());
	passStack = optimizePassOrder(passStack);
	printf("Optimize took: %fms\n", (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()) - sT).count() / 1000.0f);

	printf("Optimized pass stack:\n");
	for (size_t i = 0; i < passStack.size(); i++)
	{
		printf("%s\n", passes[passStack[i]]->getNodeName().c_str());
	}
	printf("\n");

	sT = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch());
	assignPhysicalResources(passStack);
	printf("Assign took: %fms\n", (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()) - sT).count() / 1000.0f);

	sT = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch());
	buildRenderPassesAndExecCodes(passStack);
	printf("Build took: %fms\n", (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()) - sT).count() / 1000.0f);

	std::map<std::string, TextureView> graphViewsMap;

	for (auto it = attachmentViews.begin(); it != attachmentViews.end(); it++)
		graphViewsMap[it->first] = it->second.first;

	for (size_t p = 0; p < passes.size(); p++)
	{
		RenderPassAttachment exPassAtt = {};

		if (passes[p]->hasDepthStencilOutput())
			exPassAtt = passes[p]->getDepthStencilOutput().second;
		else
			exPassAtt = passes[p]->getColorOutputs()[0].second;

		suvec3 passSize = {exPassAtt.sizeSwapchainRelative ? uint32_t(swapchainSize.x * exPassAtt.sizeX) : (uint32_t) exPassAtt.sizeX, exPassAtt.sizeSwapchainRelative ? uint32_t(swapchainSize.y * exPassAtt.sizeY) : (uint32_t) exPassAtt.sizeY, std::max<uint32_t>(exPassAtt.sizeZ, exPassAtt.arrayLayers)};

		passes[p]->getDescriptorUpdateFunction()(graphViewsMap, passSize);
	}
}

void FrameGraph::execute()
{
	CommandBuffer cmdBuffer = gfxCommandPools[execCounter]->allocateCommandBuffer();
	cmdBuffer->beginCommands(COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	for (size_t i = 0; i < execCodes.size(); i++)
	{
		FrameGraphOpCode opCode = execCodes[i].first;
		void *opDataPtr = execCodes[i].second;

		switch (opCode)
		{
			case FG_OPCODE_BEGIN_RENDER_PASS:
			{
				const FrameGraphRenderPassData &opData = *static_cast<FrameGraphRenderPassData*>(opDataPtr);
				cmdBuffer->beginRenderPass(opData.renderPass, opData.framebuffer, {0, 0, opData.size.x, opData.size.y}, opData.clearValues, opData.subpassContents);
				cmdBuffer->setScissors(0, {{0, 0, opData.size.x, opData.size.y}});
				cmdBuffer->setViewports(0, {{0, 0, (float) opData.size.x, (float) opData.size.y, 0, 1}});

				break;
			}
			case FG_OPCODE_END_RENDER_PASS:
			{
				cmdBuffer->endRenderPass();

				break;
			}
			case FG_OPCODE_NEXT_SUBPASS:
			{
				cmdBuffer->nextSubpass(SUBPASS_CONTENTS_INLINE);

				break;
			}
			case FG_OPCODE_POST_BLIT:
			{
				const PostBlitOpData &opData = *static_cast<PostBlitOpData*>(opDataPtr);
				FrameGraphTexture graphTex = graphTextures[attachmentViews[opData.attachmentName].second];
				
				uint32_t sizeX = graphTex.base.sizeSwapchainRelative ? uint32_t(swapchainSize.x * graphTex.base.sizeX) : (uint32_t) graphTex.base.sizeX;
				uint32_t sizeY = graphTex.base.sizeSwapchainRelative ? uint32_t(swapchainSize.y * graphTex.base.sizeX) : (uint32_t) graphTex.base.sizeY;
				uint32_t sizeZ = (uint32_t) graphTex.base.sizeZ;

				cmdBuffer->setTextureLayout(graphTex.attTex, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, graphTex.base.arrayLayers}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
				
				for (uint32_t m = 1; m < graphTex.base.mipLevels; m++)
				{
					TextureBlitInfo blitInfo = {};
					blitInfo.srcSubresource = {m - 1, 0, graphTex.base.arrayLayers};
					blitInfo.dstSubresource = {m, 0, graphTex.base.arrayLayers};
					blitInfo.srcOffsets[0] = {0, 0, 0};
					blitInfo.dstOffsets[0] = {0, 0, 0};
					blitInfo.srcOffsets[1] = {std::max(int32_t(sizeX >> (m - 1)), 1), std::max(int32_t(sizeY >> (m - 1)), 1), std::max(int32_t(sizeZ >> (m - 1)), 1)};
					blitInfo.dstOffsets[1] = {std::max(int32_t(sizeX >> m), 1), std::max(int32_t(sizeY >> m), 1), std::max(int32_t(sizeZ >> m), 1)};

					cmdBuffer->setTextureLayout(graphTex.attTex, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {m, 1, 0, graphTex.base.arrayLayers}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
					cmdBuffer->blitTexture(graphTex.attTex, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, graphTex.attTex, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {blitInfo}, SAMPLER_FILTER_LINEAR);
					cmdBuffer->setTextureLayout(graphTex.attTex, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, {m, 1, 0, graphTex.base.arrayLayers}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
				}

				cmdBuffer->setTextureLayout(graphTex.attTex, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, graphTex.base.mipLevels, 0, graphTex.base.arrayLayers}, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);

				break;
			}
			case FG_OPCODE_CALL_RENDER_FUNC:
			{
				const CallRenderFuncOpData &opData = *static_cast<CallRenderFuncOpData*>(opDataPtr);

				cmdBuffer->beginDebugRegion(passes[opData.passIndex]->getNodeName(), glm::vec4(0, 0.5f, 1, 1));
				passes[opData.passIndex]->getRenderFunction()(cmdBuffer, opData.counter);
				cmdBuffer->endDebugRegion();

				break;
			}
		}
	}

	cmdBuffer->endCommands();

	std::vector<Semaphore> waitSems = {};
	std::vector<PipelineStageFlags> waitSemStages = {};

	renderer->submitToQueue(QUEUE_TYPE_GRAPHICS, {cmdBuffer}, waitSems, waitSemStages, {});
	renderer->waitForQueueIdle(QUEUE_TYPE_GRAPHICS);

	gfxCommandPools[execCounter]->freeCommandBuffer(cmdBuffer);

	execCounter++;
	execCounter %= uint32_t(gfxCommandPools.size());
}

void FrameGraph::buildRenderPassesAndExecCodes(const std::vector<size_t> &passStack)
{
	std::map<std::string, size_t> currentRPAttachmentsMap;
	std::vector<AttachmentDescription> currentRPAttachments;
	std::vector<ClearValue> currentRPAttClearValues;
	std::map<std::string, std::unique_ptr<AttachmentReference>> currentRPDepthStencilReferences;
	std::vector<SubpassDescription> currentRPSubpasses;
	std::vector<SubpassDependency> currentRPSubpassDependencies;
	std::vector<size_t> currentRPPassIndices;
	std::vector<size_t> currentRPPassSubpassCount;

	bool buildingRenderPass = false;
	std::vector<std::pair<FrameGraphOpCode, void*>> currentRPOpCodes;

	for (size_t p = 0; p < passStack.size(); p++)
	{
		FrameGraphRenderPass &pass = *passes[passStack[p]];

		printf("on pass: %s\n", pass.getNodeName().c_str());

		if (pass.getPipelineType() == FG_PIPELINE_GRAPHICS)
		{
			bool buildingLayersInSubpasses = false, buildingLayersInMultiRenderPasses = false;
			uint32_t renderLayerCount = 1;

			if (pass.getColorOutputs().size() > 0)
			{
				if (pass.getColorOutputRenderLayerMethod(pass.getColorOutputs()[0].first) == RP_LAYER_RENDER_IN_MULTIPLE_SUBPASSES)
				{
					buildingLayersInSubpasses = true;
					renderLayerCount = pass.getColorOutputs()[0].second.arrayLayers;
				}
				else if (pass.getColorOutputRenderLayerMethod(pass.getColorOutputs()[0].first) == RP_LAYER_RENDER_IN_MULTIPLE_RENDERPASSES)
				{
					buildingLayersInMultiRenderPasses = true;
					renderLayerCount = pass.getColorOutputs()[0].second.arrayLayers;
				}
			}

			if (pass.hasDepthStencilOutput())
			{
				if (pass.getColorOutputRenderLayerMethod(pass.getDepthStencilOutput().first) == RP_LAYER_RENDER_IN_MULTIPLE_SUBPASSES)
				{
					buildingLayersInSubpasses = true;
					renderLayerCount = pass.getDepthStencilOutput().second.arrayLayers;
				}
				else if (pass.getColorOutputRenderLayerMethod(pass.getDepthStencilOutput().first) == RP_LAYER_RENDER_IN_MULTIPLE_RENDERPASSES)
				{
					buildingLayersInMultiRenderPasses = true;
					renderLayerCount = pass.getDepthStencilOutput().second.arrayLayers;
				}
			}

			for (uint32_t l = 0; l < renderLayerCount; l++)
			{
				if (buildingLayersInMultiRenderPasses || l == 0)
				{
					currentRPPassSubpassCount.push_back(buildingLayersInSubpasses ? renderLayerCount : 1);
					currentRPPassIndices.push_back(passStack[p]);
				}

				if (!buildingRenderPass)
				{				
					for (size_t i = 0; i < pass.getColorInputAttachments().size(); i++)
					{
						std::string attName = pass.getColorInputAttachments()[i];

						AttachmentDescription attachmentDesc = {};
						attachmentDesc.format = allAttachments[attName].format;
						attachmentDesc.initialLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						attachmentDesc.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						attachmentDesc.loadOp = ATTACHMENT_LOAD_OP_LOAD;
						attachmentDesc.storeOp = ATTACHMENT_STORE_OP_STORE;
						
						currentRPAttachmentsMap[attName] = currentRPAttachments.size();
						currentRPAttachments.push_back(attachmentDesc);
						currentRPAttClearValues.push_back({0, 0, 0, 0});
					}

					for (size_t i = 0; i < pass.getDepthStencilInputAttachments().size(); i++)
					{
						std::string attName = pass.getDepthStencilInputAttachments()[i];

						AttachmentDescription attachmentDesc = {};
						attachmentDesc.format = allAttachments[attName].format;
						attachmentDesc.initialLayout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
						attachmentDesc.finalLayout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
						attachmentDesc.loadOp = ATTACHMENT_LOAD_OP_LOAD;
						attachmentDesc.storeOp = ATTACHMENT_STORE_OP_STORE;

						currentRPAttachmentsMap[attName] = currentRPAttachments.size();
						currentRPAttachments.push_back(attachmentDesc);
						currentRPAttClearValues.push_back({0, 0, 0, 0});
					}

					buildingRenderPass = true;
				}
				else
				{
					for (size_t i = 0; i < pass.getColorInputAttachments().size(); i++)
					{
						std::string attName = pass.getColorInputAttachments()[i];

						if (currentRPAttachmentsMap.count(attName) == 0)
						{
							AttachmentDescription attachmentDesc = {};
							attachmentDesc.format = allAttachments[attName].format;
							attachmentDesc.initialLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
							attachmentDesc.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
							attachmentDesc.loadOp = ATTACHMENT_LOAD_OP_LOAD;
							attachmentDesc.storeOp = ATTACHMENT_STORE_OP_STORE;

							currentRPAttachmentsMap[attName] = currentRPAttachments.size();
							currentRPAttachments.push_back(attachmentDesc);
							currentRPAttClearValues.push_back({0, 0, 0, 0});
						}
					}

					for (size_t i = 0; i < pass.getDepthStencilInputAttachments().size(); i++)
					{
						std::string attName = pass.getDepthStencilInputAttachments()[i];

						if (currentRPAttachmentsMap.count(attName) == 0)
						{
							AttachmentDescription attachmentDesc = {};
							attachmentDesc.format = allAttachments[attName].format;
							attachmentDesc.initialLayout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
							attachmentDesc.finalLayout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
							attachmentDesc.loadOp = ATTACHMENT_LOAD_OP_LOAD;
							attachmentDesc.storeOp = ATTACHMENT_STORE_OP_STORE;

							currentRPAttachmentsMap[attName] = currentRPAttachments.size();
							currentRPAttachments.push_back(attachmentDesc);
							currentRPAttClearValues.push_back({0, 0, 0, 0});
						}
					}

					if (!buildingLayersInSubpasses && !buildingLayersInMultiRenderPasses)
						currentRPOpCodes.push_back(std::make_pair<FrameGraphOpCode, void*>(FG_OPCODE_NEXT_SUBPASS, nullptr));
				}

				for (size_t i = 0; i < pass.getColorOutputs().size(); i++)
				{
					std::string attName = pass.getColorOutputs()[i].first;
				
					AttachmentDescription attachmentDesc = {};
					attachmentDesc.format = allAttachments[attName].format;
					attachmentDesc.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
					attachmentDesc.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					attachmentDesc.loadOp = pass.shouldClearColorOutput(attName) ? ATTACHMENT_LOAD_OP_CLEAR : ATTACHMENT_LOAD_OP_DONT_CARE;
					attachmentDesc.storeOp = ATTACHMENT_STORE_OP_STORE;

					currentRPAttachmentsMap[attName + ((buildingLayersInSubpasses || buildingLayersInMultiRenderPasses) ? viewSelectLayerPostfix : viewSelectMipPostfix) + toString(l)] = currentRPAttachments.size();
					currentRPAttachments.push_back(attachmentDesc);
					currentRPAttClearValues.push_back(pass.getColorOutputClearValue(attName));
				}

				if (pass.hasDepthStencilOutput())
				{
					std::string attName = pass.getDepthStencilOutput().first;

					AttachmentDescription attachmentDesc = {};
					attachmentDesc.format = allAttachments[attName].format;
					attachmentDesc.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
					attachmentDesc.finalLayout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
					attachmentDesc.loadOp = pass.shouldClearDepthStencilOutput() ? ATTACHMENT_LOAD_OP_CLEAR : ATTACHMENT_LOAD_OP_DONT_CARE;
					attachmentDesc.storeOp = ATTACHMENT_STORE_OP_STORE;

					currentRPAttachmentsMap[attName + ((buildingLayersInSubpasses || buildingLayersInMultiRenderPasses) ? viewSelectLayerPostfix : viewSelectMipPostfix) + toString(l)] = currentRPAttachments.size();
					currentRPAttachments.push_back(attachmentDesc);
					currentRPAttClearValues.push_back(pass.getDepthStencilOutputClearValue());
				}

				std::vector<AttachmentReference> subpassInputAttachments;
				std::vector<AttachmentReference> subpassColorAttachments;

				for (size_t i = 0; i < pass.getColorInputAttachments().size(); i++)
				{
					AttachmentReference attRef = {};
					attRef.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					attRef.attachment = (uint32_t) currentRPAttachmentsMap[pass.getColorInputAttachments()[i]];

					subpassInputAttachments.push_back(attRef);
				}

				for (size_t i = 0; i < pass.getDepthStencilInputAttachments().size(); i++)
				{
					AttachmentReference attRef = {};
					attRef.layout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
					attRef.attachment = (uint32_t) currentRPAttachmentsMap[pass.getDepthStencilInputAttachments()[i]];

					subpassInputAttachments.push_back(attRef);
				}

				for (size_t i = 0; i < pass.getColorOutputs().size(); i++)
				{
					AttachmentReference attRef = {};
					attRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					attRef.attachment = (uint32_t) currentRPAttachmentsMap[pass.getColorOutputs()[i].first + ((buildingLayersInSubpasses || buildingLayersInMultiRenderPasses) ? viewSelectLayerPostfix : viewSelectMipPostfix) + toString(l)];

					subpassColorAttachments.push_back(attRef);
				}

				if (pass.hasDepthStencilOutput())
				{
					AttachmentReference *attRef = new AttachmentReference();
					attRef->layout = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					attRef->attachment = (uint32_t) currentRPAttachmentsMap[pass.getDepthStencilOutput().first + ((buildingLayersInSubpasses || buildingLayersInMultiRenderPasses) ? viewSelectLayerPostfix : viewSelectMipPostfix) + toString(l)];

					currentRPDepthStencilReferences[pass.getDepthStencilOutput().first + ((buildingLayersInSubpasses || buildingLayersInMultiRenderPasses) ? viewSelectLayerPostfix : viewSelectMipPostfix) + toString(l)] = std::unique_ptr<AttachmentReference>(attRef);
				}

				SubpassDescription subpassDesc = {};
				subpassDesc.bindPoint = PIPELINE_BIND_POINT_GRAPHICS;
				subpassDesc.inputAttachments = subpassInputAttachments;
				subpassDesc.colorAttachments = subpassColorAttachments;
				subpassDesc.depthStencilAttachment = pass.hasDepthStencilOutput() ? currentRPDepthStencilReferences[pass.getDepthStencilOutput().first + ((buildingLayersInSubpasses || buildingLayersInMultiRenderPasses) ? viewSelectLayerPostfix : viewSelectMipPostfix) + toString(l)].get() : nullptr;

				for (size_t i = 0; i < currentRPAttachments.size(); i++)
				{
					bool found = false;

					for (size_t a = 0; a < subpassInputAttachments.size(); a++)
					{
						if (subpassInputAttachments[a].attachment == i)
						{
							found = true;
							break;
						}
					}

					if (!found)
					{
						for (size_t a = 0; a < subpassColorAttachments.size(); a++)
						{
							if (subpassColorAttachments[a].attachment == i)
							{
								found = true;
								break;
							}
						}
					}

					if (!found && pass.hasDepthStencilOutput() && currentRPDepthStencilReferences[pass.getDepthStencilOutput().first + ((buildingLayersInSubpasses || buildingLayersInMultiRenderPasses) ? viewSelectLayerPostfix : viewSelectMipPostfix) + toString(l)]->attachment == i)
					{
						found = true;
					}

					if (!found)
					{
						subpassDesc.preserveAttachments.push_back(i);
					}
				}

				currentRPSubpasses.push_back(subpassDesc);

				callRenderFuncOpsData.push_back(std::unique_ptr<CallRenderFuncOpData>(new CallRenderFuncOpData()));
				CallRenderFuncOpData &crfopData = *callRenderFuncOpsData.back();
				crfopData.passIndex = passStack[p];
				crfopData.counter = buildingLayersInMultiRenderPasses ? l : 0;

				if (l == 0 || buildingLayersInMultiRenderPasses)
					currentRPOpCodes.push_back(std::make_pair<FrameGraphOpCode, void*>(FG_OPCODE_CALL_RENDER_FUNC, &crfopData));

				// If we can't merge with the next pass, then we'll end the render pass
				if ((p == passStack.size() - 1 || !checkIsMergeValid(passStack[p + 1], passStack[p])) && (l == renderLayerCount - 1 || buildingLayersInMultiRenderPasses))
				{
					beginRenderPassOpsData.push_back(std::unique_ptr<FrameGraphRenderPassData>(new FrameGraphRenderPassData()));
					FrameGraphRenderPassData &rpData = *(beginRenderPassOpsData.back().get());
					RenderPassAttachment attExample = pass.hasDepthStencilOutput() ? pass.getDepthStencilOutput().second : pass.getColorOutputs()[0].second;

					uint32_t attSizeX = attExample.sizeSwapchainRelative ? uint32_t(swapchainSize.x * attExample.sizeX) : (uint32_t) attExample.sizeX;
					uint32_t attSizeY = attExample.sizeSwapchainRelative ? uint32_t(swapchainSize.y * attExample.sizeY) : (uint32_t) attExample.sizeY;

					std::vector<TextureView> rpFramebufferTVs(currentRPAttachmentsMap.size());
					std::vector<std::string> testStrs(currentRPAttachmentsMap.size());
					std::vector<ClearValue> rpClearValues(currentRPAttachmentsMap.size());

					for (auto rpAttIt = currentRPAttachmentsMap.begin(); rpAttIt != currentRPAttachmentsMap.end(); rpAttIt++)
					{
						printf("%u using %s\n", rpAttIt->second, rpAttIt->first.c_str());
						rpFramebufferTVs[rpAttIt->second] = attachmentViews[rpAttIt->first].first;
						testStrs[rpAttIt->second] = rpAttIt->first;
						rpClearValues[rpAttIt->second] = currentRPAttClearValues[rpAttIt->second];
					}

					printf("\n");

					rpData.renderPass = renderer->createRenderPass(currentRPAttachments, currentRPSubpasses, currentRPSubpassDependencies);
					rpData.size = {attSizeX, attSizeY, attExample.arrayLayers};
					rpData.framebuffer = renderer->createFramebuffer(rpData.renderPass, rpFramebufferTVs, attSizeX, attSizeY, (buildingLayersInSubpasses || buildingLayersInMultiRenderPasses) ? 1 : attExample.arrayLayers);
					rpData.subpassContents = SUBPASS_CONTENTS_INLINE;
					rpData.clearValues = rpClearValues;

					uint32_t initBaseSubpass = 0;
					for (size_t i = 0; i < currentRPPassIndices.size(); i++)
					{
						passes[currentRPPassIndices[i]]->getInitFunction()(rpData.renderPass, initBaseSubpass);
						initBaseSubpass += currentRPPassSubpassCount[i];
					}

					buildingRenderPass = false;
					currentRPAttachmentsMap.clear();
					currentRPAttachments.clear();
					currentRPDepthStencilReferences.clear();
					currentRPSubpasses.clear();
					currentRPSubpassDependencies.clear();
					currentRPAttClearValues.clear();
					currentRPPassIndices.clear();
					currentRPPassSubpassCount.clear();

					execCodes.push_back(std::make_pair<FrameGraphOpCode, void*>(FG_OPCODE_BEGIN_RENDER_PASS, &rpData));
					execCodes.insert(execCodes.end(), currentRPOpCodes.begin(), currentRPOpCodes.end());
					execCodes.push_back(std::make_pair<FrameGraphOpCode, void*>(FG_OPCODE_END_RENDER_PASS, nullptr));

					for (size_t i = 0; i < pass.getColorOutputs().size(); i++)
					{
						if (pass.getColorOutputMipGenMethod(pass.getColorOutputs()[i].first) == RP_MIP_GEN_POST_BLIT)
						{
							postBlitOpsData.push_back(std::unique_ptr<PostBlitOpData>(new PostBlitOpData()));
							PostBlitOpData &opData = *(postBlitOpsData.back());
							opData.attachmentName = pass.getColorOutputs()[i].first;

							execCodes.push_back(std::make_pair<FrameGraphOpCode, void*>(FG_OPCODE_POST_BLIT, &opData));

						}
					}

					currentRPOpCodes.clear();
				}
				else
				{
					SubpassDependency subpassDep = {};
					subpassDep.srcAccessMask = ACCESS_COLOR_ATTACHMENT_WRITE_BIT | ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					subpassDep.dstAccessMask = ACCESS_INPUT_ATTACHMENT_READ_BIT;
					subpassDep.srcSubpasss = (uint32_t) currentRPSubpasses.size() - 1;
					subpassDep.dstSubpass = (uint32_t) currentRPSubpasses.size();
					subpassDep.srcStageMask = PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
					subpassDep.dstStageMask = PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
					subpassDep.byRegionDependency = true;

					currentRPSubpassDependencies.push_back(subpassDep);
				}
			}
		}
		else if (pass.getPipelineType() == FG_PIPELINE_COMPUTE)
		{
			callRenderFuncOpsData.push_back(std::unique_ptr<CallRenderFuncOpData>(new CallRenderFuncOpData()));
			CallRenderFuncOpData &crfopData = *callRenderFuncOpsData.back();
			crfopData.passIndex = passStack[p];

			execCodes.push_back(std::make_pair<FrameGraphOpCode, void*>(FG_OPCODE_CALL_RENDER_FUNC, &crfopData));

			passes[passStack[p]]->getInitFunction()(nullptr, 0);
		}
	}

	for (size_t i = 0; i < execCodes.size(); i++)
	{
		switch (execCodes[i].first)
		{
			case FG_OPCODE_BEGIN_RENDER_PASS:
				printf("FG_OPCODE_BEGIN_RENDER_PASS\n");
				break;
			case FG_OPCODE_END_RENDER_PASS:
				printf("FG_OPCODE_END_RENDER_PASS\n");
				break;
			case FG_OPCODE_NEXT_SUBPASS:
				printf("FG_OPCODE_NEXT_SUBPASS\n");
				break;
			case FG_OPCODE_POST_BLIT:
				printf("FG_OPCODE_POST_BLIT\n");
				break;
			case FG_OPCODE_CALL_RENDER_FUNC:
				printf("FG_OPCODE_CALL_RENDER_FUNC \"%s\"\n", passes[static_cast<CallRenderFuncOpData*>(execCodes[i].second)->passIndex]->getNodeName().c_str());
				break;
		}
	}
}

void FrameGraph::assignPhysicalResources(const std::vector<size_t> &passStack)
{
	for (size_t i = 0; i < passStack.size(); i++)
	{
		FrameGraphRenderPass &pass = *passes[passStack[i]];

		for (size_t o = 0; o < pass.getColorOutputs().size(); o++)
		{
			allAttachments.emplace(pass.getColorOutputs()[o]);
			attachmentUsages[pass.getColorOutputs()[o].first] = TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
			attachmentLifetimes[pass.getColorOutputs()[o].first] = {std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::min()};

			if (pass.getColorOutputMipGenMethod(pass.getColorOutputs()[o].first) == RP_MIP_GEN_POST_BLIT)
				attachmentUsages[pass.getColorOutputs()[o].first] |= TEXTURE_USAGE_TRANSFER_DST_BIT | TEXTURE_USAGE_TRANSFER_SRC_BIT;

			if (backbufferSource == pass.getColorOutputs()[o].first)
				attachmentUsages[pass.getColorOutputs()[o].first] |= TEXTURE_USAGE_SAMPLED_BIT;

			if (pass.getPipelineType() == FG_PIPELINE_COMPUTE)
				attachmentUsages[pass.getColorOutputs()[o].first] |= TEXTURE_USAGE_TRANSFER_SRC_BIT | TEXTURE_USAGE_TRANSFER_DST_BIT | TEXTURE_USAGE_STORAGE_BIT;
		}

		if (pass.hasDepthStencilOutput())
		{
			allAttachments.emplace(pass.getDepthStencilOutput());
			attachmentUsages[pass.getDepthStencilOutput().first] = TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			attachmentLifetimes[pass.getDepthStencilOutput().first] = {std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::min()};

			if (backbufferSource == pass.getDepthStencilOutput().first)
				attachmentUsages[pass.getDepthStencilOutput().first] |= TEXTURE_USAGE_SAMPLED_BIT;

			if (pass.getPipelineType() == FG_PIPELINE_COMPUTE)
				attachmentUsages[pass.getDepthStencilOutput().first] |= TEXTURE_USAGE_TRANSFER_SRC_BIT | TEXTURE_USAGE_TRANSFER_DST_BIT | TEXTURE_USAGE_STORAGE_BIT;
		}
	}

	for (size_t i = 0; i < passStack.size(); i++)
	{
		FrameGraphRenderPass &pass = *passes[passStack[i]];

		for (size_t o = 0; o < pass.getColorInputs().size(); o++)
		{
			attachmentUsages[pass.getColorInputs()[o]] |= TEXTURE_USAGE_SAMPLED_BIT;

			attachmentLifetimes[pass.getColorInputs()[o]].x = std::min<uint32_t>(attachmentLifetimes[pass.getColorInputs()[o]].x, i);
			attachmentLifetimes[pass.getColorInputs()[o]].y = std::max<uint32_t>(attachmentLifetimes[pass.getColorInputs()[o]].y, i);
		}
		
		for (size_t o = 0; o < pass.getDepthStencilInputs().size(); o++)
		{
			attachmentUsages[pass.getDepthStencilInputs()[o]] |= TEXTURE_USAGE_SAMPLED_BIT;
		
			attachmentLifetimes[pass.getDepthStencilInputs()[o]].x = std::min<uint32_t>(attachmentLifetimes[pass.getDepthStencilInputs()[o]].x, i);
			attachmentLifetimes[pass.getDepthStencilInputs()[o]].y = std::max<uint32_t>(attachmentLifetimes[pass.getDepthStencilInputs()[o]].y, i);
		}

		for (size_t o = 0; o < pass.getColorInputAttachments().size(); o++)
		{
			attachmentUsages[pass.getColorInputAttachments()[o]] |= TEXTURE_USAGE_INPUT_ATTACHMENT_BIT;

			if (!passAttachmentsHaveSameSize(pass.hasDepthStencilOutput() ? pass.getDepthStencilOutput().second : pass.getColorOutputs()[0].second, allAttachments[pass.getColorInputAttachments()[o]]))
				attachmentUsages[pass.getColorInputAttachments()[o]] |= TEXTURE_USAGE_TRANSFER_SRC_BIT;

			attachmentLifetimes[pass.getColorInputAttachments()[o]].x = std::min<uint32_t>(attachmentLifetimes[pass.getColorInputAttachments()[o]].x, i);
			attachmentLifetimes[pass.getColorInputAttachments()[o]].y = std::max<uint32_t>(attachmentLifetimes[pass.getColorInputAttachments()[o]].y, i);
		}

		for (size_t o = 0; o < pass.getDepthStencilInputAttachments().size(); o++)
		{
			attachmentUsages[pass.getDepthStencilInputAttachments()[o]] |= TEXTURE_USAGE_INPUT_ATTACHMENT_BIT;

			if (!passAttachmentsHaveSameSize(pass.hasDepthStencilOutput() ? pass.getDepthStencilOutput().second : pass.getColorOutputs()[0].second, allAttachments[pass.getDepthStencilInputAttachments()[o]]))
				attachmentUsages[pass.getDepthStencilInputAttachments()[o]] |= TEXTURE_USAGE_TRANSFER_SRC_BIT;

			attachmentLifetimes[pass.getDepthStencilInputAttachments()[o]].x = std::min<uint32_t>(attachmentLifetimes[pass.getDepthStencilInputAttachments()[o]].x, i);
			attachmentLifetimes[pass.getDepthStencilInputAttachments()[o]].y = std::max<uint32_t>(attachmentLifetimes[pass.getDepthStencilInputAttachments()[o]].y, i);
		}

		for (size_t o = 0; o < pass.getColorOutputs().size(); o++)
		{
			attachmentLifetimes[pass.getColorOutputs()[o].first].x = std::min<uint32_t>(attachmentLifetimes[pass.getColorOutputs()[o].first].x, i);
			attachmentLifetimes[pass.getColorOutputs()[o].first].y = std::max<uint32_t>(attachmentLifetimes[pass.getColorOutputs()[o].first].y, i);
		}

		if (pass.hasDepthStencilOutput())
		{
			attachmentLifetimes[pass.getDepthStencilOutput().first].x = std::min<uint32_t>(attachmentLifetimes[pass.getDepthStencilOutput().first].x, i);
			attachmentLifetimes[pass.getDepthStencilOutput().first].y = std::max<uint32_t>(attachmentLifetimes[pass.getDepthStencilOutput().first].y, i);
		}
	}

	auto sT = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch());
	for (auto attIt = allAttachments.begin(); attIt != allAttachments.end(); attIt++)
	{
		const RenderPassAttachment &att = attIt->second;
		suvec2 attLifetime = attachmentLifetimes[attIt->first];
		bool foundAlias = false;
		size_t aliasIndex = 0;

		// Search for a texture that we can alias instead of creating a new one first
		for (size_t i = 0; i < graphTextures.size(); i++)
		{
			if (att == graphTextures[i].base)
			{
				bool foundNoLifetimeConflict = true;

				for (size_t l = 0; l < graphTextures[i].usageLifetimes.size(); l++)
				{
					if (!(attLifetime.x > graphTextures[i].usageLifetimes[l].y || attLifetime.y < graphTextures[i].usageLifetimes[l].x) || att.mipLevels > 0)
					{
						foundNoLifetimeConflict = false;
						break;
					}
				}

				if (foundNoLifetimeConflict)
				{
					printf("%s found an alias!\n", attIt->first.c_str());
					foundAlias = true;
					aliasIndex = i;
					break;
				}
			}
		}

		foundAlias = false;

		if (!foundAlias)
		{
			uint32_t sizeX = att.sizeSwapchainRelative ? uint32_t(swapchainSize.x * att.sizeX) : uint32_t(att.sizeX);
			uint32_t sizeY = att.sizeSwapchainRelative ? uint32_t(swapchainSize.y * att.sizeY) : uint32_t(att.sizeY);
			FrameGraphTexture graphTex = {};
			graphTex.base = att;
			graphTex.usageLifetimes = {attLifetime};
			graphTex.attTex = renderer->createTexture({sizeX, sizeY, uint32_t(att.sizeZ)}, att.format, attachmentUsages[attIt->first], MEMORY_USAGE_GPU_ONLY, true, att.mipLevels, att.arrayLayers);
			renderer->setObjectDebugName(graphTex.attTex, OBJECT_TYPE_TEXTURE, attIt->first);

			printf("%s using view type %u\n", attIt->first.c_str(), att.viewType);
			graphTextures.push_back(graphTex);
			attachmentViews[attIt->first] = std::make_pair(renderer->createTextureView(graphTex.attTex, att.viewType, {0, att.mipLevels, 0, att.arrayLayers}, att.format), graphTextures.size() - 1);
			renderer->setObjectDebugName(attachmentViews[attIt->first].first, OBJECT_TYPE_TEXTURE_VIEW, attIt->first + "_view");

			for (uint32_t m = 0; m < att.mipLevels; m++)
			{
				attachmentViews[attIt->first + viewSelectMipPostfix + toString(m)] = std::make_pair(renderer->createTextureView(graphTex.attTex, att.arrayLayers > 1 ? TEXTURE_VIEW_TYPE_2D_ARRAY : TEXTURE_VIEW_TYPE_2D, {m, 1, 0, att.arrayLayers}, att.format), graphTextures.size() - 1);
				renderer->setObjectDebugName(attachmentViews[attIt->first + viewSelectMipPostfix + toString(m)].first, OBJECT_TYPE_TEXTURE_VIEW, attIt->first + "_view_miplvl_" + toString(m));
			}
			
			for (uint32_t l = 0; l < att.arrayLayers; l++)
			{
				attachmentViews[attIt->first + viewSelectLayerPostfix + toString(l)] = std::make_pair(renderer->createTextureView(graphTex.attTex, TEXTURE_VIEW_TYPE_2D, {0, 1, l, 1}, att.format), graphTextures.size() - 1);
				renderer->setObjectDebugName(attachmentViews[attIt->first + viewSelectLayerPostfix + toString(l)].first, OBJECT_TYPE_TEXTURE_VIEW, attIt->first + "_view_layer_" + toString(l));
			}
		}
		else
		{
			graphTextures[aliasIndex].usageLifetimes.push_back(attLifetime);
			attachmentViews[attIt->first] = std::make_pair(renderer->createTextureView(graphTextures[aliasIndex].attTex, att.viewType, {0, att.mipLevels, 0, att.arrayLayers}, att.format), aliasIndex);
		}
	}
	printf("actual creation took: %fms\n", (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()) - sT).count() / 1000.0f);
}

std::vector<size_t> FrameGraph::optimizePassOrder(const std::vector<size_t> &passStack)
{
	std::vector<size_t> unscheduledPasses(passStack);
	std::vector<size_t> scheduledPasses;

	scheduledPasses.push_back(unscheduledPasses.back());
	unscheduledPasses.pop_back();

	while (unscheduledPasses.size() > 0)
	{
		size_t mergedCount = 0;
		size_t testPass = scheduledPasses.back();
		
		while (true)
		{
			bool foundValidMerge = false;
			for (int64_t i = int64_t(unscheduledPasses.size()) - 1; i >= 0; i--)
			{
				if (checkIsMergeValid(testPass, unscheduledPasses[i]))
				{
					foundValidMerge = true;
					mergedCount++;

					scheduledPasses.push_back(unscheduledPasses[i]);
					unscheduledPasses.erase(unscheduledPasses.begin() + size_t(i));
					break;
				}
			}

			if (!foundValidMerge)
				break;
		}

		if (mergedCount == 0)
		{
			scheduledPasses.push_back(unscheduledPasses.back());
			unscheduledPasses.pop_back();
		}
	}

	std::reverse(scheduledPasses.begin(), scheduledPasses.end());

	unscheduledPasses = scheduledPasses;
	scheduledPasses.clear();

	while (unscheduledPasses.size() > 0)
	{
		size_t bestCandidate = 0;
		uint32_t bestOverlapScore = 0;

		for (size_t i = 0; i < unscheduledPasses.size(); i++)
		{
			uint32_t overlapScore = 0;

			if (scheduledPasses.size() > 0 && checkIsMergeValid(unscheduledPasses[i], scheduledPasses.back()))
			{
				overlapScore = std::numeric_limits<uint32_t>::max();
			}
			else
			{
				for (size_t j = 0; j < scheduledPasses.size(); j++)
				{
					if (passContainsPassInDependencyChain(unscheduledPasses[i], scheduledPasses[j]))
						break;

					overlapScore++;
				}
			}

			if (overlapScore <= bestOverlapScore)
				continue;

			bool possible_candidate = true;
			for (size_t j = 0; j < i; j++)
			{
				if (passContainsPassInDependencyChain(unscheduledPasses[i], unscheduledPasses[j]))
				{
					possible_candidate = false;
					break;
				}
			}

			if (!possible_candidate)
				continue;

			bestCandidate = i;
			bestOverlapScore = overlapScore;
		}

		scheduledPasses.push_back(unscheduledPasses[bestCandidate]);
		unscheduledPasses.erase(unscheduledPasses.begin() + bestCandidate);
	}

	unscheduledPasses = scheduledPasses;
	scheduledPasses.clear();

	scheduledPasses.push_back(unscheduledPasses.back());
	unscheduledPasses.pop_back();

	while (unscheduledPasses.size() > 0)
	{
		size_t mergedCount = 0;
		size_t testPass = scheduledPasses.back();

		while (true)
		{
			bool foundValidMerge = false;
			for (int64_t i = int64_t(unscheduledPasses.size()) - 1; i >= 0; i--)
			{
				if (checkIsMergeValid(testPass, unscheduledPasses[i]))
				{
					foundValidMerge = true;
					mergedCount++;

					scheduledPasses.push_back(unscheduledPasses[i]);
					unscheduledPasses.erase(unscheduledPasses.begin() + size_t(i));
					break;
				}
			}

			if (!foundValidMerge)
				break;
		}

		if (mergedCount == 0)
		{
			scheduledPasses.push_back(unscheduledPasses.back());
			unscheduledPasses.pop_back();
		}
	}

	std::reverse(scheduledPasses.begin(), scheduledPasses.end());

	return scheduledPasses;
}

void FrameGraph::validate()
{
	// Some basic checks first
	if (backbufferSource.empty())
	{
		printf("%s Frame graph is missing a backbuffer source!\n", ERR_PREFIX);
		throw std::exception("invalid frame graph, no backbuffer source");
	}

	if (passes.size() == 0)
	{
		printf("%s Frame graph is empty, there are no passes!\n", ERR_PREFIX);
		throw std::exception("invalid frame graph, passes.size() == 0");
	}

	std::vector<std::string> allOutputs; // A list of all outputs from all the render passes

	for (size_t i = 0; i < passes.size(); i++)
	{
		const auto &passColorOutputs = passes[i]->getColorOutputs();

		for (size_t a = 0; a < passColorOutputs.size(); a++)
			allOutputs.push_back(passColorOutputs[a].first);

		if (passes[i]->hasDepthStencilOutput())
			allOutputs.push_back(passes[i]->getDepthStencilOutput().first);
	}

	// Make sure the backbuffer source is an actual output from one of the render passes
	if (std::find(allOutputs.begin(), allOutputs.end(), backbufferSource) == allOutputs.end())
	{
		printf("%s The frame graph backbuffer source \"%s\" doesn't exist!\n", ERR_PREFIX, backbufferSource.c_str());
		throw std::exception("invalid framegraph, backbuffer source does not exist");
	}

	// Check to make sure no pass has an input that doesn't exist
	for (size_t i = 0; i < passes.size(); i++)
	{
		std::vector<std::string> colorInputs = passes[i]->getColorInputs();
		std::vector<std::string> colorInputAttachments = passes[i]->getColorInputAttachments();
		std::vector<std::string> depthStencilInputs = passes[i]->getDepthStencilInputs();
		std::vector<std::string> depthStencilInputAttachments = passes[i]->getDepthStencilInputAttachments();

		for (size_t j = 0; j < colorInputs.size(); j++)
		{
			if (std::find(allOutputs.begin(), allOutputs.end(), colorInputs[j]) == allOutputs.end())
			{
				printf("%s RenderPass \"%s\" contains a color input \"%s\" that doesn't exist!\n", ERR_PREFIX, passes[i]->getNodeName().c_str(), colorInputs[j].c_str());
				throw std::exception("invalid frame graph, input does not exist");
			}
		}

		for (size_t j = 0; j < colorInputAttachments.size(); j++)
		{
			if (std::find(allOutputs.begin(), allOutputs.end(), colorInputAttachments[j]) == allOutputs.end())
			{
				printf("%s RenderPass \"%s\" contains a color input attachment \"%s\" that doesn't exist!\n", ERR_PREFIX, passes[i]->getNodeName().c_str(), colorInputAttachments[j].c_str());
				throw std::exception("invalid frame graph, input does not exist");
			}
		}

		for (size_t j = 0; j < depthStencilInputs.size(); j++)
		{
			if (std::find(allOutputs.begin(), allOutputs.end(), depthStencilInputs[j]) == allOutputs.end())
			{
				printf("%s RenderPass \"%s\" contains an depth/stencil input \"%s\" that doesn't exist!\n", ERR_PREFIX, passes[i]->getNodeName().c_str(), depthStencilInputs[j].c_str());
				throw std::exception("invalid frame graph, input does not exist");
			}
		}

		for (size_t j = 0; j < depthStencilInputAttachments.size(); j++)
		{
			if (std::find(allOutputs.begin(), allOutputs.end(), colorInputs[j]) == depthStencilInputAttachments.end())
			{
				printf("%s RenderPass \"%s\" contains an depth/stencil input attachment \"%s\" that doesn't exist!\n", ERR_PREFIX, passes[i]->getNodeName().c_str(), depthStencilInputAttachments[j].c_str());
				throw std::exception("invalid frame graph, input does not exist");
			}
		}
	}

	// Make sure no passes implicitly have an output that is to one of it's inputs, that should be declared explicitly
	for (size_t i = 0; i < passes.size(); i++)
	{
		std::vector<std::string> colorInputs = passes[i]->getColorInputs();
		std::vector<std::string> colorInputAttachments = passes[i]->getColorInputAttachments();
		std::vector<std::string> depthStencilInputs = passes[i]->getDepthStencilInputs();
		std::vector<std::string> depthStencilInputAttachments = passes[i]->getDepthStencilInputAttachments();

		std::vector<std::pair<std::string, RenderPassAttachment>> colorOutputs = passes[i]->getColorOutputs();

		for (size_t o = 0; o < colorOutputs.size(); o++)
		{
			if (std::find(colorInputs.begin(), colorInputs.end(), colorOutputs[o].first) != colorInputs.end())
			{
				printf("%s RenderPass \"%s\" contains an output \"%s\" that is also one if it's color inputs, that should be declared explicitly using NOTIMPL(..)\n", ERR_PREFIX, passes[i]->getNodeName().c_str(), colorOutputs[o].first.c_str());
				throw std::exception("invalid frame graph, a passes's output is implicitly one of it's outputs");
			}
			else if (std::find(colorInputAttachments.begin(), colorInputAttachments.end(), colorOutputs[o].first) != colorInputAttachments.end())
			{
				printf("%s RenderPass \"%s\" contains an output \"%s\" that is also one if it's color input attachments, that should be declared explicitly using NOTIMPL(..)\n", ERR_PREFIX, passes[i]->getNodeName().c_str(), colorOutputs[o].first.c_str());
				throw std::exception("invalid frame graph, a passes's output is implicitly one of it's outputs");
			}
		}

		if (passes[i]->hasDepthStencilOutput())
		{
			if (std::find(depthStencilInputs.begin(), depthStencilInputs.end(), passes[i]->getDepthStencilOutput().first) != depthStencilInputs.end())
			{
				printf("%s RenderPass \"%s\" contains an output \"%s\" that is also one if it's depth/stencil inputs, that should be declared explicitly using NOTIMPL(..)\n", ERR_PREFIX, passes[i]->getNodeName().c_str(), passes[i]->getDepthStencilOutput().first.c_str());
				throw std::exception("invalid frame graph, a passes's output is implicitly one of it's outputs");
			}
			else if (std::find(depthStencilInputAttachments.begin(), depthStencilInputAttachments.end(), passes[i]->getDepthStencilOutput().first) != depthStencilInputAttachments.end())
			{
				printf("%s RenderPass \"%s\" contains an output \"%s\" that is also one if it's depth/stencil input attachments, that should be declared explicitly using NOTIMPL(..)\n", ERR_PREFIX, passes[i]->getNodeName().c_str(), passes[i]->getDepthStencilOutput().first.c_str());
				throw std::exception("invalid frame graph, a passes's output is implicitly one of it's outputs");
			}
		}
	}

	// Test to see if any passes have a color input that's actually a depth/stencil texture, or vice versa

	// Check to see if any passes have duplicate outputs or inputs

	// Do a basic check for any cyclic dependencies

	// Check if any compute passes contain any input attachments, which can't happen because compute shader exist outside of render passes

	// Check to see if any of the passes have input attachments, and if they do that they're outputs are all the same size
	
	// Make sure same-name outputs of different passes have the same attachment info

	// Make sure that no more than one pass writes to an output
	
	// Make sure compute passes don't have subpass inputs
}

void FrameGraph::recursiveFindWritePasses(const std::string &name, std::vector<size_t> &passStack)
{
	std::vector<size_t> writePasses = getPassesThatWriteToResource(name);

	for (size_t i = 0; i < writePasses.size(); i++)
	{
		if (std::find(passStack.begin(), passStack.end(), writePasses[i]) != passStack.end())
			continue;

		passStack.push_back(writePasses[i]);
		FrameGraphRenderPass &renderPass = *passes[writePasses[i]].get();
		
		for (size_t j = 0; j < renderPass.getColorInputs().size(); j++)
			recursiveFindWritePasses(renderPass.getColorInputs()[j], passStack);

		for (size_t j = 0; j < renderPass.getColorInputAttachments().size(); j++)
			recursiveFindWritePasses(renderPass.getColorInputAttachments()[j], passStack);

		for (size_t j = 0; j < renderPass.getDepthStencilInputs().size(); j++)
			recursiveFindWritePasses(renderPass.getDepthStencilInputs()[j], passStack);

		for (size_t j = 0; j < renderPass.getDepthStencilInputAttachments().size(); j++)
			recursiveFindWritePasses(renderPass.getDepthStencilInputAttachments()[j], passStack);
	}
}

std::vector<size_t> FrameGraph::getPassesThatWriteToResource(const std::string &name)
{
	std::vector<size_t> writePasses;

	for (size_t p = 0; p < passes.size(); p++)
	{
		std::vector<std::pair<std::string, RenderPassAttachment>> colorOutputs = passes[p]->getColorOutputs();
	
		for (size_t o = 0; o < colorOutputs.size(); o++)
		{
			if (colorOutputs[o].first == name)
			{
				writePasses.push_back(p);
				break;
			}
		}

		if (passes[p]->hasDepthStencilOutput() && !(writePasses.size() > 0 && writePasses.back() == p))
		{
			if (passes[p]->getDepthStencilOutput().first == name)
			{
				writePasses.push_back(p);
				continue;
			}
		}
	}

	return writePasses;
}

bool FrameGraph::passAttachmentsHaveSameSize(const RenderPassAttachment& att0, const RenderPassAttachment &att1)
{
	if (att0.arrayLayers != att1.arrayLayers)
		return false;

	if (att0.sizeSwapchainRelative != att1.sizeSwapchainRelative)
		return false;
	
	return att0.sizeX == att1.sizeX && att0.sizeY == att1.sizeY && att0.sizeZ == att1.sizeZ;
}

bool FrameGraph::checkIsMergeValid(size_t passIndex, size_t writePassIndex)
{
	if (passIndex == writePassIndex)
		return false;

	if (!passContainsPassInDependencyChain(passIndex, writePassIndex))
		return false;

	FrameGraphRenderPass &pass = *passes[passIndex];
	FrameGraphRenderPass &writePass = *passes[writePassIndex];

	if (pass.getPipelineType() != writePass.getPipelineType())
		return false;

	for (size_t i = 0; i < writePass.getColorOutputs().size(); i ++)
		if (writePass.getColorOutputMipGenMethod(writePass.getColorOutputs()[i].first) != RP_MIP_GEN_NONE)
			return false;

	for (size_t i = 0; i < pass.getColorOutputs().size(); i++)
		if (pass.getColorOutputMipGenMethod(pass.getColorOutputs()[i].first) != RP_MIP_GEN_NONE)
			return false;

	std::vector<std::string> depthStencilInputs = pass.getDepthStencilInputs();
	std::vector<std::string> passInputs = pass.getColorInputs();
	passInputs.insert(passInputs.end(), depthStencilInputs.begin(), depthStencilInputs.end());

	std::vector<std::string> depthStencilInputAttachments = pass.getDepthStencilInputAttachments();
	std::vector<std::string> passInputAtts = pass.getColorInputAttachments();
	passInputAtts.insert(passInputAtts.end(), depthStencilInputAttachments.begin(), depthStencilInputAttachments.end());

	/*
	Check normal inputs first, because if the writePass that is going to be merged is in this input's depedency chain, then it's impossible
	for them to be merged.
	*/
	for (size_t i = 0; i < passInputs.size(); i++)
		if (attachmentContainsPassInDependencyChain(passInputs[i], writePassIndex))
			return false;


	for (size_t i = 0; i < passInputAtts.size(); i++)
		if (inputAttachmentHasInvalidSubpassChainToPass(passInputAtts[i], writePassIndex))
			return false;

	// Check to make sure the outputs of the two passes we're trying to merge have the same sizes
	auto passOutputs = pass.getColorOutputs();
	if (pass.hasDepthStencilOutput())
		appendVector(passOutputs, {pass.getDepthStencilOutput()});

	auto writePassOutputs = writePass.getColorOutputs();
	if (writePass.hasDepthStencilOutput())
		appendVector(writePassOutputs, {writePass.getDepthStencilOutput()});

	for (size_t po = 0; po < passOutputs.size(); po++)
	{
		for (size_t wpo = 0; wpo < writePassOutputs.size(); wpo++)
		{
			if (!passAttachmentsHaveSameSize(passOutputs[po].second, writePassOutputs[wpo].second))
				return false;
		}
	}

	return true;
}

bool FrameGraph::inputAttachmentHasInvalidSubpassChainToPass(const std::string &name, size_t passIndex)
{
	std::vector<size_t> writePasses = getPassesThatWriteToResource(name);

	for (size_t w = 0; w < writePasses.size(); w++)
	{
		FrameGraphRenderPass &pass = *passes[writePasses[w]];

		std::vector<std::string> passInputs = pass.getColorInputs();
		appendVector(passInputs, pass.getDepthStencilInputs());
		appendVector(passInputs, pass.getColorInputAttachments());
		appendVector(passInputs, pass.getDepthStencilInputAttachments());

		for (size_t i = 0; i < passInputs.size(); i++)
			if (attachmentContainsPassInDependencyChain(passInputs[i], passIndex))
				return true;
	}

	/*
	If we get here, we either have no chain, or the chain is all input attachments.
	Now we check, if it's an input attachment chain, if they all have the same size
	*/



	return false;
}

bool FrameGraph::attachmentContainsPassInDependencyChain(const std::string &name, size_t pass)
{
	std::vector<size_t> writePasses = getPassesThatWriteToResource(name);

	for (size_t i = 0; i < writePasses.size(); i++)
	{
		if (writePasses[i] == pass)
			return true;

		FrameGraphRenderPass &writePass = *passes[writePasses[i]];

		std::vector<std::string> passInputs = writePass.getColorInputs();
		appendVector(passInputs, writePass.getDepthStencilInputs());
		appendVector(passInputs, writePass.getColorInputAttachments());
		appendVector(passInputs, writePass.getDepthStencilInputAttachments());

		for (size_t wi = 0; wi < passInputs.size(); wi++)
			if (attachmentContainsPassInDependencyChain(passInputs[wi], pass))
				return true;
	}

	return false;
}

bool FrameGraph::passContainsPassInDependencyChain(size_t passIndex, size_t writePassIndex)
{
	if (passIndex == writePassIndex)
		return true;

	if (writePassIndex > passIndex)
		return false;

	FrameGraphRenderPass &pass = *passes[passIndex];
	std::vector<std::string> passInputs = pass.getColorInputs();
	appendVector(passInputs, pass.getDepthStencilInputs());
	appendVector(passInputs, pass.getColorInputAttachments());
	appendVector(passInputs, pass.getDepthStencilInputAttachments());

	for (size_t i = 0; i < passInputs.size(); i++)
	{
		std::vector<size_t> writePasses = getPassesThatWriteToResource(passInputs[i]);

		for (size_t w = 0; w < writePasses.size(); w++)
		{
			if (writePasses[w] == writePassIndex)
				return true;

			if (passContainsPassInDependencyChain(writePasses[w], writePassIndex))
				return true;
		}
	}

	return false;
}

void FrameGraph::setBackbufferSource(const std::string &name)
{
	backbufferSource = name;
}

TextureView FrameGraph::getBackbufferSourceTextureView()
{
	return attachmentViews[backbufferSource].first;
}

FrameGraphRenderPass &FrameGraph::addRenderPass(const std::string &name, FrameGraphPipelineType pipelineType)
{
	passes.push_back(std::unique_ptr<FrameGraphRenderPass> (new FrameGraphRenderPass(name, pipelineType)));

	return *passes.back().get();
}
