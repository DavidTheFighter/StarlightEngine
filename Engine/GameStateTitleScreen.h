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
 * GameStateTitleScreen.h
 * 
 * Created on: Sep 30, 2017
 *     Author: david
 */

#ifndef ENGINE_GAMESTATETITLESCREEN_H_
#define ENGINE_GAMESTATETITLESCREEN_H_

#include <common.h>
#include <Engine/GameState.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>
#include <Game/Events/EventHandler.h>

class GameStateTitleScreen : public GameState
{
	public:

		CommandPool titleScreenCommandPool;
		CommandBuffer titleScreenGUICommandBuffer;

		Texture titleScreenRT, titleScreenRTDepth;
		TextureView titleScreenRTView, titleScreenRTDepthView;
		Sampler titleScreenSampler;

		sivec2 titleScreenFramebufferSize;
		Framebuffer titleScreenFramebuffer;

		GameStateTitleScreen (StarlightEngine *enginePtr);
		virtual ~GameStateTitleScreen ();

		void init ();
		void destroy ();

		void pause ();
		void resume ();

		void handleEvents ();
		void update ();
		void render ();

		void createTitleScreenRenderTargets ();
		void destroyTitleScreenRenderTargets ();

		static void gameWindowResizedCallback (const EventWindowResizeData &eventData, void *usrPtr);

	private:

};

#endif /* ENGINE_GAMESTATETITLESCREEN_H_ */
