/*
 * RenderGame.cpp
 *
 *  Created on: Sep 15, 2017
 *      Author: david
 */

#include "Rendering/RenderGame.h"

#include <Game/Game.h>

RenderGame::RenderGame (Renderer *rendererBackend, ResourceManager *rendererResourceManager)
{
	renderer = rendererBackend;
	resources = rendererResourceManager;
	testGBufferRenderPass = nullptr;
	defaultMaterialPipeline = nullptr;
	testMaterialDescriptorSet = nullptr;

	gbuffer_AlbedoRoughness = nullptr;
	gbuffer_NormalMetalness = nullptr;
	gbuffer_Depth = nullptr;

	gbuffer_AlbedoRoughnessView = nullptr;
	gbuffer_NormalMetalnessView = nullptr;
	gbuffer_DepthView = nullptr;

	gbufferSampler = nullptr;

	testGBufferFramebuffer = nullptr;

	testGBufferCommandPool = nullptr;
	testGBufferCommandBuffer = nullptr;

	testMesh = nullptr;

	gbufferRenderDimensions = glm::vec2(1920, 1080);
}

CommandPool testPool;
ResourceTexture testTexture;
TextureView testTextureView;
Sampler testSampler;

RenderGame::~RenderGame ()
{
	resources->returnTexture(testTexture);

	renderer->destroyTextureView(testTextureView);
	renderer->destroySampler(testSampler);
	renderer->destroyCommandPool(testPool);

	destroyGBuffer();

	renderer->destroyCommandPool(testGBufferCommandPool);

	renderer->destroyPipeline(defaultMaterialPipeline);
	renderer->destroyDescriptorSet(testMaterialDescriptorSet);
	renderer->destroyRenderPass(testGBufferRenderPass);
}

void RenderGame::init()
{
	testPool = renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, COMMAND_POOL_TRANSIENT_BIT);

	testTexture = resources->loadTextureImmediate("/media/david/Main Disk/Programming/StarlightEngineDev/StarlightEngine/GameData/textures/test-png.png");
	testTextureView = renderer->createTextureView(testTexture->texture);
	testSampler = renderer->createSampler();

	//** ------------------------------------------------------------------------------------------------------------------------------ **/

	createRenderPass();
	createDescriptorSets();
	createGraphicsPipeline();

	createGBuffer();


	testGBufferCommandPool = renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, 0);

	createTestMesh();
	//createTestCommandBuffer();

	renderer->setSwapchainTexture(gbuffer_AlbedoRoughnessView, gbufferSampler, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	EventHandler::instance()->registerObserver(EVENT_WINDOW_RESIZE, gameWindowResizedCallback, this);
}

void RenderGame::renderGame()
{
	createTestCommandBuffer();

	renderer->submitToQueue(QUEUE_TYPE_GRAPHICS, {testGBufferCommandBuffer}, nullptr);
	renderer->waitForQueueIdle(QUEUE_TYPE_GRAPHICS);
}

