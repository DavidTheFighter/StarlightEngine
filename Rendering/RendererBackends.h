/*
 * RendererBackends.h
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#ifndef RENDERING_RENDERERBACKENDS_H_
#define RENDERING_RENDERERBACKENDS_H_

typedef enum RendererBackend
{
	RENDERER_BACKEND_VULKAN = 0,
	RENDERER_BACKEND_OPENGL,
	RENDERER_BACKEND_D3D12,
	RENDERER_BACKEND_MAX_ENUM
} RendererBackend;

#endif /* RENDERING_RENDERERBACKENDS_H_ */
