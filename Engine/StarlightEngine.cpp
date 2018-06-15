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
 * StarlightEngine.cpp
 * 
 * Created on: Sep 30, 2017
 *     Author: david
 */

#include "Engine/StarlightEngine.h"

#include <Engine/GameState.h>

#include <Input/Window.h>

#include <Rendering/Renderer/Renderer.h>
#include <Rendering/UI/GUIRenderer.h>

#include <Resources/ResourceManager.h>

#include <World/WorldHandler.h>
#include <World/Physics/WorldPhysics.h>

#include <Game/API/SEAPI.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
 //#define NK_INCLUDE_DEFAULT_FONT
 //#define NK_BUTTON_TRIGGER_ON_RELEASE

#include <nuklear.h>

StarlightEngine::StarlightEngine (const std::vector<std::string> &launchArgs, uint32_t engineUpdateFrequencyCap)
{
	engineIsRunning = false;
	updateFrequencyCap = engineUpdateFrequencyCap;

	renderer = nullptr;
	mainWindow = nullptr;
	resources = nullptr;
	guiRenderer = nullptr;
	worldHandler = nullptr;
	renderer = nullptr;
	api = nullptr;

	this->launchArgs = launchArgs;

	lastUpdateTime = 0;
	cpuTimer = 0;
	lastLoopCPUTime = 0;

	finalOutputTexture = nullptr;
	finalOutputTextureView = nullptr;

	finalOutputTextureDepth = nullptr;
	finalOutputTextureDepthView = nullptr;

	finalOutputFramebuffer = nullptr;

	guiTexturePassthroughPipeline = nullptr;
	guiBackgroundTextureView = nullptr;

	cmdBufferIndex = 0;
}

StarlightEngine::~StarlightEngine ()
{
	EventHandler::instance()->removeObserver(EVENT_WINDOW_RESIZE, windowResizeEventCallback, this);
}

