/*
 * RendererBackends.h
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#ifndef RENDERING_RENDERERBACKENDS_H_
#define RENDERING_RENDERERBACKENDS_H_

/*
 * All the different rendering backends that this engine has or might have. The only one
 * currently impelemented is Vulkan, with D3D11 being partially implemented.
 */
typedef enum RendererBackend
{
	RENDERER_BACKEND_VULKAN = 0,
	RENDERER_BACKEND_D3D11 = 1,
	RENDERER_BACKEND_D3D12 = 2,
	RENDERER_BACKEND_MAX_ENUM
} RendererBackend;

#endif /* RENDERING_RENDERERBACKENDS_H_ */
