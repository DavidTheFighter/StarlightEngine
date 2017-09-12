/*
 * OpenGLRenderer.h
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#ifndef RENDERING_OPENGL_OPENGLRENDERER_H_
#define RENDERING_OPENGL_OPENGLRENDERER_H_

#include <common.h>
#include <Rendering/Renderer.h>

class OpenGLRenderer : public Renderer
{
	public:
		OpenGLRenderer (const RendererAllocInfo& allocInfo);
		virtual ~OpenGLRenderer ();

		void initRenderer();

	private:

		RendererAllocInfo onAllocInfo;
};

#endif
