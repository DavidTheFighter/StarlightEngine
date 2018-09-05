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
 * PostProcess.cpp
 *
 *  Created on: Apr 21, 2018
 *      Author: David
 */

#include "Rendering/PostProcess/PostProcess.h"

#include <Rendering/Renderer/Renderer.h>

PostProcess::PostProcess (Renderer *rendererPtr)
{
	renderer = rendererPtr;

	combinePipeline = nullptr;

	combineDescriptorPool = nullptr;
	combineDescriptorSet = nullptr;

	inputSampler = nullptr;
}

PostProcess::~PostProcess ()
{
	renderer->destroyDescriptorPool(combineDescriptorPool);

	renderer->destroyPipeline(combinePipeline);

	renderer->destroySampler(inputSampler);
}

void PostProcess::combineTonemapPassInit(RenderPass renderPass, uint32_t baseSubpass)
{
	createCombinePipeline(renderPass, baseSubpass);

	combineDescriptorPool = renderer->createDescriptorPool({{
		{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT}
	}}, 1);

	combineDescriptorSet = combineDescriptorPool->allocateDescriptorSet();
	inputSampler = renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SAMPLER_FILTER_NEAREST, SAMPLER_FILTER_NEAREST, 1);

	DescriptorImageInfo inputSamplerInfo = {};
	inputSamplerInfo.sampler = inputSampler;

	DescriptorWriteInfo inputSamplerWrite = {};
	inputSamplerWrite.descriptorCount = 1;
	inputSamplerWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLER;
	inputSamplerWrite.dstBinding = 0;
	inputSamplerWrite.dstSet = combineDescriptorSet;
	inputSamplerWrite.imageInfo = {inputSamplerInfo};

	renderer->writeDescriptorSets({inputSamplerWrite});
}

void PostProcess::combineTonemapPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size)
{
	DescriptorImageInfo deferredOutputInfo = {};
	deferredOutputInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	deferredOutputInfo.view = views["deferredLighting"];

	DescriptorWriteInfo deferredOutputWrite = {};
	deferredOutputWrite.descriptorCount = 1;
	deferredOutputWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	deferredOutputWrite.dstBinding = 1;
	deferredOutputWrite.dstSet = combineDescriptorSet;
	deferredOutputWrite.imageInfo = {deferredOutputInfo};

	renderer->writeDescriptorSets({deferredOutputWrite});
}

void PostProcess::combineTonemapPassRender(CommandBuffer cmdBuffer, uint32_t counter)
{
	cmdBuffer->beginDebugRegion("Final/Combine", glm::vec4(0.8f, 0, 0, 1.0f));
	cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, combinePipeline);
	cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {combineDescriptorSet});

	// Vertex positions contained in the shader as constants
	cmdBuffer->draw(6);
}

void PostProcess::createCombinePipeline (RenderPass renderPass, uint32_t baseSubpass)
{
	ShaderModule vertShader = renderer->createShaderModule("GameData/shaders/combine.hlsl", SHADER_STAGE_VERTEX_BIT, SHADER_LANGUAGE_HLSL, "CombineVS");
	ShaderModule fragShader = renderer->createShaderModule("GameData/shaders/combine.hlsl", SHADER_STAGE_FRAGMENT_BIT, SHADER_LANGUAGE_HLSL, "CombineFS");

	PipelineShaderStage vertShaderStage = {};
	vertShaderStage.entry = "CombineVS";
	vertShaderStage.module = vertShader;

	PipelineShaderStage fragShaderStage = {};
	fragShaderStage.entry = "CombineFS";
	fragShaderStage.module = fragShader;

	PipelineVertexInputInfo vertexInput = {};
	vertexInput.vertexInputAttribs = {};
	vertexInput.vertexInputBindings = {};

	PipelineInputAssemblyInfo inputAssembly = {};
	inputAssembly.primitiveRestart = false;
	inputAssembly.topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	PipelineViewportInfo viewportInfo = {};
	viewportInfo.scissors = {{0, 0, 1920, 1080}};
	viewportInfo.viewports = {{0, 0, 1920, 1080}};

	PipelineRasterizationInfo rastInfo = {};
	rastInfo.clockwiseFrontFace = false;
	rastInfo.cullMode = CULL_MODE_NONE;
	rastInfo.lineWidth = 1;
	rastInfo.polygonMode = POLYGON_MODE_FILL;
	rastInfo.rasterizerDiscardEnable = false;
	rastInfo.enableOutOfOrderRasterization = false;

	PipelineDepthStencilInfo depthInfo = {};
	depthInfo.enableDepthTest = false;
	depthInfo.enableDepthWrite = false;
	depthInfo.minDepthBounds = 0;
	depthInfo.maxDepthBounds = 1;
	depthInfo.depthCompareOp = COMPARE_OP_ALWAYS;

	PipelineColorBlendAttachment colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = false;
	colorBlendAttachment.colorWriteMask = COLOR_COMPONENT_R_BIT | COLOR_COMPONENT_G_BIT | COLOR_COMPONENT_B_BIT | COLOR_COMPONENT_A_BIT;

	PipelineColorBlendInfo colorBlend = {};
	colorBlend.attachments = {colorBlendAttachment};
	colorBlend.logicOpEnable = false;
	colorBlend.logicOp = LOGIC_OP_COPY;
	colorBlend.blendConstants[0] = 1.0f;
	colorBlend.blendConstants[1] = 1.0f;
	colorBlend.blendConstants[2] = 1.0f;
	colorBlend.blendConstants[3] = 1.0f;

	PipelineDynamicStateInfo dynamicState = {};
	dynamicState.dynamicStates = {DYNAMIC_STATE_VIEWPORT, DYNAMIC_STATE_SCISSOR};

	GraphicsPipelineInfo info = {};
	info.stages = {vertShaderStage, fragShaderStage};
	info.vertexInputInfo = vertexInput;
	info.inputAssemblyInfo = inputAssembly;
	info.viewportInfo = viewportInfo;
	info.rasterizationInfo = rastInfo;
	info.depthStencilInfo = depthInfo;
	info.colorBlendInfo = colorBlend;
	info.dynamicStateInfo = dynamicState;

	info.inputPushConstantRanges = {};
	info.inputSetLayouts = {{
			{0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
			{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT}
	}};

	combinePipeline = renderer->createGraphicsPipeline(info, renderPass, baseSubpass);

	renderer->destroyShaderModule(vertShaderStage.module);
	renderer->destroyShaderModule(fragShaderStage.module);
}
