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
 * StarlightEngine.h
 * 
 * Created on: Sep 30, 2017
 *     Author: david
 */

#ifndef ENGINE_STARLIGHTENGINE_H_
#define ENGINE_STARLIGHTENGINE_H_

#include <common.h>
#include <Rendering/Renderer/RendererBackends.h>
#include <Game/Events/EventHandler.h>

#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

class Window;
class Renderer;
class GameState;
class ResourceManager;
class GUIRenderer;
class SEAPI;
class WorldPhysics;
class WorldHandler;

struct nk_context;

class StarlightEngine
{
	public:

		Renderer *renderer;
		Window *mainWindow;
		ResourceManager *resources;
		GUIRenderer *guiRenderer;
		WorldHandler *worldHandler;
		SEAPI *api;

		struct nk_context *ctx;

		RenderPass guiRenderPass;

		// The image first rendered onto the gui, usually the final combined output from the rendered 3d world

		TextureView finalOutputTextureView;
		TextureView finalOutputTextureDepthView;

		StarlightEngine (const std::vector<std::string> &launchArgs, uint32_t engineUpdateFrequencyCap);
		virtual ~StarlightEngine ();

		void init (RendererBackend rendererBackendType);
		void destroy ();

		void handleEvents ();
		void update ();
		void render ();

		void changeState (GameState *state);
		void pushState (GameState *state);
		void popState ();

		bool isRunning ();
		void quit ();

		void setGUIBackground(TextureView background);

		double getTime();
		std::string getWorkingDir();

		static void windowResizeEventCallback (const EventWindowResizeData &eventData, void *usrPtr);

	private:

		std::vector<std::string> launchArgs;
		std::vector<GameState*> gameStates;

		std::string workingDir;

		Texture finalOutputTexture;
		Texture finalOutputTextureDepth;
		Framebuffer finalOutputFramebuffer;

		TextureView guiBackgroundTextureView;
		DescriptorSet guiBackgroundDescriptor;

		Sampler guiBackgroundSampler;
		Sampler presentSampler;

		Pipeline guiTexturePassthroughPipeline;

		CommandPool engineCommandPool;
		std::vector<CommandBuffer> guiCmdBuffers;
		uint32_t cmdBufferIndex;

		double lastUpdateTime;
		float cpuTimer;
		float lastLoopCPUTime;

		bool engineIsRunning;

		// Defines an upper limit to the frequency at which the game is allowed to update. Can be pretty high without causing any trouble. Defined in Hertz
		uint32_t updateFrequencyCap;

		void createGuiRenderPass();
		void createGUITexturePassthroughPipeline();
};

#endif /* ENGINE_STARLIGHTENGINE_H_ */
