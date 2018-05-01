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
 * SEAPI.h
 *
 *  Created on: Apr 29, 2018
 *      Author: David
 */
#ifndef GAME_API_SEAPI_H_
#define GAME_API_SEAPI_H_

#include <common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>
#include <Resources/ResourceManager.h>

class StarlightEngine;
class Game;

/*
 * Contains general data about the world/environment
 */
typedef struct
{
		glm::vec3 sunDirection;	// The direction of the sun
		float worldTime;		// The time of day in seconds
		//
} WorldEnvironmentUBO;

/*
 * This class acts as a gateway for any part of the engine (or an external library) to interface w/ the engine in a
 * "relatively clean way" (TM). It's kinda like a service locator, but maybe not as well implemented. The main intention
 * for this is to be a central hub for one part of the engine to access another w/o a crap-ton of coupling. Theoretically,
 * one can create as many SEAPI instances as needed, as they should interact with each other appropriately.
 *
 * Note none of these functions are implied to be thread safe, unless otherwise stated.
 */
class SEAPI
{
	public:
		SEAPI (StarlightEngine *enginePtr);
		virtual ~SEAPI ();

		double getWorldTime ();
		uint64_t getCalendarDate ();

		Buffer getWorldEnvironmentUBO ();

		void init ();
		void update (float delta);

	private:

		StarlightEngine *engine;

		WorldEnvironmentUBO worldEnvironmentUBOData;
		Buffer worldEnvironmentUBO;
};

#endif /* GAME_API_SEAPI_H_ */
