/*
 * RenderGame.h
 *
 *  Created on: Sep 15, 2017
 *      Author: david
 */

#ifndef RENDERING_RENDERGAME_H_
#define RENDERING_RENDERGAME_H_

#include <common.h>

class Renderer;

class RenderGame
{
	public:

		Renderer* renderer;

		RenderGame (Renderer* rendererBackend);
		virtual ~RenderGame ();

		void init();
		void renderGame();
};

#endif /* RENDERING_RENDERERS_RENDERGAME_H_ */
