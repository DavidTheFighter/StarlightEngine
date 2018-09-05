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
class WorldRenderer;

/*
 * Contains general data about the world/environment
 */
typedef struct
{
		glm::vec3 sunDirection;	// The direction of the sun
		float worldTime;		// The time of day in seconds
		//
		glm::mat4 sunMVPs[3];
		//
		glm::mat4 terrainSunMVPs[5];
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

		// Gets the environment UBO data. Thread safe assuming SEAPI::update() is not being called concurrently (a worker/job thread would be safe)
		Buffer getWorldEnvironmentUBO ();

		// Gets the direction of the sun. Thread safe assuming SEAPI::update() is not being called concurrently (a worker/job thread would be safe)
		glm::vec3 getSunDirection ();

		// Gets the main camera position. Thread safe assuming SEAPI::update() is not being called concurrently (a worker/job thread would be safe)
		glm::vec3 getMainCameraPosition();

		// Gets the main camera direction. Thread safe assuming SEAPI::update() is not being called concurrently (a worker/job thread would be safe)
		glm::vec3 getMainCameraDirection();

		// Gets the main camera projection matrix. Thread safe assuming SEAPI::update() is not being called concurrently (a worker/job thread would be safe)
		glm::mat4 getMainCameraProjMat();

		// Gets the main camera view matrix. Thread safe assuming SEAPI::update() is not being called concurrently (a worker/job thread would be safe)
		glm::mat4 getMainCameraViewMat();

		void init ();
		void update (float delta);

		void setWorldRendererPtr (WorldRenderer *worldRendererPtr);

		float getDebugVariable(const std::string &varName);
		void setDebugVariable(const std::string &varName, float value);

	private:

		StarlightEngine *engine;
		WorldRenderer *worldRenderer;

		glm::vec3 mainCameraPosition;
		glm::vec3 mainCameraDirection;
		glm::mat4 mainCameraProjMat;
		glm::mat4 mainCameraViewMat;

		WorldEnvironmentUBO worldEnvironmentUBOData;
		Buffer worldEnvironmentUBO;

		std::map<std::string, float> debugVariables;
};

#endif /* GAME_API_SEAPI_H_ */
