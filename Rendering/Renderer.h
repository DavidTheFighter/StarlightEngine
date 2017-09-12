/*
 * Renderer.h
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#ifndef RENDERING_RENDERER_H_
#define RENDERING_RENDERER_H_

#include <common.h>
#include <Rendering/RendererBackends.h>

class Window;

typedef struct RendererAllocInfo
{
		RendererBackend backend;
		std::vector<std::string> launchArgs;
		Window* window;
} RendererAllocInfo;

class Renderer
{
	public:
		Renderer ();
		virtual ~Renderer ();

		virtual void initRenderer() = 0;

		static RendererBackend chooseRendererBackend (const std::vector<std::string>& launchArgs);
		static Renderer* allocateRenderer (const RendererAllocInfo& allocInfo);
};

#endif /* RENDERING_RENDERER_H_ */
