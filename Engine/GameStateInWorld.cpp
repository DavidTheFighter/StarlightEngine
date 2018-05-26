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
#include <Rendering/Deferred/DeferredRenderer.h>
#include <Rendering/PostProcess/PostProcess.h>

#include <World/WorldHandler.h>

#include <Input/Window.h>

#include <Resources/ResourceManager.h>

#include <Game/Game.h>
#include <Game/API/SEAPI.h>

GameStateInWorld::GameStateInWorld (StarlightEngine *enginePtr)
{
	engine = enginePtr;
	world = nullptr;
	worldRenderer = nullptr;
	presentSampler = nullptr;
	testGame = nullptr;
	deferredRenderer = nullptr;
	postprocess = nullptr;
}

GameStateInWorld::~GameStateInWorld ()
{

}

void GameStateInWorld::init ()
{
	EventHandler::instance()->registerObserver(EVENT_WINDOW_RESIZE, windowResizedCallback, this);

	world = new WorldHandler(engine);
	testGame = new Game(engine->mainWindow);

	Game::setInstance(testGame);

	worldRenderer = new WorldRenderer(engine, world);
	deferredRenderer = new DeferredRenderer(engine, worldRenderer);
	postprocess = new PostProcess(engine);
	deferredRenderer->game = testGame;

	engine->api->setWorldRendererPtr(worldRenderer);

	// Setup a test material
	{
		MaterialDef dirt = {};
		strcpy(dirt.uniqueName, "dirt");
		strcpy(dirt.pipelineUniqueName, "engine.defaultMaterial");
		strcpy(dirt.textureFiles[0], "GameData/textures/dirt/dirt-albedo.png");
		strcpy(dirt.textureFiles[1], "GameData/textures/dirt/dirt-normals.png");
		strcpy(dirt.textureFiles[2], "GameData/textures/dirt/dirt-roughness.png");
		strcpy(dirt.textureFiles[3], "GameData/textures/dirt/dirt-metalness.png");
		strcpy(dirt.textureFiles[4], "GameData/textures/dirt/dirt-ao.png");
		strcpy(dirt.textureFiles[5], "GameData/textures/dirt/dirt-height.png");
		strcpy(dirt.textureFiles[6], "");
		strcpy(dirt.textureFiles[7], "");

		dirt.enableAnisotropy = true;
		dirt.linearFiltering = true;
		dirt.linearMipmapFiltering = true;
		dirt.addressMode = SAMPLER_ADDRESS_MODE_REPEAT;

		engine->resources->addMaterialDef(dirt);

		MaterialDef slate = {};
		strcpy(slate.uniqueName, "slate");
		strcpy(slate.pipelineUniqueName, "engine.defaultMaterial");
		strcpy(slate.textureFiles[0], "GameData/textures/slate/slate-albedo.png");
		strcpy(slate.textureFiles[1], "GameData/textures/slate/slate-normals.png");
		strcpy(slate.textureFiles[2], "GameData/textures/slate/slate-roughness.png");
		strcpy(slate.textureFiles[3], "GameData/textures/slate/slate-metalness.png");
		strcpy(slate.textureFiles[4], "GameData/textures/slate/slate-ao.png");
		strcpy(slate.textureFiles[5], "GameData/textures/slate/slate-height.png");
		strcpy(slate.textureFiles[6], "");
		strcpy(slate.textureFiles[7], "");

		slate.enableAnisotropy = true;
		slate.linearFiltering = true;
		slate.linearMipmapFiltering = true;
		slate.addressMode = SAMPLER_ADDRESS_MODE_REPEAT;

		engine->resources->addMaterialDef(slate);
	}

	// Setup the default material pipelines
	{
		PipelineDef defaultMaterial = {};
		strcpy(defaultMaterial.uniqueName, "engine.defaultMaterial");
		strcpy(defaultMaterial.vertexShaderFile, "GameData/shaders/vulkan/defaultMaterial.glsl");
		strcpy(defaultMaterial.tessControlShaderFile, "");
		strcpy(defaultMaterial.tessEvalShaderFile, "");
		strcpy(defaultMaterial.geometryShaderFile, "");
		strcpy(defaultMaterial.fragmentShaderFile, "GameData/shaders/vulkan/defaultMaterial.glsl");
		strcpy(defaultMaterial.shadows_fragmentShaderFile, "GameData/shaders/vulkan/default-material-shadows.frag.glsl");

		defaultMaterial.clockwiseFrontFace = false;
		defaultMaterial.backfaceCulling = true;
		defaultMaterial.frontfaceCullilng = false;
		defaultMaterial.canRenderDepth = true;

		engine->resources->addPipelineDef(defaultMaterial);
	}

	// Setup a test mesh
	{
		StaticMeshDef bridge = {};
		strcpy(bridge.uniqueName, "bridge");
		bridge.meshLODFiles.push_back("GameData/meshes/test-bridge.dae");
		bridge.meshLODNames.push_back("bridge0");
		bridge.meshLODMaxDists.push_back(8192);

		engine->resources->addMeshDef(bridge);

		StaticMeshDef boulder = {};
		strcpy(boulder.uniqueName, "boulder");
		boulder.meshLODFiles.push_back("GameData/meshes/test-boulder.dae");
		boulder.meshLODNames.push_back("boulder");
		boulder.meshLODMaxDists.push_back(8192);

		engine->resources->addMeshDef(boulder);

		StaticMeshDef lodTest = {};
		strcpy(lodTest.uniqueName, "LOD Test");

		for (uint32_t i = 0; i < 5; i ++)
		{
			lodTest.meshLODFiles.push_back("GameData/meshes/lod-test.dae");
			lodTest.meshLODNames.push_back("lod" + toString(i));
		}

		lodTest.meshLODMaxDists = {512, 768, 1024, 1566, 2048};

		engine->resources->addMeshDef(lodTest);
	}

	{
		engine->resources->loadMaterialImmediate("dirt");
		engine->resources->loadMaterialImmediate("slate");
	}

	{
		engine->resources->loadStaticMeshImmediate("bridge");
		engine->resources->loadStaticMeshImmediate("boulder");
		engine->resources->loadStaticMeshImmediate("LOD Test");
	}

	LevelDef testLevel = {};
	strcpy(testLevel.uniqueName, "Test Level");

	engine->resources->addLevelDef(testLevel);

	world->setActiveLevel(engine->resources->getLevelDef(std::string(testLevel.uniqueName)));

	LevelData &dat = *world->getActiveLevelData();

	LevelStaticObjectType testObjType = {};
	testObjType.materialDefUniqueNameHash = std::hash<std::string> {} ("slate");
	testObjType.meshDefUniqueNameHash = std::hash<std::string> {} ("bridge");
	testObjType.boundingSphereRadius_maxLodDist_padding = {8, 1, 0, 0};

	LevelStaticObject testObjInstance = {};

	std::vector<LevelStaticObject> testObjs;

	for (size_t i = 0; i < 64; i ++)
	{
		testObjInstance.position_scale = {(float) (rand() % 4096), (float) (rand() % 8), (float) (rand() % 4096), 1.0f};
		testObjInstance.rotation = {0, 0, 0, 1};

		testObjs.push_back(testObjInstance);
	}

	double sT = engine->getTime();
	dat.insertStaticObjects(testObjType, testObjs);
	printf("Ins took: %f\n", (engine->getTime() - sT) * 1000.0);

	testObjs.clear();

	testObjType.materialDefUniqueNameHash = std::hash<std::string> {} ("slate");
	testObjType.meshDefUniqueNameHash = std::hash<std::string> {} ("boulder");

	for (size_t i = 0; i < 64; i ++)
	{
		testObjInstance.position_scale = {(float) (rand() % 1024) + 1024.0f, (float) (rand() % 8), (float) (rand() % 1024), 8.0f};
		testObjInstance.rotation = {0, 0, 0, 1};

		testObjs.push_back(testObjInstance);
	}

	sT = engine->getTime();
	dat.insertStaticObjects(testObjType, testObjs);
	printf("Ins took: %f\n", (engine->getTime() - sT) * 1000.0);

	testObjs.clear();

	testObjType.materialDefUniqueNameHash = std::hash<std::string> {} ("dirt");
	testObjType.meshDefUniqueNameHash = std::hash<std::string> {} ("bridge");

	for (size_t i = 0; i < 8; i ++)
	{
		testObjInstance.position_scale = {(float) (rand() % 1024), (float) (rand() % 8), (float) (rand() % 1024) + 1024.0f, 1.0f};
		testObjInstance.rotation = {0, 0, 0, 1};

		testObjs.push_back(testObjInstance);
	}

	sT = engine->getTime();
	dat.insertStaticObjects(testObjType, testObjs);
	printf("Ins took: %f\n", (engine->getTime() - sT) * 1000.0);

	testObjs.clear();

	testObjType.materialDefUniqueNameHash = std::hash<std::string> {} ("slate");
	testObjType.meshDefUniqueNameHash = std::hash<std::string> {} ("LOD Test");

	for (size_t i = 0; i < 2048; i ++)
	{
		testObjInstance.position_scale = {(float) (rand() % 8192), (float) (rand() % 8), (float) (rand() % 8192), 32.0f};
		testObjInstance.rotation = {0, 0, 0, 1};

		testObjs.push_back(testObjInstance);
	}

	sT = engine->getTime();
	dat.insertStaticObjects(testObjType, testObjs);
	printf("Ins took: %f\n", (engine->getTime() - sT) * 1000.0);

	testGame->init();

	worldRenderer->init({engine->mainWindow->getWidth(), engine->mainWindow->getHeight()});
	deferredRenderer->init();
	postprocess->init();

	deferredRenderer->setGBuffer(worldRenderer->gbufferView[0], worldRenderer->gbufferView[1], worldRenderer->gbufferView[2], {engine->mainWindow->getWidth(), engine->mainWindow->getHeight()});
	worldRenderer->setGBufferDimensions({engine->mainWindow->getWidth(), engine->mainWindow->getHeight()});
	deferredRenderer->setGBuffer(worldRenderer->gbufferView[0], worldRenderer->gbufferView[1], worldRenderer->gbufferView[2], {engine->mainWindow->getWidth(), engine->mainWindow->getHeight()});
	postprocess->setInputs(worldRenderer->gbufferView[0], worldRenderer->gbufferView[1], worldRenderer->gbufferView[2], deferredRenderer->deferredOutputView, {engine->mainWindow->getWidth(), engine->mainWindow->getHeight()});

	engine->renderer->setSwapchainTexture(engine->mainWindow, postprocess->postprocessOutputTextureView, worldRenderer->testSampler, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	presentSampler = engine->renderer->createSampler();

	engine->resources->setPipelineRenderPass(worldRenderer->gbufferRenderPass, worldRenderer->shadowsRenderPass);

	{
		engine->resources->loadPipelineImmediate("engine.defaultMaterial");
	}
}

void GameStateInWorld::destroy ()
{
	worldRenderer->destroy();
	deferredRenderer->destroy();
	postprocess->destroy();

	engine->renderer->destroySampler(presentSampler);
	engine->resources->returnMaterial("dirt");
	engine->resources->returnMaterial("slate");
	engine->resources->returnStaticMesh("bridge");
	engine->resources->returnStaticMesh("boulder");
	engine->resources->returnStaticMesh("LOD Test");

	engine->resources->returnPipeline("engine.defaultMaterial");

	delete testGame;
	delete world->getActiveLevel();
	delete world;
	delete worldRenderer;
	delete deferredRenderer;
	delete postprocess;

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

void GameStateInWorld::update (float delta)
{
	testGame->update(delta);

	glm::vec3 playerLookDir = glm::vec3(cos(testGame->mainCamera.lookAngles.y) * sin(testGame->mainCamera.lookAngles.x), sin(testGame->mainCamera.lookAngles.y), cos(testGame->mainCamera.lookAngles.y) * cos(testGame->mainCamera.lookAngles.x));
	glm::vec3 playerLookRight = glm::vec3(sin(testGame->mainCamera.lookAngles.x - M_PI * 0.5f), 0, cos(testGame->mainCamera.lookAngles.x - M_PI * 0.5f));
	glm::vec3 playerLookUp = glm::cross(playerLookRight, playerLookDir);

	glm::vec3 cameraCellOffset = glm::floor(testGame->mainCamera.position / float(LEVEL_CELL_SIZE)) * float(LEVEL_CELL_SIZE);

	worldRenderer->camViewMat = glm::lookAt(testGame->mainCamera.position - cameraCellOffset, testGame->mainCamera.position - cameraCellOffset + playerLookDir, playerLookUp);
	worldRenderer->cameraPosition = testGame->mainCamera.position;
	deferredRenderer->invCamMVPMat = glm::inverse(worldRenderer->camProjMat * worldRenderer->camViewMat);

	worldRenderer->update();
}

void GameStateInWorld::render ()
{
	worldRenderer->render3DWorld();
	deferredRenderer->renderDeferredLighting();
	postprocess->renderPostProcessing(deferredRenderer->lightingSemaphores[deferredRenderer->cmdBufferIndex]);

	engine->renderer->waitForQueueIdle(QUEUE_TYPE_GRAPHICS);
}

void GameStateInWorld::windowResizedCallback (EventWindowResizeData &eventData, void *usrPtr)
{
	GameStateInWorld *gameState = static_cast<GameStateInWorld*>(usrPtr);

	if (eventData.window == gameState->engine->mainWindow)
	{
		gameState->worldRenderer->setGBufferDimensions({eventData.width, eventData.height});
		gameState->deferredRenderer->setGBuffer(gameState->worldRenderer->gbufferView[0], gameState->worldRenderer->gbufferView[1], gameState->worldRenderer->gbufferView[2], {gameState->engine->mainWindow->getWidth(), gameState->engine->mainWindow->getHeight()});
		gameState->postprocess->setInputs(gameState->worldRenderer->gbufferView[0], gameState->worldRenderer->gbufferView[1], gameState->worldRenderer->gbufferView[2], gameState->deferredRenderer->deferredOutputView, {gameState->engine->mainWindow->getWidth(), gameState->engine->mainWindow->getHeight()});
		gameState->engine->renderer->setSwapchainTexture(gameState->engine->mainWindow, gameState->postprocess->postprocessOutputTextureView, gameState->worldRenderer->testSampler, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		//gameState->engine->renderer->setSwapchainTexture(gameState->engine->mainWindow, gameState->worldRenderer->gbuffer_AlbedoRoughnessView, gameState->presentSampler, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

