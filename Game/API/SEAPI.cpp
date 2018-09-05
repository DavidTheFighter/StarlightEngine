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
#include <Rendering/World/WorldRenderer.h>
#include <Rendering/World/CSM.h>

#include <sunpos.h>

SEAPI::SEAPI (StarlightEngine *enginePtr)
{
	engine = enginePtr;
	worldEnvironmentUBO = nullptr;
	worldRenderer = nullptr;
}

SEAPI::~SEAPI ()
{
	engine->renderer->destroyBuffer(worldEnvironmentUBO);
}

double SEAPI::getWorldTime ()
{
	return Game::instance()->worldTime;
}

uint64_t SEAPI::getCalendarDate ()
{
	return Game::instance()->calendarDate;
}

Buffer SEAPI::getWorldEnvironmentUBO ()
{
	return worldEnvironmentUBO;
}

glm::vec3 SEAPI::getSunDirection ()
{
	return worldEnvironmentUBOData.sunDirection;
}

glm::vec3 SEAPI::getMainCameraPosition()
{
	return mainCameraPosition;
}

glm::vec3 SEAPI::getMainCameraDirection()
{
	return mainCameraDirection;
}

glm::mat4 SEAPI::getMainCameraProjMat()
{
	return mainCameraProjMat;
}

glm::mat4 SEAPI::getMainCameraViewMat()
{
	return mainCameraViewMat;
}

void SEAPI::init ()
{
	worldEnvironmentUBO = engine->renderer->createBuffer(sizeof(WorldEnvironmentUBO), BUFFER_USAGE_UNIFORM_BUFFER, false, false, MEMORY_USAGE_CPU_ONLY, false);
}

void SEAPI::setWorldRendererPtr (WorldRenderer *worldRendererPtr)
{
	worldRenderer = worldRendererPtr;
}


void SEAPI::update (float delta)
{
	mainCameraPosition = Game::instance()->mainCamera.position;
	mainCameraDirection = glm::vec3(cos(Game::instance()->mainCamera.lookAngles.y) * sin(Game::instance()->mainCamera.lookAngles.x), sin(Game::instance()->mainCamera.lookAngles.y), cos(Game::instance()->mainCamera.lookAngles.y) * cos(Game::instance()->mainCamera.lookAngles.x));

	glm::vec3 playerLookRight = glm::vec3(sin(Game::instance()->mainCamera.lookAngles.x - M_PI * 0.5f), 0, cos(Game::instance()->mainCamera.lookAngles.x - M_PI * 0.5f));
	glm::vec3 playerLookUp = glm::cross(playerLookRight, mainCameraDirection);
	glm::vec3 cameraCellOffset = glm::floor((mainCameraPosition) / float(LEVEL_CELL_SIZE)) * float(LEVEL_CELL_SIZE);

	mainCameraProjMat = glm::perspective<float>(60.0f * (M_PI / 180.0f), engine->mainWindow->getWidth() / float(engine->mainWindow->getHeight()), 100000.0f, 0.1f);
	mainCameraViewMat = glm::lookAt(mainCameraPosition - cameraCellOffset, mainCameraPosition - cameraCellOffset + mainCameraDirection, playerLookUp);
	mainCameraProjMat[1][1] *= -1.0f;

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

		//worldEnvironmentUBOData.sunDirection = glm::normalize(glm::vec3(1, 1, 0.1f));

		glm::mat4 biasMatrix(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.5, 0.5, 0.0, 1.0
		);

		worldEnvironmentUBOData.sunMVPs[0] = biasMatrix * worldRenderer->sunCSM->getCamProjMat(0) * worldRenderer->sunCSM->getCamViewMat() * glm::inverse(mainCameraViewMat);
		worldEnvironmentUBOData.sunMVPs[1] = biasMatrix * worldRenderer->sunCSM->getCamProjMat(1) * worldRenderer->sunCSM->getCamViewMat() * glm::inverse(mainCameraViewMat);
		worldEnvironmentUBOData.sunMVPs[2] = biasMatrix * worldRenderer->sunCSM->getCamProjMat(2) * worldRenderer->sunCSM->getCamViewMat() * glm::inverse(mainCameraViewMat);

		//		outHeightmapTexcoord = vec3((vertex.xz + cellCoordOffset * 256.0f - (pushConsts.cameraCellCoord + vec2(cameraCellCoordOffset)) * 256.0f) / (512.0f * pow(2, clipmapArrayLayer)), clipmapArrayLayer);

		float cameraCellCoordOffsetArray[5] = {0, -1, -3, -7, -15};
		glm::mat4 terrainSunProj = glm::ortho<float>(-250, 1250, -512, 512, 1250, -250);
		terrainSunProj[1][1] *= -1;
		glm::mat4 view = glm::lookAt(-worldEnvironmentUBOData.sunDirection, glm::vec3(0), glm::vec3(0, 1, 0));

		glm::vec3 cameraCellOffset0 = glm::floor((Game::instance()->mainCamera.position - glm::vec3(LEVEL_CELL_SIZE * 0.5f)) / float(LEVEL_CELL_SIZE)) * float(LEVEL_CELL_SIZE) * glm::vec3(1, 0, 1);
		glm::vec3 cameraCellOffset1 = glm::floor(Game::instance()->mainCamera.position / float(LEVEL_CELL_SIZE)) * float(LEVEL_CELL_SIZE);
		glm::vec3 cameraCellOffset = cameraCellOffset1 - cameraCellOffset0;

		for (int i = 0; i < 5; i++)
			worldEnvironmentUBOData.terrainSunMVPs[i] = biasMatrix * terrainSunProj * view * glm::scale(glm::mat4(1), glm::vec3(1024.0f / (512.0f * glm::pow(2.0f, float(i))), 1, 1024.0f / (512.0f * glm::pow(2.0f, float(i))))) * glm::translate(glm::mat4(1), cameraCellOffset - glm::vec3(cameraCellCoordOffsetArray[i], 0, cameraCellCoordOffsetArray[i]) * float(LEVEL_CELL_SIZE)) * glm::inverse(mainCameraViewMat);

		void *uboDataPtr = engine->renderer->mapBuffer(worldEnvironmentUBO);
		memcpy(uboDataPtr, &worldEnvironmentUBOData, sizeof(WorldEnvironmentUBO));
		engine->renderer->unmapBuffer(worldEnvironmentUBO);
	}
}

float SEAPI::getDebugVariable(const std::string &varName)
{
	return debugVariables[varName];
}

void SEAPI::setDebugVariable(const std::string &varName, float value)
{
	debugVariables[varName] = value;
}