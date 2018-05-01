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
 * GameStateInWorld.h
 * 
 * Created on: Oct 10, 2017
 *     Author: david
 */

#ifndef ENGINE_GAMESTATEINWORLD_H_
#define ENGINE_GAMESTATEINWORLD_H_

#include <common.h>

#include <Engine/GameState.h>
#include <Game/Events/EventHandler.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

class WorldHandler;
class WorldRenderer;
class DeferredRenderer;
class PostProcess;
class Game;

class GameStateInWorld : public GameState
{
	public:

		StarlightEngine *engine;
		WorldHandler *world;
		WorldRenderer *worldRenderer;
		DeferredRenderer *deferredRenderer;
		PostProcess *postprocess;
		Game *testGame; // Probably a temp, probably will restructure this later

		GameStateInWorld (StarlightEngine *enginePtr);
		virtual ~GameStateInWorld ();

		void init ();
		void destroy ();

		void pause ();
		void resume ();

		void handleEvents ();
		void update (float delta);
		void render ();

		static void windowResizedCallback(EventWindowResizeData &eventData, void *usrPtr);

	private:

		Sampler presentSampler;
};

#endif /* ENGINE_GAMESTATEINWORLD_H_ */