void RenderGame::gameWindowResizedCallback (const EventWindowResizeData &eventData, void *usrPtr)
{
	RenderGame *inst = static_cast<RenderGame*> (usrPtr);

	if (eventData.width == 0 || eventData.height == 0)
		return;

	inst->gbufferRenderDimensions = glm::vec2(eventData.width, eventData.height);

	inst->renderer->recreateSwapchain();

	inst->destroyGBuffer();
	inst->createGBuffer();

	inst->renderer->setSwapchainTexture(inst->gbuffer_AlbedoRoughnessView, inst->gbufferSampler, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void RenderGame::createTestMesh()
{
	testMesh = resources->loadMeshImmediate("/media/david/Main Disk/Programming/StarlightEngineDev/StarlightEngine/GameData/meshes/test-bridge.dae", "bridge0");
}

void RenderGame::createTestCommandBuffer ()
{
	if (testGBufferCommandBuffer != nullptr)
	{
		renderer->freeCommandBuffer(testGBufferCommandBuffer);
	}

	testGBufferCommandBuffer = renderer->allocateCommandBuffer(testGBufferCommandPool);

	std::vector<ClearValue> clearValues = std::vector<ClearValue>(3);
	clearValues[0].color =
	{	0, 0, 0, 0};
	clearValues[1].color =
	{	0, 0, 0, 0};
	clearValues[2].depthStencil =
	{	0, 0};

	std::vector<Viewport> v = {{0, 0, gbufferRenderDimensions.x, gbufferRenderDimensions.y, 0, 1}};
	std::vector<Scissor> s = {{0, 0, gbufferRenderDimensions.x, gbufferRenderDimensions.y}};

	PipelineInputLayout pipelineInputLayout = {};
	pipelineInputLayout.pushConstantRanges = {{0, sizeof(glm::mat4), SHADER_STAGE_VERTEX_BIT}};
	pipelineInputLayout.setLayouts.push_back({{0, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT}});

	renderer->beginCommandBuffer(testGBufferCommandBuffer, COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

	renderer->cmdBeginDebugRegion(testGBufferCommandBuffer, "Render Test GBuffer", glm::vec4(0, 0.5f, 1, 1));
	renderer->cmdBeginRenderPass(testGBufferCommandBuffer, testGBufferRenderPass, testGBufferFramebuffer, s[0], clearValues, SUBPASS_CONTENTS_INLINE);
	renderer->cmdBindPipeline(testGBufferCommandBuffer, PIPELINE_BIND_POINT_GRAPHICS, defaultMaterialPipeline);

	renderer->cmdSetViewport(testGBufferCommandBuffer, 0, v);
	renderer->cmdSetScissor(testGBufferCommandBuffer, 0, s);

	glm::vec3 playerPos = Game::instance()->mainCamera.position;
	glm::vec2 playerLookAngles = Game::instance()->mainCamera.lookAngles;

	glm::vec3 playerLookDir = glm::vec3(cos(playerLookAngles.y) * sin(playerLookAngles.x), sin(playerLookAngles.y), cos(playerLookAngles.y) * cos(playerLookAngles.x));
	glm::vec3 playerLookRight = glm::vec3(sin(playerLookAngles.x - M_PI * 0.5f), 0, cos(playerLookAngles.x - M_PI * 0.5f));
	glm::vec3 playerLookUp = glm::cross(playerLookRight, playerLookDir);

	glm::mat4 proj = glm::perspective<float>(60 * (3.141f / 180.0f), 1920 / float(1080), 10000.0f, 1.0f);
	proj[1][1] *= -1;
	glm::mat4 view = glm::lookAt<float>(playerPos, playerPos + playerLookDir, playerLookUp);
	glm::mat4 mvp = proj * view;

	renderer->cmdBindDescriptorSets(testGBufferCommandBuffer, PIPELINE_BIND_POINT_GRAPHICS, pipelineInputLayout, 0, {testMaterialDescriptorSet});

	{
		renderer->cmdPushConstants(testGBufferCommandBuffer, pipelineInputLayout, SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp[0][0]);
		renderer->cmdBindIndexBuffer(testGBufferCommandBuffer, testMesh->meshBuffer, 0, false);
		renderer->cmdBindVertexBuffers(testGBufferCommandBuffer, 0, {testMesh->meshBuffer}, {testMesh->indexChunkSize});

		renderer->cmdDrawIndexed(testGBufferCommandBuffer, testMesh->faceCount * 3);
	}

	renderer->cmdEndRenderPass(testGBufferCommandBuffer);
	renderer->cmdEndDebugRegion(testGBufferCommandBuffer);
	renderer->endCommandBuffer(testGBufferCommandBuffer);
}

const ResourceFormat testAttachmentFormat = RESOURCE_FORMAT_R8G8B8A8_UNORM;
const ResourceFormat testDepthAttachmentFormat = RESOURCE_FORMAT_D32_SFLOAT;

void RenderGame::createDescriptorSets()
{
	std::vector<DescriptorSetLayoutBinding> layoutBindings;
	layoutBindings.push_back({0, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT});

	testMaterialDescriptorSet = renderer->createDescriptorSet(layoutBindings);

	DescriptorWriteInfo write0 = {};
	write0.dstSet = testMaterialDescriptorSet;
	write0.dstBinding = 0;
	write0.dstArrayElement = 0;
	write0.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write0.descriptorCount = 1;
	write0.imageInfo.push_back({testSampler, testTextureView, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

	renderer->writeDescriptorSets({write0});
}

void RenderGame::createGBuffer()
{
	svec3 extent = {gbufferRenderDimensions.x, gbufferRenderDimensions.y, 1};

	gbuffer_AlbedoRoughness = renderer->createTexture(extent, testAttachmentFormat, TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | TEXTURE_USAGE_SAMPLED_BIT, MEMORY_USAGE_GPU_ONLY, true);
	gbuffer_NormalMetalness = renderer->createTexture(extent, testAttachmentFormat, TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | TEXTURE_USAGE_SAMPLED_BIT, MEMORY_USAGE_GPU_ONLY, false);
	gbuffer_Depth = renderer->createTexture(extent, testDepthAttachmentFormat, TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | TEXTURE_USAGE_SAMPLED_BIT, MEMORY_USAGE_GPU_ONLY, false);

	gbuffer_AlbedoRoughnessView = renderer->createTextureView(gbuffer_AlbedoRoughness);
	gbuffer_NormalMetalnessView = renderer->createTextureView(gbuffer_NormalMetalness);
	gbuffer_DepthView = renderer->createTextureView(gbuffer_Depth);

	gbufferSampler = renderer->createSampler();

	{
		renderer->setObjectDebugName(gbuffer_AlbedoRoughness, OBJECT_TYPE_TEXTURE, "GBuffer - Albedo & Roughness");
		renderer->setObjectDebugName(gbuffer_NormalMetalness, OBJECT_TYPE_TEXTURE, "GBuffer - Normals & Metalness");
		renderer->setObjectDebugName(gbuffer_Depth, OBJECT_TYPE_TEXTURE, "GBuffer - Depth");

		renderer->setObjectDebugName(gbufferSampler, OBJECT_TYPE_SAMPLER, "GBuffer Sampler");
	}

	testGBufferFramebuffer = renderer->createFramebuffer(testGBufferRenderPass, {gbuffer_AlbedoRoughnessView, gbuffer_NormalMetalnessView, gbuffer_DepthView}, gbufferRenderDimensions.x, gbufferRenderDimensions.y);
}

void RenderGame::destroyGBuffer()
{
	renderer->destroyTexture(gbuffer_AlbedoRoughness);
	renderer->destroyTexture(gbuffer_NormalMetalness);
	renderer->destroyTexture(gbuffer_Depth);

	renderer->destroyTextureView(gbuffer_AlbedoRoughnessView);
	renderer->destroyTextureView(gbuffer_NormalMetalnessView);
	renderer->destroyTextureView(gbuffer_DepthView);

	renderer->destroySampler(gbufferSampler);

	renderer->destroyFramebuffer(testGBufferFramebuffer);
}

void RenderGame::createRenderPass()
{
	AttachmentDescription albedo_roughnessAttachment = {}, normal_metalnessAttachment = {}, depthAttachment = {};
	albedo_roughnessAttachment.format = testAttachmentFormat;
	albedo_roughnessAttachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
	albedo_roughnessAttachment.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedo_roughnessAttachment.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
	albedo_roughnessAttachment.storeOp = ATTACHMENT_STORE_OP_STORE;

	normal_metalnessAttachment.format = testAttachmentFormat;
	normal_metalnessAttachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
	normal_metalnessAttachment.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normal_metalnessAttachment.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
	normal_metalnessAttachment.storeOp = ATTACHMENT_STORE_OP_STORE;

	depthAttachment.format = testDepthAttachmentFormat;
	depthAttachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	depthAttachment.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = ATTACHMENT_STORE_OP_STORE;

	AttachmentReference albedo_roughnessRef = {}, normal_metalnessRef = {}, depthRef = {};
	albedo_roughnessRef.attachment = 0;
	albedo_roughnessRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	normal_metalnessRef.attachment = 1;
	normal_metalnessRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	depthRef.attachment = 2;
	depthRef.layout = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	SubpassDescription subpass0 = {};
	subpass0.bindPoint = PIPELINE_BIND_POINT_GRAPHICS;
	subpass0.colorAttachments = {albedo_roughnessRef, normal_metalnessRef};
	subpass0.depthStencilAttachment = &depthRef;

	testGBufferRenderPass = renderer->createRenderPass({albedo_roughnessAttachment, normal_metalnessAttachment, depthAttachment}, {subpass0}, {});
}

void RenderGame::createGraphicsPipeline()
{
	ShaderModule vertShader = renderer->createShaderModule("GameData/shaders/vulkan/test-platform.glsl", SHADER_STAGE_VERTEX_BIT);
	ShaderModule fragShader = renderer->createShaderModule("GameData/shaders/vulkan/test-platform.glsl", SHADER_STAGE_FRAGMENT_BIT);

	const size_t vertexStride = 44; // 44 corresponds to a vec3 vertex, vec2 uv, vec3 normal, vec3 tangent
	VertexInputBinding bindingDesc;
	bindingDesc.binding = 0;
	bindingDesc.stride = vertexStride;
	bindingDesc.inputRate = VERTEX_INPUT_RATE_VERTEX;

	PipelineShaderStage vertShaderStage = {};
	vertShaderStage.entry = "main";
	vertShaderStage.module = vertShader;

	PipelineShaderStage fragShaderStage = {};
	fragShaderStage.entry = "main";
	fragShaderStage.module = fragShader;

	std::vector<VertexInputAttrib> attribDesc = std::vector<VertexInputAttrib> (4);
	attribDesc[0].binding = 0;
	attribDesc[0].location = 0;
	attribDesc[0].format = RESOURCE_FORMAT_R32G32B32_SFLOAT;
	attribDesc[0].offset = 0;

	attribDesc[1].binding = 0;
	attribDesc[1].location = 1;
	attribDesc[1].format = RESOURCE_FORMAT_R32G32_SFLOAT;
	attribDesc[1].offset = static_cast<uint32_t> (sizeof(glm::vec3));

	attribDesc[2].binding = 0;
	attribDesc[2].location = 2;
	attribDesc[2].format = RESOURCE_FORMAT_R32G32B32_SFLOAT;
	attribDesc[2].offset = static_cast<uint32_t> (sizeof(glm::vec3) + sizeof(glm::vec2));

	attribDesc[3].binding = 0;
	attribDesc[3].location = 3;
	attribDesc[3].format = RESOURCE_FORMAT_R32G32B32_SFLOAT;
	attribDesc[3].offset = static_cast<uint32_t> (sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3));

	PipelineVertexInputInfo vertexInput = {};
	vertexInput.vertexInputAttribs = attribDesc;
	vertexInput.vertexInputBindings = {bindingDesc};

	PipelineInputAssemblyInfo inputAssembly = {};
	inputAssembly.primitiveRestart = false;
	inputAssembly.topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	PipelineViewportInfo viewportInfo = {};
	viewportInfo.scissors = {{0, 0, 1920, 1080}};
	viewportInfo.viewports = {{0, 0, 1920, 1080}};

	PipelineRasterizationInfo rastInfo = {};
	rastInfo.clockwiseFrontFace = false;
	rastInfo.cullMode = CULL_MODE_BACK_BIT;
	rastInfo.lineWidth = 1;
	rastInfo.polygonMode = POLYGON_MODE_FILL;
	rastInfo.rasterizerDiscardEnable = false;

	PipelineDepthStencilInfo depthInfo = {};
	depthInfo.enableDepthTest = true;
	depthInfo.enableDepthWrite = true;
	depthInfo.minDepthBounds = 0;
	depthInfo.maxDepthBounds = 1;
	depthInfo.depthCompareOp = COMPARE_OP_GREATER;

	PipelineColorBlendAttachment colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = false;
	colorBlendAttachment.colorWriteMask = COLOR_COMPONENT_R_BIT | COLOR_COMPONENT_G_BIT | COLOR_COMPONENT_B_BIT | COLOR_COMPONENT_A_BIT;

	PipelineColorBlendInfo colorBlend = {};
	colorBlend.attachments = {colorBlendAttachment, colorBlendAttachment};
	colorBlend.logicOpEnable = false;
	colorBlend.logicOp = LOGIC_OP_COPY;

	PipelineDynamicStateInfo dynamicState = {};
	dynamicState.dynamicStates = {DYNAMIC_STATE_VIEWPORT, DYNAMIC_STATE_SCISSOR};

	PipelineInfo info = {};
	info.stages = {vertShaderStage, fragShaderStage};
	info.vertexInputInfo = vertexInput;
	info.inputAssemblyInfo = inputAssembly;
	info.viewportInfo = viewportInfo;
	info.rasterizationInfo = rastInfo;
	info.depthStencilInfo = depthInfo;
	info.colorBlendInfo = colorBlend;
	info.dynamicStateInfo = dynamicState;

	PipelineInputLayout pipelineInputLayout = {};
	pipelineInputLayout.pushConstantRanges = {{0, sizeof(glm::mat4), SHADER_STAGE_VERTEX_BIT}};
	pipelineInputLayout.setLayouts.push_back({{0, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT}});

	defaultMaterialPipeline = renderer->createGraphicsPipeline(info, pipelineInputLayout, testGBufferRenderPass, 0);

	renderer->destroyShaderModule(vertShader);
	renderer->destroyShaderModule(fragShader);
}
