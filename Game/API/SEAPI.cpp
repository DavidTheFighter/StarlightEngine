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
 * SEAPI.cpp
 *
 *  Created on: Apr 29, 2018
 *      Author: David
 */
#include "Game/API/SEAPI.h"

#include <Engine/StarlightEngine.h>

#include <Game/Game.h>

#include <Rendering/Renderer/Renderer.h>

#include <sunpos.h>

SEAPI::SEAPI (StarlightEngine *enginePtr)
{
	engine = enginePtr;
	worldEnvironmentUBO = nullptr;
}

SEAPI::~SEAPI ()
{
	
}

/*
 * Gets the time of day in seconds. Always returns a value in range [0, SECONDS_IN_DAY)
 */
double SEAPI::getWorldTime ()
{
	return Game::instance()->worldTime;
}

/*
 * Gets the number of days since year 0.
 */
uint64_t SEAPI::getCalendarDate ()
{
	return Game::instance()->calendarDate;
}

/*
 * Returns a UBO that contains world/environment data. Updated once per frame.
 * DO NOT WRITE TO THIS BUFFER, IT IS FOR READ-ONLY PURPOSES.
 */
Buffer SEAPI::getWorldEnvironmentUBO ()
{
	return worldEnvironmentUBO;
}

glm::vec3 SEAPI::getSunDirection ()
{
	return worldEnvironmentUBOData.sunDirection;
}

void SEAPI::init ()
{
	worldEnvironmentUBO = engine->renderer->createBuffer(sizeof(WorldEnvironmentUBO), BUFFER_USAGE_UNIFORM_BUFFER_BIT, MEMORY_USAGE_CPU_ONLY, false);
}

void SEAPI::update (float delta)
{
	// Update the world/environment UBO
	{
		worldEnvironmentUBOData.worldTime = getWorldTime();

		cTime t = {};
		t.dSeconds = worldEnvironmentUBOData.worldTime;
		t.iYear = floor(getCalendarDate() / 365.0f);
		t.iMonth = 3; //floor((getCalendarDate() - t.iYear * 365.0) / 30.0);
		t.iDay = 22; //floor(getCalendarDate() -  - t.iYear * 365.0 - t.iMonth * 30.0);

		cLocation loc = {};
		loc.dLatitude = 15;
		loc.dLongitude = 90;

		cSunCoordinates coords = {};

		sunpos(t, loc, &coords);

		//	glm::vec3 playerLookDir = glm::normalize(glm::vec3(cos(game->mainCamera.lookAngles.y) * sin(game->mainCamera.lookAngles.x), sin(game->mainCamera.lookAngles.y), cos(game->mainCamera.lookAngles.y) * cos(game->mainCamera.lookAngles.x)));

		//weData.sunDirection = glm::normalize(glm::vec3(cos(coords.dZenithAngle) * sin(coords.dAzimuth), sin(coords.dZenithAngle), cos(coords.dZenithAngle) * cos(coords.dAzimuth)));
		//weData.sunDirection = glm::normalize(glm::vec3(sin(coords.dAzimuth), cos(coords.dAzimuth) * cos(coords.dZenithAngle), cos(coords.dAzimuth) * sin(coords.dZenithAngle)));
		worldEnvironmentUBOData.sunDirection = glm::normalize(
				glm::vec3(sin(coords.dAzimuth) * cos(0.5 * M_PI - coords.dZenithAngle), sin(0.5 * M_PI - coords.dZenithAngle), cos(coords.dAzimuth) * cos(0.5 * M_PI - coords.dZenithAngle)));
		//weData.sunDirection = glm::normalize(glm::vec3(-cos(weData.worldTime / float(SECONDS_IN_DAY) * M_PI * 2.0f), sin(weData.worldTime / float(SECONDS_IN_DAY) * M_PI * 2.0f), 0));

		//worldEnvironmentUBOData.sunDirection = glm::normalize(glm::vec3(-1, 1, 0.1f));

		glm::mat4 sunProj = glm::ortho<float>(-8, 8, 8, -8, -8, 8);
		glm::mat4 sunView = glm::lookAt(Game::instance()->mainCamera.position + engine->api->getSunDirection(), Game::instance()->mainCamera.position, glm::vec3(0, 1, 0));

		glm::vec3 playerLookDir = glm::vec3(cos(Game::instance()->mainCamera.lookAngles.y) * sin(Game::instance()->mainCamera.lookAngles.x), sin(Game::instance()->mainCamera.lookAngles.y), cos(Game::instance()->mainCamera.lookAngles.y) * cos(Game::instance()->mainCamera.lookAngles.x));
		glm::vec3 playerLookRight = glm::vec3(sin(Game::instance()->mainCamera.lookAngles.x - M_PI * 0.5f), 0, cos(Game::instance()->mainCamera.lookAngles.x - M_PI * 0.5f));
		glm::vec3 playerLookUp = glm::cross(playerLookRight, playerLookDir);

		glm::mat4 camViewMat = glm::lookAt(Game::instance()->mainCamera.position, Game::instance()->mainCamera.position + playerLookDir, playerLookUp);

		worldEnvironmentUBOData.sunMVP = sunProj * sunView * glm::inverse(camViewMat);

		void *uboDataPtr = engine->renderer->mapBuffer(worldEnvironmentUBO);
		memcpy(uboDataPtr, &worldEnvironmentUBOData, sizeof(WorldEnvironmentUBO));
		engine->renderer->unmapBuffer(worldEnvironmentUBO);
	}
}
