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
 * GameStateTitleScreen.cpp
 * 
 * Created on: Sep 30, 2017
 *     Author: david
 */

#include "Engine/GameStateTitleScreen.h"

#include <Engine/StarlightEngine.h>

#include <Rendering/Renderer/Renderer.h>
#include <Rendering/GUIRenderer.h>

#include <Input/Window.h>

GameStateTitleScreen::GameStateTitleScreen (StarlightEngine *enginePtr)
{
	engine = enginePtr;

	titleScreenCommandPool = nullptr;
	titleScreenGUICommandBuffer = nullptr;

	titleScreenRT = titleScreenRTDepth = nullptr;
	titleScreenRTView = titleScreenRTDepthView = nullptr;
	titleScreenSampler = nullptr;

	titleScreenFramebuffer = nullptr;

	EventHandler::instance()->registerObserver(EVENT_WINDOW_RESIZE, gameWindowResizedCallback, this);
}

GameStateTitleScreen::~GameStateTitleScreen ()
{
	EventHandler::instance()->removeObserver(EVENT_WINDOW_RESIZE, gameWindowResizedCallback, this);
}

void GameStateTitleScreen::init ()
{
	titleScreenCommandPool = engine->renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, 0);

	titleScreenSampler = engine->renderer->createSampler();

	createTitleScreenRenderTargets();
}

void GameStateTitleScreen::destroy ()
{
	destroyTitleScreenRenderTargets();

	engine->renderer->destroySampler(titleScreenSampler);

	engine->renderer->destroyCommandPool(titleScreenCommandPool);
}

void GameStateTitleScreen::handleEvents ()
{

}

void GameStateTitleScreen::update ()
{
	engine->guiRenderer->writeTestGUI();
}

void GameStateTitleScreen::render ()
{
	if (titleScreenGUICommandBuffer != nullptr)
		engine->renderer->freeCommandBuffer(titleScreenGUICommandBuffer);

	titleScreenGUICommandBuffer = engine->renderer->allocateCommandBuffer(titleScreenCommandPool);

	titleScreenGUICommandBuffer->beginCommands(0);

	engine->guiRenderer->recordGUIRenderCommandList(titleScreenGUICommandBuffer, titleScreenFramebuffer, titleScreenFramebufferSize.x, titleScreenFramebufferSize.y);

	titleScreenGUICommandBuffer->endCommands();

	engine->renderer->submitToQueue(QUEUE_TYPE_GRAPHICS, {titleScreenGUICommandBuffer});
	engine->renderer->waitForQueueIdle(QUEUE_TYPE_GRAPHICS);
}

void GameStateTitleScreen::pause ()
{

}

void GameStateTitleScreen::resume ()
{

}

void GameStateTitleScreen::createTitleScreenRenderTargets ()
{
	svec3 titleScreenExtent = {(float) glm::max(engine->mainWindow->getWidth(), 1u), (float) glm::max(engine->mainWindow->getHeight(), 1u), 1};

	// Usually render targets should get their own memory allocation, but for the title screen it's unimportant
	titleScreenRT = engine->renderer->createTexture(titleScreenExtent, RESOURCE_FORMAT_R8G8B8A8_UNORM, TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | TEXTURE_USAGE_SAMPLED_BIT, MEMORY_USAGE_GPU_ONLY, false);
	titleScreenRTDepth = engine->renderer->createTexture(titleScreenExtent, RESOURCE_FORMAT_D32_SFLOAT, TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | TEXTURE_USAGE_SAMPLED_BIT, MEMORY_USAGE_GPU_ONLY, false);

	titleScreenRTView = engine->renderer->createTextureView(titleScreenRT);
	titleScreenRTDepthView = engine->renderer->createTextureView(titleScreenRTDepth);

	titleScreenFramebufferSize = {(uint32_t) titleScreenExtent.x, (uint32_t) titleScreenExtent.y};
	titleScreenFramebuffer = engine->renderer->createFramebuffer(engine->guiRenderer->guiRenderPass, {titleScreenRTView, titleScreenRTDepthView}, (uint32_t) titleScreenExtent.x, (uint32_t) titleScreenExtent.y, 1);

	engine->renderer->setSwapchainTexture(titleScreenRTView, titleScreenSampler, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void GameStateTitleScreen::destroyTitleScreenRenderTargets ()
{
	engine->renderer->waitForDeviceIdle();

	engine->renderer->destroyTexture(titleScreenRT);
	engine->renderer->destroyTexture(titleScreenRTDepth);

	engine->renderer->destroyTextureView(titleScreenRTView);
	engine->renderer->destroyTextureView(titleScreenRTDepthView);

	engine->renderer->destroyFramebuffer(titleScreenFramebuffer);
}

void GameStateTitleScreen::gameWindowResizedCallback (const EventWindowResizeData &eventData, void *usrPtr)
{
	GameStateTitleScreen *instance = static_cast<GameStateTitleScreen*>(usrPtr);

	instance->destroyTitleScreenRenderTargets();
	instance->createTitleScreenRenderTargets();
}
