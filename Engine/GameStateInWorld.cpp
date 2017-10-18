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
 * GameStateInWorld.cpp
 * 
 * Created on: Oct 10, 2017
 *     Author: david
 */

#include "Engine/GameStateInWorld.h"

#include <Engine/StarlightEngine.h>

#include <Rendering/Renderer/Renderer.h>
#include <Rendering/World/WorldRenderer.h>

#include <World/WorldHandler.h>

#include <Input/Window.h>

#include <Resources/ResourceManager.h>

#include <Game/Game.h>

GameStateInWorld::GameStateInWorld (StarlightEngine *enginePtr)
{
	engine = enginePtr;
	world = nullptr;
	worldRenderer = nullptr;
	presentSampler = nullptr;
	testGame = nullptr;
}

GameStateInWorld::~GameStateInWorld ()
{

}

void GameStateInWorld::init ()
{
	EventHandler::instance()->registerObserver(EVENT_WINDOW_RESIZE, windowResizedCallback, this);

	world = new WorldHandler(engine);
	worldRenderer = new WorldRenderer(engine, world);
	testGame = new Game(engine->mainWindow);

	worldRenderer->init({engine->mainWindow->getWidth(), engine->mainWindow->getHeight()});
	testGame->init();

	presentSampler = engine->renderer->createSampler();

	// Setup a test material
	{
		MaterialDef dirt = {};
		strcpy(dirt.uniqueName, "dirt");
		strcpy(dirt.pipelineUniqueName, "engine.defaultMaterial");
		strcpy(dirt.textureFiles[0], "GameData/textures/dirt/dirt-albedo.png");
		strcpy(dirt.textureFiles[1], "GameData/textures/dirt/dirt-normals.png");
		strcpy(dirt.textureFiles[2], "GameData/textures/dirt/dirt-roughness.png");
		strcpy(dirt.textureFiles[3], "GameData/textures/dirt/dirt-metalness.png");
		strcpy(dirt.textureFiles[4], "");

		dirt.enableAnisotropy = true;
		dirt.linearFiltering = true;
		dirt.linearMipmapFiltering = true;
		dirt.addressMode = SAMPLER_ADDRESS_MODE_REPEAT;

		engine->resources->addMaterialDef(dirt);
	}

	// Setup a test mesh
	{
		MeshDef bridge = {};
		strcpy(bridge.uniqueName, "bridge");
		strcpy(bridge.meshFile, "GameData/meshes/test-bridge.dae");
		strcpy(bridge.meshName, "bridge0");

		engine->resources->addMeshDef(bridge);
	}

	{
		engine->resources->loadMaterialImmediate("dirt");
	}

	LevelDef testLevel = {};
	strcpy(testLevel.uniqueName, "Test Level");

	engine->resources->addLevelDef(testLevel);

	world->setActiveLevel(engine->resources->getLevelDef(std::string(testLevel.uniqueName)));

	LevelData &dat = *world->getActiveLevelData();

	LevelStaticObject testObj = {};
	testObj.boundingSphereRadius_padding = {8, 0, 0, 0};
	testObj.materialDefUniqueNameHash = std::hash<std::string> {} ("dirt");
	testObj.meshDefUniqueNameHash = std::hash<std::string> {} ("bridge");

	std::vector<LevelStaticObject> testObjs;

	for (size_t i = 0; i < 4096; i ++)
	{
		testObj.position_scale = {(float) (rand() % 16384), (float) (rand() % 8), (float) (rand() % 16384), 1.0f};
		testObj.rotation = {0, 0, 0, 1};

		testObjs.push_back(testObj);
	}

	double sT = engine->getTime();
	dat.insertStaticObjects(testObjs);
	printf("Ins took: %f\n", (engine->getTime() - sT) * 1000.0);
}

void GameStateInWorld::destroy ()
{
	worldRenderer->destroy();

	engine->renderer->destroySampler(presentSampler);

	delete testGame;
	delete world->getActiveLevel();
	delete world;
	delete worldRenderer;

	EventHandler::instance()->removeObserver(EVENT_WINDOW_RESIZE, windowResizedCallback, this);
}

void GameStateInWorld::pause ()
{

}

void GameStateInWorld::resume ()
{

}

void GameStateInWorld::handleEvents ()
{

}

void GameStateInWorld::update ()
{
	testGame->update(1 / 60.0f);

	glm::vec3 playerLookDir = glm::vec3(cos(testGame->mainCamera.lookAngles.y) * sin(testGame->mainCamera.lookAngles.x), sin(testGame->mainCamera.lookAngles.y), cos(testGame->mainCamera.lookAngles.y) * cos(testGame->mainCamera.lookAngles.x));
	glm::vec3 playerLookRight = glm::vec3(sin(testGame->mainCamera.lookAngles.x - M_PI * 0.5f), 0, cos(testGame->mainCamera.lookAngles.x - M_PI * 0.5f));
	glm::vec3 playerLookUp = glm::cross(playerLookRight, playerLookDir);

	worldRenderer->camViewMat = glm::lookAt(testGame->mainCamera.position, testGame->mainCamera.position + playerLookDir, playerLookUp);
}

void GameStateInWorld::render ()
{
	worldRenderer->render();
}

void GameStateInWorld::windowResizedCallback (EventWindowResizeData &eventData, void *usrPtr)
{
	GameStateInWorld *gameState = static_cast<GameStateInWorld*>(usrPtr);

	if (eventData.window == gameState->engine->mainWindow)
	{
		gameState->worldRenderer->setGBufferDimensions({eventData.width, eventData.height});

		//gameState->engine->renderer->setSwapchainTexture(gameState->engine->mainWindow, gameState->worldRenderer->gbuffer_AlbedoRoughnessView, gameState->presentSampler, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

