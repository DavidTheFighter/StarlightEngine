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
 * DeferredRenderer.cpp
 * 
 * Created on: Mar 24, 2018
 *     Author: david
 */

#include "Rendering/Deferred/DeferredRenderer.h"

#include <Engine/StarlightEngine.h>

#include <Game/Game.h>

#include <Rendering/Renderer/Renderer.h>
#include <Rendering/World/WorldRenderer.h>
#include <Rendering/World/CSM.h>
#include <Rendering/Deferred/AtmosphereRenderer.h>
#include <Rendering/Deferred/SkyCubemapRenderer.h>
#include <Rendering/World/TerrainShadowRenderer.h>

#include <Game/API/SEAPI.h>

DeferredRenderer::DeferredRenderer (StarlightEngine *enginePtr, AtmosphereRenderer *atmospherePtr)
{
	engine = enginePtr;
	atmosphere = atmospherePtr;

	deferredPipeline = nullptr;
	linearSampler = nullptr;
	atmosphereTextureSampler = nullptr;

	deferredInputsDescriptorPool = nullptr;
	deferredInputDescriptorSet = nullptr;
	game = nullptr;
}

DeferredRenderer::~DeferredRenderer ()
{
	engine->renderer->destroyDescriptorPool(deferredInputsDescriptorPool);

	engine->renderer->destroySampler(linearSampler);
	engine->renderer->destroySampler(atmosphereTextureSampler);
	engine->renderer->destroySampler(shadowsSampler);
	engine->renderer->destroySampler(ditherSampler);

	engine->renderer->destroyPipeline(deferredPipeline);

	engine->resources->returnTexture(brdfLUT);
}

const uint32_t lightingPcSize = (uint32_t) (sizeof(glm::mat4) + sizeof(glm::vec4) + sizeof(glm::vec4) + sizeof(glm::vec4));