void StarlightEngine::init (RendererBackend rendererBackendType)
{
	engineIsRunning = true;

#ifdef __linux__
	workingDir = "/media/david/Main Disk/Programming/StarlightEngineDev/StarlightEngine/";
#elif defined(_WIN32)
	workingDir = "A:\\Programming\\StarlightEngineDev-win\\";
#endif

	mainWindow = new Window(rendererBackendType);
	mainWindow->initWindow(0, 0, APP_NAME);

	EventHandler::instance()->registerObserver(EVENT_WINDOW_RESIZE, windowResizeEventCallback, this);

	RendererAllocInfo renderAlloc = {};
	renderAlloc.backend = rendererBackendType;
	renderAlloc.launchArgs = launchArgs;
	renderAlloc.mainWindow = mainWindow;

	renderer = Renderer::allocateRenderer(renderAlloc);
	renderer->workingDir = workingDir;
	renderer->initRenderer();

	resources = new ResourceManager(renderer, workingDir);

	createGuiRenderPass();
	createGUITexturePassthroughPipeline();

	ctx = new nk_context();
	nk_init_default(ctx, NULL);

	guiRenderer = new GUIRenderer(renderer);
	guiRenderer->temp_engine = this;
	guiRenderer->init(*ctx);

	worldHandler = new WorldHandler(this);
	worldHandler->init();

	api = new SEAPI(this);
	api->init();
	api->setDebugVariable("physics", 0);

	finalOutputTexture = renderer->createTexture({(float) mainWindow->getWidth(), (float) mainWindow->getHeight(), 1}, RESOURCE_FORMAT_R8G8B8A8_UNORM, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_COLOR_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true);
	finalOutputTextureDepth = renderer->createTexture({(float) mainWindow->getWidth(), (float) mainWindow->getHeight(), 1}, RESOURCE_FORMAT_D32_SFLOAT, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true);

	finalOutputTextureView = renderer->createTextureView(finalOutputTexture);
	finalOutputTextureDepthView = renderer->createTextureView(finalOutputTextureDepth);

	finalOutputFramebuffer = renderer->createFramebuffer(guiRenderPass, {finalOutputTextureView, finalOutputTextureDepthView}, mainWindow->getWidth(), mainWindow->getHeight());

	engineCommandPool = renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, COMMAND_POOL_RESET_COMMAND_BUFFER_BIT);
	guiCmdBuffers = engineCommandPool->allocateCommandBuffers(COMMAND_BUFFER_LEVEL_PRIMARY, 3);

	guiBackgroundDescriptor = guiRenderer->guiTextureDescriptorPool->allocateDescriptorSet();
	guiBackgroundSampler = renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	presentSampler = renderer->createSampler(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SAMPLER_FILTER_NEAREST, SAMPLER_FILTER_NEAREST);

	renderer->setSwapchainTexture(mainWindow, finalOutputTextureView, presentSampler, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void StarlightEngine::destroy ()
{
	renderer->waitForDeviceIdle();

	while (!gameStates.empty())
	{
		gameStates.back()->destroy();
		gameStates.pop_back();
	}

	renderer->destroyCommandPool(engineCommandPool);
	renderer->destroySampler(guiBackgroundSampler);
	renderer->destroySampler(presentSampler);

	renderer->destroyTexture(finalOutputTexture);
	renderer->destroyTextureView(finalOutputTextureView);
	
	renderer->destroyTexture(finalOutputTextureDepth);
	renderer->destroyTextureView(finalOutputTextureDepthView);

	renderer->destroyFramebuffer(finalOutputFramebuffer);
	renderer->destroyRenderPass(guiRenderPass);
	renderer->destroyPipeline(guiTexturePassthroughPipeline);

	worldHandler->destroy();
	guiRenderer->destroy();

	delete guiRenderer;
	delete api;
	delete resources;
	delete mainWindow;
	delete renderer;
	delete worldHandler;
}

void StarlightEngine::windowResizeEventCallback (const EventWindowResizeData &eventData, void *usrPtr)
{
	StarlightEngine *enginePtr = static_cast<StarlightEngine*>(usrPtr);

	if (enginePtr->finalOutputTexture != nullptr)
	{
		enginePtr->renderer->destroyTexture(enginePtr->finalOutputTexture);
		enginePtr->renderer->destroyTextureView(enginePtr->finalOutputTextureView);
	}

	if (enginePtr->finalOutputTextureDepth != nullptr)
	{
		enginePtr->renderer->destroyTexture(enginePtr->finalOutputTextureDepth);
		enginePtr->renderer->destroyTextureView(enginePtr->finalOutputTextureDepthView);
	}

	if (enginePtr->finalOutputFramebuffer != nullptr)
		enginePtr->renderer->destroyFramebuffer(enginePtr->finalOutputFramebuffer);

	enginePtr->finalOutputTexture = enginePtr->renderer->createTexture({(float) enginePtr->mainWindow->getWidth(), (float) enginePtr->mainWindow->getHeight(), 1}, RESOURCE_FORMAT_R8G8B8A8_UNORM, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_COLOR_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true);
	enginePtr->finalOutputTextureDepth = enginePtr->renderer->createTexture({(float) enginePtr->mainWindow->getWidth(), (float) enginePtr->mainWindow->getHeight(), 1}, RESOURCE_FORMAT_D32_SFLOAT, TEXTURE_USAGE_SAMPLED_BIT | TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, MEMORY_USAGE_GPU_ONLY, true);

	enginePtr->finalOutputTextureView = enginePtr->renderer->createTextureView(enginePtr->finalOutputTexture);
	enginePtr->finalOutputTextureDepthView = enginePtr->renderer->createTextureView(enginePtr->finalOutputTextureDepth);

	enginePtr->finalOutputFramebuffer = enginePtr->renderer->createFramebuffer(enginePtr->guiRenderPass, {enginePtr->finalOutputTextureView, enginePtr->finalOutputTextureDepthView}, enginePtr->mainWindow->getWidth(), enginePtr->mainWindow->getHeight());

	enginePtr->renderer->recreateSwapchain(enginePtr->mainWindow);
	enginePtr->renderer->setSwapchainTexture(enginePtr->mainWindow, enginePtr->finalOutputTextureView, enginePtr->presentSampler, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void StarlightEngine::handleEvents ()
{
	cpuTimer = glfwGetTime();

	mainWindow->pollEvents();

	if (mainWindow->userRequestedClose())
	{
		quit();

		return;
	}

	if (!gameStates.empty())
		gameStates.back()->handleEvents();
}

void StarlightEngine::update ()
{
	if (true)
	{
		double frameTimeTarget = 1 / double(updateFrequencyCap);

		if (getTime() - lastUpdateTime < frameTimeTarget)
		{
#ifdef __linux__
			usleep(uint32_t(std::max<double>(frameTimeTarget - (getTime() - lastUpdateTime) - 0.001, 0) * 1000000.0));
#elif defined(__WIN32)
			Sleep(DWORD(std::max<double>(frameTimeTarget - (getTime() - lastUpdateTime) - 0.001, 0) * 1000.0));
#endif
			while (getTime() - lastUpdateTime < frameTimeTarget)
			{
				// Busy wait for the rest of the time
			}
		}
	}

	float delta = getTime() - lastUpdateTime;

	if (true)
	{
		static double windowTitleFrametimeUpdateTimer;

		windowTitleFrametimeUpdateTimer += getTime() - lastUpdateTime;

		if (windowTitleFrametimeUpdateTimer > 0.3333)
		{
			windowTitleFrametimeUpdateTimer = 0;

			char windowTitle[256];
			sprintf(windowTitle, "%s (delta-t - %.3f ms, cpu-t - %.3fms)", APP_NAME, delta * 1000.0, lastLoopCPUTime * 1000.0);

			mainWindow->setTitle(windowTitle);
		}
	}

	lastUpdateTime = getTime();

	worldHandler->worldPhysics->update(delta);

	if (!gameStates.empty())
		gameStates.back()->update(delta);

	api->update(delta);
}

void StarlightEngine::render ()
{

	if (!gameStates.empty())
		gameStates.back()->render();

	renderer->waitForQueueIdle(QUEUE_TYPE_GRAPHICS);

	cmdBufferIndex ++;
	cmdBufferIndex %= guiCmdBuffers.size();

	std::vector<ClearValue> clearValues = std::vector<ClearValue>(2);
	clearValues[0].color = {0, 0.4f, 1.0f, 1};
	clearValues[1].depthStencil = {1, 0};

	uint32_t renderWidth = mainWindow->getWidth();
	uint32_t renderHeight = mainWindow->getHeight();

	guiRenderer->writeTestGUI(*ctx);

	guiCmdBuffers[cmdBufferIndex]->beginCommands(COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	guiCmdBuffers[cmdBufferIndex]->beginDebugRegion("GUI", glm::vec4(0.929f, 0.22f, 1.0f, 1.0f));
	guiCmdBuffers[cmdBufferIndex]->beginRenderPass(guiRenderPass, finalOutputFramebuffer, {0, 0, renderWidth, renderHeight}, clearValues, SUBPASS_CONTENTS_INLINE);
	guiCmdBuffers[cmdBufferIndex]->setScissors(0, {{0, 0, renderWidth, renderHeight}});
	guiCmdBuffers[cmdBufferIndex]->setViewports(0, {{0, 0, (float) renderWidth, (float) renderHeight, 0.0f, 1.0f}});

	if (guiBackgroundTextureView != nullptr)
	{
		guiCmdBuffers[cmdBufferIndex]->insertDebugMarker("GUI Background");
		guiCmdBuffers[cmdBufferIndex]->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, guiTexturePassthroughPipeline);
		guiCmdBuffers[cmdBufferIndex]->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {guiBackgroundDescriptor});

		guiCmdBuffers[cmdBufferIndex]->draw(6);
	}

	guiCmdBuffers[cmdBufferIndex]->insertDebugMarker("Nuklear GUI");
	guiRenderer->recordGUIRenderCommandList(guiCmdBuffers[cmdBufferIndex], *ctx, renderWidth, renderHeight);

	guiCmdBuffers[cmdBufferIndex]->endRenderPass();
	guiCmdBuffers[cmdBufferIndex]->endDebugRegion();
	guiCmdBuffers[cmdBufferIndex]->endCommands();

	lastLoopCPUTime = (glfwGetTime() - cpuTimer);

	renderer->submitToQueue(QUEUE_TYPE_GRAPHICS, {guiCmdBuffers[cmdBufferIndex]}, {}, {}, {});

	renderer->waitForQueueIdle(QUEUE_TYPE_GRAPHICS);

	renderer->presentToSwapchain(mainWindow);
}

void StarlightEngine::setGUIBackground(TextureView background)
{
	guiBackgroundTextureView = background;

	RendererDescriptorWriteInfo write0 = {};
	write0.dstSet = guiBackgroundDescriptor;
	write0.dstArrayElement = 0;
	write0.dstBinding = 0;
	write0.descriptorCount = 1;
	write0.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write0.imageInfo = {{guiBackgroundSampler, background, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};

	renderer->writeDescriptorSets({write0});
}

void StarlightEngine::createGuiRenderPass()
{
	AttachmentDescription colorAttachment0 = {}, depthAttachment = {};
	colorAttachment0.format = RESOURCE_FORMAT_R8G8B8A8_UNORM;
	colorAttachment0.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
	colorAttachment0.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colorAttachment0.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment0.storeOp = ATTACHMENT_STORE_OP_STORE;

	depthAttachment.format = RESOURCE_FORMAT_D32_SFLOAT;
	depthAttachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	depthAttachment.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = ATTACHMENT_STORE_OP_STORE;

	AttachmentReference colorAttachment0Ref = {}, depthAttachmentRef;
	colorAttachment0Ref.attachment = 0;
	colorAttachment0Ref.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	SubpassDescription subpass0 = {};
	subpass0.bindPoint = PIPELINE_BIND_POINT_GRAPHICS;
	subpass0.colorAttachments = {colorAttachment0Ref};
	subpass0.depthStencilAttachment = &depthAttachmentRef;

	guiRenderPass = renderer->createRenderPass({colorAttachment0, depthAttachment}, {subpass0}, {});
}

void StarlightEngine::createGUITexturePassthroughPipeline()
{
	ShaderModule vertShader = renderer->createShaderModule(getWorkingDir() + "GameData/shaders/vulkan/texture-passthrough.glsl", SHADER_STAGE_VERTEX_BIT);
	ShaderModule fragShader = renderer->createShaderModule(getWorkingDir() + "GameData/shaders/vulkan/texture-passthrough.glsl", SHADER_STAGE_FRAGMENT_BIT);

	VertexInputBinding bindingDesc;
	bindingDesc.binding = 0;
	bindingDesc.stride = static_cast<uint32_t>(sizeof(nk_vertex));
	bindingDesc.inputRate = VERTEX_INPUT_RATE_VERTEX;

	PipelineShaderStage vertShaderStage = {};
	vertShaderStage.entry = "main";
	vertShaderStage.module = vertShader;

	PipelineShaderStage fragShaderStage = {};
	fragShaderStage.entry = "main";
	fragShaderStage.module = fragShader;

	std::vector<VertexInputAttrib> attribDesc = std::vector<VertexInputAttrib>(3);
	attribDesc[0].binding = 0;
	attribDesc[0].location = 0;
	attribDesc[0].format = RESOURCE_FORMAT_R32G32_SFLOAT;
	attribDesc[0].offset = static_cast<uint32_t>(offsetof(nk_vertex, vertex));

	attribDesc[1].binding = 0;
	attribDesc[1].location = 1;
	attribDesc[1].format = RESOURCE_FORMAT_R32G32_SFLOAT;
	attribDesc[1].offset = static_cast<uint32_t>(offsetof(nk_vertex, uv));

	attribDesc[2].binding = 0;
	attribDesc[2].location = 2;
	attribDesc[2].format = RESOURCE_FORMAT_R32G32B32A32_SFLOAT;
	attribDesc[2].offset = static_cast<uint32_t>(offsetof(nk_vertex, color));

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
	rastInfo.clockwiseFrontFace = true;
	rastInfo.cullMode = CULL_MODE_BACK_BIT;
	rastInfo.lineWidth = 1;
	rastInfo.polygonMode = POLYGON_MODE_FILL;
	rastInfo.rasterizerDiscardEnable = false;

	PipelineDepthStencilInfo depthInfo = {};
	depthInfo.enableDepthTest = true;
	depthInfo.enableDepthWrite = true;
	depthInfo.minDepthBounds = 0;
	depthInfo.maxDepthBounds = 1;
	depthInfo.depthCompareOp = COMPARE_OP_LESS_OR_EQUAL;

	PipelineColorBlendAttachment colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = false;
	colorBlendAttachment.colorWriteMask = COLOR_COMPONENT_R_BIT | COLOR_COMPONENT_G_BIT | COLOR_COMPONENT_B_BIT | COLOR_COMPONENT_A_BIT;

	colorBlendAttachment.colorBlendOp = BLEND_OP_ADD;
	colorBlendAttachment.alphaBlendOp = BLEND_OP_ADD;

	colorBlendAttachment.srcAlphaBlendFactor = BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = BLEND_FACTOR_ZERO;

	colorBlendAttachment.srcColorBlendFactor = BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

	PipelineColorBlendInfo colorBlend = {};
	colorBlend.attachments =
	{colorBlendAttachment};
	colorBlend.logicOpEnable = false;
	colorBlend.logicOp = LOGIC_OP_COPY;
	colorBlend.blendConstants[0] = 1.0f;
	colorBlend.blendConstants[1] = 1.0f;
	colorBlend.blendConstants[2] = 1.0f;
	colorBlend.blendConstants[3] = 1.0f;

	PipelineDynamicStateInfo dynamicState = {};
	dynamicState.dynamicStates = {DYNAMIC_STATE_VIEWPORT, DYNAMIC_STATE_SCISSOR};

	PipelineInfo info = {};
	info.stages =
	{vertShaderStage, fragShaderStage};
	info.vertexInputInfo = vertexInput;
	info.inputAssemblyInfo = inputAssembly;
	info.viewportInfo = viewportInfo;
	info.rasterizationInfo = rastInfo;
	info.depthStencilInfo = depthInfo;
	info.colorBlendInfo = colorBlend;
	info.dynamicStateInfo = dynamicState;

	info.inputPushConstantRanges = {};
	info.inputSetLayouts = {{{0, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT}}};

	guiTexturePassthroughPipeline = renderer->createGraphicsPipeline(info, guiRenderPass, 0);

	renderer->destroyShaderModule(vertShader);
	renderer->destroyShaderModule(fragShader);
}

double StarlightEngine::getTime ()
{
	return glfwGetTime();
}

std::string StarlightEngine::getWorkingDir ()
{
	return workingDir;
}

void StarlightEngine::changeState (GameState *state)
{
	if (!gameStates.empty())
	{
		gameStates.back()->destroy();
		gameStates.pop_back();
	}

	gameStates.push_back(state);
	gameStates.back()->init();
}

void StarlightEngine::pushState (GameState *state)
{
	if (!gameStates.empty())
	{
		gameStates.back()->pause();
	}

	gameStates.push_back(state);
	gameStates.back()->init();
}

void StarlightEngine::popState ()
{
	if (!gameStates.empty())
	{
		gameStates.back()->destroy();
		gameStates.pop_back();
	}

	if (!gameStates.empty())
	{
		gameStates.back()->resume();
	}
}

bool StarlightEngine::isRunning ()
{
	return engineIsRunning;
}

void StarlightEngine::quit ()
{
	engineIsRunning = false;
}

GameState::~GameState ()
{

}