void DeferredRenderer::lightingPassInit(RenderPass renderPass, uint32_t baseSubpass)
{
	atmosphereTextureSampler = engine->renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SAMPLER_FILTER_LINEAR, SAMPLER_FILTER_LINEAR);
	createDeferredLightingPipeline(renderPass, baseSubpass);

	linearSampler = engine->renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SAMPLER_FILTER_LINEAR, SAMPLER_FILTER_LINEAR, 1.0f, {0, 16, 0});
	shadowsSampler = engine->renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SAMPLER_FILTER_LINEAR, SAMPLER_FILTER_LINEAR);
	ditherSampler = engine->renderer->createSampler(SAMPLER_ADDRESS_MODE_REPEAT, SAMPLER_FILTER_NEAREST, SAMPLER_FILTER_NEAREST);

	brdfLUT = engine->resources->loadTextureImmediate("GameData/textures/brdf2dlut.dds");

	deferredInputsDescriptorPool = engine->renderer->createDescriptorPool({{
		{0, DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, SHADER_STAGE_FRAGMENT_BIT},
		{1, DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, SHADER_STAGE_FRAGMENT_BIT},
		{2, DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, SHADER_STAGE_FRAGMENT_BIT},
		{3, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{4, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{5, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{6, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{7, DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_FRAGMENT_BIT},
		{8, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{9, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{10, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{11, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{12, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{13, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{14, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{15, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{16, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT}
		}}, 1);

	deferredInputDescriptorSet = deferredInputsDescriptorPool->allocateDescriptorSet();

	DescriptorImageInfo transmittanceImageInfo = {};
	transmittanceImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	transmittanceImageInfo.sampler = atmosphereTextureSampler;
	transmittanceImageInfo.view = atmosphere->transmittanceTV;

	DescriptorImageInfo scatteringImageInfo = {};
	scatteringImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	scatteringImageInfo.sampler = atmosphereTextureSampler;
	scatteringImageInfo.view = atmosphere->scatteringTV;

	DescriptorImageInfo singleMieScatteringImageInfo = {};
	singleMieScatteringImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	singleMieScatteringImageInfo.sampler = atmosphereTextureSampler;
	singleMieScatteringImageInfo.view = atmosphere->scatteringTV;

	DescriptorImageInfo irradianceImageInfo = {};
	irradianceImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	irradianceImageInfo.sampler = atmosphereTextureSampler;
	irradianceImageInfo.view = atmosphere->irradianceTV;

	DescriptorImageInfo sunShadowmapSamplerInfo = {};
	sunShadowmapSamplerInfo.sampler = shadowsSampler;

	DescriptorImageInfo ditherTextureInfo = {};
	ditherTextureInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ditherTextureInfo.sampler = ditherSampler;
	ditherTextureInfo.view = engine->resources->getDitherPatternTexture();

	DescriptorImageInfo linearSamplerInfo = {};
	linearSamplerInfo.sampler = linearSampler;

	DescriptorImageInfo brdfLUTImageInfo = {};
	brdfLUTImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	brdfLUTImageInfo.view = brdfLUT->textureView;

	DescriptorBufferInfo worldEnvironmentUBOBufferInfo = {};
	worldEnvironmentUBOBufferInfo.buffer = engine->api->getWorldEnvironmentUBO();
	worldEnvironmentUBOBufferInfo.range = sizeof(WorldEnvironmentUBO);

	DescriptorWriteInfo transmittanceWriteInfo = {}, scatteringWriteInfo = {}, singleMieScatteringWriteInfo = {}, irradianceWriteInfo = {}, sunShadowmapSamplerWriteInfo = {}, ditherSamplerWriteInfo = {}, ditherTextureWriteInfo = {}, worldEnvironmentUBOWrite = {}, linearSamplerWrite = {}, brdfLUTWrite = {};
	transmittanceWriteInfo.descriptorCount = 1;
	transmittanceWriteInfo.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	transmittanceWriteInfo.dstBinding = 3;
	transmittanceWriteInfo.dstSet = deferredInputDescriptorSet;
	transmittanceWriteInfo.imageInfo = {transmittanceImageInfo};

	scatteringWriteInfo.descriptorCount = 1;
	scatteringWriteInfo.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	scatteringWriteInfo.dstBinding = 4;
	scatteringWriteInfo.dstSet = deferredInputDescriptorSet;
	scatteringWriteInfo.imageInfo = {scatteringImageInfo};

	singleMieScatteringWriteInfo.descriptorCount = 1;
	singleMieScatteringWriteInfo.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	singleMieScatteringWriteInfo.dstBinding = 5;
	singleMieScatteringWriteInfo.dstSet = deferredInputDescriptorSet;
	singleMieScatteringWriteInfo.imageInfo = {singleMieScatteringImageInfo};

	irradianceWriteInfo.descriptorCount = 1;
	irradianceWriteInfo.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	irradianceWriteInfo.dstBinding = 6;
	irradianceWriteInfo.dstSet = deferredInputDescriptorSet;
	irradianceWriteInfo.imageInfo = {irradianceImageInfo};

	sunShadowmapSamplerWriteInfo.descriptorCount = 1;
	sunShadowmapSamplerWriteInfo.descriptorType = DESCRIPTOR_TYPE_SAMPLER;
	sunShadowmapSamplerWriteInfo.dstBinding = 10;
	sunShadowmapSamplerWriteInfo.dstSet = deferredInputDescriptorSet;
	sunShadowmapSamplerWriteInfo.imageInfo = {sunShadowmapSamplerInfo};

	ditherSamplerWriteInfo.descriptorCount = 1;
	ditherSamplerWriteInfo.descriptorType = DESCRIPTOR_TYPE_SAMPLER;
	ditherSamplerWriteInfo.dstBinding = 11;
	ditherSamplerWriteInfo.dstSet = deferredInputDescriptorSet;
	ditherSamplerWriteInfo.imageInfo = {ditherTextureInfo};

	ditherTextureWriteInfo.descriptorCount = 1;
	ditherTextureWriteInfo.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	ditherTextureWriteInfo.dstBinding = 13;
	ditherTextureWriteInfo.dstSet = deferredInputDescriptorSet;
	ditherTextureWriteInfo.imageInfo = {ditherTextureInfo};

	worldEnvironmentUBOWrite.descriptorCount = 1;
	worldEnvironmentUBOWrite.descriptorType = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	worldEnvironmentUBOWrite.dstBinding = 7;
	worldEnvironmentUBOWrite.dstSet = deferredInputDescriptorSet;
	worldEnvironmentUBOWrite.bufferInfo = {worldEnvironmentUBOBufferInfo};

	linearSamplerWrite.descriptorCount = 1;
	linearSamplerWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLER;
	linearSamplerWrite.dstBinding = 12;
	linearSamplerWrite.dstSet = deferredInputDescriptorSet;
	linearSamplerWrite.imageInfo = {linearSamplerInfo};

	brdfLUTWrite.descriptorCount = 1;
	brdfLUTWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	brdfLUTWrite.dstBinding = 16;
	brdfLUTWrite.dstSet = deferredInputDescriptorSet;
	brdfLUTWrite.imageInfo = {brdfLUTImageInfo};

	engine->renderer->writeDescriptorSets({transmittanceWriteInfo, scatteringWriteInfo, singleMieScatteringWriteInfo, irradianceWriteInfo, sunShadowmapSamplerWriteInfo, ditherSamplerWriteInfo, ditherTextureWriteInfo, worldEnvironmentUBOWrite, linearSamplerWrite, brdfLUTWrite});
}

void DeferredRenderer::lightingPassDescriptorUpdate(std::map<std::string, TextureView> views, suvec3 size)
{
	renderSize = size;

	DescriptorImageInfo gbuffer0ImageInfo = {};
	gbuffer0ImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	gbuffer0ImageInfo.view = views["gbuffer0"];

	DescriptorImageInfo gbuffer1ImageInfo = {};
	gbuffer1ImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	gbuffer1ImageInfo.view = views["gbuffer1"];

	DescriptorImageInfo gbufferDepthImageInfo = {};
	gbufferDepthImageInfo.layout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	gbufferDepthImageInfo.view = views["gbufferDepth"];

	DescriptorImageInfo sunShadowmapImageInfo = {};
	sunShadowmapImageInfo.layout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	sunShadowmapImageInfo.view = views["sunShadows"];

	DescriptorImageInfo terrainShadowmapImageInfo = {};
	terrainShadowmapImageInfo.layout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	terrainShadowmapImageInfo.view = views["sunShadows"];

	DescriptorImageInfo skyCubemapImageInfo = {};
	skyCubemapImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	skyCubemapImageInfo.view = views["skybox"];

	DescriptorImageInfo skyEnviroImageInfo = {};
	skyEnviroImageInfo.layout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	skyEnviroImageInfo.view = views["enviroSkyboxSpecIBL"];

	DescriptorWriteInfo gbuffer0Write = {}, gbuffer1Write = {}, gbufferDepthWrite = {}, sunShadowmapWrite = {}, terrainShadowmapWrite = {}, skyCubemapWrite = {}, skyEnviroWrite = {};
	gbuffer0Write.descriptorCount = 1;
	gbuffer0Write.descriptorType = DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	gbuffer0Write.dstBinding = 0;
	gbuffer0Write.dstSet = deferredInputDescriptorSet;
	gbuffer0Write.imageInfo = {gbuffer0ImageInfo};

	gbuffer1Write.descriptorCount = 1;
	gbuffer1Write.descriptorType = DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	gbuffer1Write.dstBinding = 1;
	gbuffer1Write.dstSet = deferredInputDescriptorSet;
	gbuffer1Write.imageInfo = {gbuffer1ImageInfo};

	gbufferDepthWrite.descriptorCount = 1;
	gbufferDepthWrite.descriptorType = DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	gbufferDepthWrite.dstBinding = 2;
	gbufferDepthWrite.dstSet = deferredInputDescriptorSet;
	gbufferDepthWrite.imageInfo = {gbufferDepthImageInfo};
	
	sunShadowmapWrite.descriptorCount = 1;
	sunShadowmapWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	sunShadowmapWrite.dstBinding = 8;
	sunShadowmapWrite.dstSet = deferredInputDescriptorSet;
	sunShadowmapWrite.imageInfo = {sunShadowmapImageInfo};

	terrainShadowmapWrite.descriptorCount = 1;
	terrainShadowmapWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	terrainShadowmapWrite.dstBinding = 9;
	terrainShadowmapWrite.dstSet = deferredInputDescriptorSet;
	terrainShadowmapWrite.imageInfo = {terrainShadowmapImageInfo};

	skyCubemapWrite.descriptorCount = 1;
	skyCubemapWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	skyCubemapWrite.dstBinding = 14;
	skyCubemapWrite.dstSet = deferredInputDescriptorSet;
	skyCubemapWrite.imageInfo = {skyCubemapImageInfo};

	skyEnviroWrite.descriptorCount = 1;
	skyEnviroWrite.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	skyEnviroWrite.dstBinding = 15;
	skyEnviroWrite.dstSet = deferredInputDescriptorSet;
	skyEnviroWrite.imageInfo = {skyEnviroImageInfo};

	engine->renderer->writeDescriptorSets({gbuffer0Write, gbuffer1Write, gbufferDepthWrite, sunShadowmapWrite, terrainShadowmapWrite, skyCubemapWrite, skyEnviroWrite});
}

void DeferredRenderer::lightingPassRender(CommandBuffer cmdBuffer, uint32_t counter)
{
	DeferredRendererLightingPushConsts pushConstData = {};
	pushConstData.invCamMVPMat = glm::inverse(engine->api->getMainCameraProjMat() * engine->api->getMainCameraViewMat());
	pushConstData.cameraPosition = game->mainCamera.position;
	pushConstData.prjMat_aspectRatio_tanHalfFOV = glm::vec4(engine->api->getMainCameraProjMat()[2][2], engine->api->getMainCameraProjMat()[3][2], renderSize.x / float(renderSize.y), std::tan(60 * (M_PI / 180.0f) * 0.5f));
	
	cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, deferredPipeline);
	cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {deferredInputDescriptorSet});
	cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstData), &pushConstData);

	cmdBuffer->draw(6);
}

void DeferredRenderer::createDeferredLightingPipeline (RenderPass renderPass, uint32_t baseSubpass)
{
	const std::string insertMarker = "#SE_BUILTIN_INCLUDE_ATMOSPHERE_LIB";
	const std::string shaderSourceFile = "GameData/shaders/vulkan/lighting.glsl";

	std::string lightingSource = FileLoader::instance()->readFile(shaderSourceFile);
	size_t insertPos = lightingSource.find(insertMarker);

	if (insertPos != std::string::npos)
	{
		lightingSource.replace(insertPos, insertMarker.length(), atmosphere->getAtmosphericShaderLib());
	}
	else
	{
		printf("%s Didn't find location to insert atmopsheric shader lib into the deferred lighting shader\n", WARN_PREFIX);

		lightingSource.replace(insertPos, insertMarker.length(), "");
	}

	ShaderModule vertShader = engine->renderer->createShaderModuleFromSource(lightingSource, shaderSourceFile, SHADER_STAGE_VERTEX_BIT, SHADER_LANGUAGE_GLSL);
	ShaderModule fragShader = engine->renderer->createShaderModuleFromSource(lightingSource, shaderSourceFile, SHADER_STAGE_FRAGMENT_BIT, SHADER_LANGUAGE_GLSL);

	PipelineShaderStage vertShaderStage = {};
	vertShaderStage.entry = "main";
	vertShaderStage.module = vertShader;

	PipelineShaderStage fragShaderStage = {};
	fragShaderStage.entry = "main";
	fragShaderStage.module = fragShader;

	PipelineVertexInputInfo vertexInput = {};
	vertexInput.vertexInputAttribs =
	{};
	vertexInput.vertexInputBindings =
	{};

	PipelineInputAssemblyInfo inputAssembly = {};
	inputAssembly.primitiveRestart = false;
	inputAssembly.topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	PipelineViewportInfo viewportInfo = {};
	viewportInfo.scissors =
	{
		{	0, 0, 1920, 1080}};
	viewportInfo.viewports =
	{
		{	0, 0, 1920, 1080}};

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
	colorBlend.attachments =
	{	colorBlendAttachment};
	colorBlend.logicOpEnable = false;
	colorBlend.logicOp = LOGIC_OP_COPY;
	colorBlend.blendConstants[0] = 1.0f;
	colorBlend.blendConstants[1] = 1.0f;
	colorBlend.blendConstants[2] = 1.0f;
	colorBlend.blendConstants[3] = 1.0f;

	PipelineDynamicStateInfo dynamicState = {};
	dynamicState.dynamicStates =
	{	DYNAMIC_STATE_VIEWPORT, DYNAMIC_STATE_SCISSOR};

	GraphicsPipelineInfo info = {};
	info.stages =
	{	vertShaderStage, fragShaderStage};
	info.vertexInputInfo = vertexInput;
	info.inputAssemblyInfo = inputAssembly;
	info.viewportInfo = viewportInfo;
	info.rasterizationInfo = rastInfo;
	info.depthStencilInfo = depthInfo;
	info.colorBlendInfo = colorBlend;
	info.dynamicStateInfo = dynamicState;

	info.inputPushConstantRanges = {{0, lightingPcSize, SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_FRAGMENT_BIT}};
	info.inputSetLayouts = {{
		{0, DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, SHADER_STAGE_FRAGMENT_BIT},
		{1, DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, SHADER_STAGE_FRAGMENT_BIT},
		{2, DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, SHADER_STAGE_FRAGMENT_BIT},
		{3, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{4, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{5, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{6, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{7, DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_FRAGMENT_BIT},
		{8, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{9, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{10, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{11, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{12, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT},
		{13, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{14, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{15, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT},
		{16, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT}
		}};

	deferredPipeline = engine->renderer->createGraphicsPipeline(info, renderPass, baseSubpass);

	engine->renderer->destroyShaderModule(vertShaderStage.module);
	engine->renderer->destroyShaderModule(fragShaderStage.module);
}
