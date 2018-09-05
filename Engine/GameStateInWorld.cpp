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
	worldRenderer = nullptr;
	testGame = nullptr;
	deferredRenderer = nullptr;
	postprocess = nullptr;
	skyboxRenderer = nullptr;
	atmosphere = nullptr;

	frameGraph = std::unique_ptr<FrameGraph>(new FrameGraph(engine->renderer, {1920, 1080}));
}

GameStateInWorld::~GameStateInWorld ()
{

}

void GameStateInWorld::init ()
{
	EventHandler::instance()->registerObserver(EVENT_WINDOW_RESIZE, windowResizedCallback, this);

	testGame = new Game(engine->mainWindow);

	Game::setInstance(testGame);

	atmosphere = new AtmosphereRenderer(engine->renderer);
	worldRenderer = new WorldRenderer(engine, engine->worldHandler);
	deferredRenderer = new DeferredRenderer(engine, atmosphere);
	postprocess = new PostProcess(engine->renderer);
	deferredRenderer->game = testGame;
	skyboxRenderer = new SkyCubemapRenderer(engine->renderer, atmosphere, 256, 5);

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
		strcpy(slate.textureFiles[0], "GameData/textures/slate/slate-albedo.dds2");
		strcpy(slate.textureFiles[1], "GameData/textures/slate/slate-normals.dds2");
		strcpy(slate.textureFiles[2], "GameData/textures/slate/slate-roughness.dds2");
		strcpy(slate.textureFiles[3], "GameData/textures/slate/slate-metalness.dds2");
		strcpy(slate.textureFiles[4], "GameData/textures/slate/slate-ao.dds2");
		strcpy(slate.textureFiles[5], "GameData/textures/slate/slate-height.dds2");
		strcpy(slate.textureFiles[6], "");
		strcpy(slate.textureFiles[7], "");

		slate.enableAnisotropy = true;
		slate.linearFiltering = true;
		slate.linearMipmapFiltering = true;
		slate.addressMode = SAMPLER_ADDRESS_MODE_REPEAT;

		engine->resources->addMaterialDef(slate);

		MaterialDef pavingstones = {};
		strcpy(pavingstones.uniqueName, "pavingstones");
		strcpy(pavingstones.pipelineUniqueName, "engine.defaultMaterial");
		strcpy(pavingstones.textureFiles[0], "GameData/textures/pavingstones/pavingstones-albedo.dds2");
		strcpy(pavingstones.textureFiles[1], "GameData/textures/pavingstones/pavingstones-normals.dds2");
		strcpy(pavingstones.textureFiles[2], "GameData/textures/pavingstones/pavingstones-roughness.dds2");
		strcpy(pavingstones.textureFiles[3], "GameData/textures/black.dds");
		strcpy(pavingstones.textureFiles[4], "GameData/textures/pavingstones/pavingstones-ao.dds2");
		strcpy(pavingstones.textureFiles[5], "GameData/textures/pavingstones/pavingstones-height.dds2");
		strcpy(pavingstones.textureFiles[6], "");
		strcpy(pavingstones.textureFiles[7], "");

		pavingstones.enableAnisotropy = true;
		pavingstones.linearFiltering = true;
		pavingstones.linearMipmapFiltering = true;
		pavingstones.addressMode = SAMPLER_ADDRESS_MODE_REPEAT;

		engine->resources->addMaterialDef(pavingstones);

		MaterialDef pbrTest = {};
		strcpy(pbrTest.uniqueName, "pbrTestMat");
		strcpy(pbrTest.pipelineUniqueName, "engine.defaultMaterial");
		strcpy(pbrTest.textureFiles[0], "GameData/textures/white.dds");
		strcpy(pbrTest.textureFiles[1], "GameData/textures/test/pbr-test/pbr-normals.dds2");
		strcpy(pbrTest.textureFiles[2], "GameData/textures/test/pbr-test/pbr-roughness.dds2");
		strcpy(pbrTest.textureFiles[3], "GameData/textures/test/pbr-test/pbr-metalness.dds2");
		strcpy(pbrTest.textureFiles[4], "GameData/textures/white.dds");
		strcpy(pbrTest.textureFiles[5], "GameData/textures/white.dds");
		strcpy(pbrTest.textureFiles[6], "");
		strcpy(pbrTest.textureFiles[7], "");

		pbrTest.enableAnisotropy = true;
		pbrTest.linearFiltering = true;
		pbrTest.linearMipmapFiltering = true;
		pbrTest.addressMode = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		engine->resources->addMaterialDef(pbrTest);
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

		StaticMeshDef pbrTest = {};
		strcpy(pbrTest.uniqueName, "pbrTest");
		pbrTest.meshLODFiles.push_back("GameData/meshes/pbr-test.dae");
		pbrTest.meshLODNames.push_back("pbrSphereTest");
		pbrTest.meshLODMaxDists.push_back(8192);

		engine->resources->addMeshDef(pbrTest);

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
		engine->resources->loadMaterialImmediate("pavingstones");
		engine->resources->loadMaterialImmediate("pbrTestMat");
	}

	{
		engine->resources->loadStaticMeshImmediate("bridge");
		engine->resources->loadStaticMeshImmediate("boulder");
		engine->resources->loadStaticMeshImmediate("LOD Test");
		engine->resources->loadStaticMeshImmediate("pbrTest");
	}

	LevelDef testLevel = {};
	strcpy(testLevel.uniqueName, "Test Level");
	strcpy(testLevel.fileName, "TestLevel");

	engine->resources->addLevelDef(testLevel);
	LevelDef *lvlDef = engine->resources->getLevelDef(std::string(testLevel.uniqueName));

	engine->worldHandler->loadLevel(lvlDef);
	engine->worldHandler->setActiveLevel(lvlDef);

	LevelData &dat = *engine->worldHandler->getActiveLevelData();

	LevelStaticObjectType testObjType = {};
	testObjType.materialDefUniqueNameHash = std::hash<std::string> {} ("pavingstones");
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

	testObjType.materialDefUniqueNameHash = std::hash<std::string> {} ("pbrTestMat");
	testObjType.meshDefUniqueNameHash = std::hash<std::string> {} ("pbrTest");

	for (size_t i = 0; i < 1; i++)
	{
		testObjInstance.position_scale = {10, 0, 10, 8.0f};
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

	RenderPassAttachment gbuffer0, gbuffer1, gbufferDepth;
	gbuffer0.format = RESOURCE_FORMAT_R8G8B8A8_UNORM;
	gbuffer1.format = RESOURCE_FORMAT_R16G16B16A16_SFLOAT;
	gbufferDepth.format = RESOURCE_FORMAT_D32_SFLOAT;

	RenderPassAttachment sunShadows;
	sunShadows.arrayLayers = 3;
	sunShadows.format = RESOURCE_FORMAT_D16_UNORM;
	sunShadows.sizeX = 4096;
	sunShadows.sizeY = 4096;
	sunShadows.sizeSwapchainRelative = false;
	sunShadows.viewType = TEXTURE_VIEW_TYPE_2D_ARRAY;

	RenderPassAttachment skybox;
	skybox.format = RESOURCE_FORMAT_B10G11R11_UFLOAT_PACK32;
	skybox.arrayLayers = 6;
	skybox.sizeSwapchainRelative = false;
	skybox.sizeX = 256;
	skybox.sizeY = 256;
	skybox.viewType = TEXTURE_VIEW_TYPE_CUBE;

	RenderPassAttachment enviroSkybox;
	enviroSkybox.format = RESOURCE_FORMAT_B10G11R11_UFLOAT_PACK32;
	enviroSkybox.arrayLayers = 6;
	enviroSkybox.sizeSwapchainRelative = false;
	enviroSkybox.sizeX = 256;
	enviroSkybox.sizeY = 256;
	enviroSkybox.mipLevels = uint32_t(glm::floor(glm::log2((float) enviroSkybox.sizeX))) + 1;
	enviroSkybox.viewType = TEXTURE_VIEW_TYPE_CUBE;

	RenderPassAttachment enviroSkyboxSpecIBL;
	enviroSkyboxSpecIBL.format = RESOURCE_FORMAT_B10G11R11_UFLOAT_PACK32;
	enviroSkyboxSpecIBL.arrayLayers = 6;
	enviroSkyboxSpecIBL.sizeSwapchainRelative = false;
	enviroSkyboxSpecIBL.sizeX = skyboxRenderer->getFaceResolution();
	enviroSkyboxSpecIBL.sizeY = skyboxRenderer->getFaceResolution();
	enviroSkyboxSpecIBL.mipLevels = std::min(uint32_t(glm::floor(glm::log2((float) enviroSkybox.sizeX))) + 1, skyboxRenderer->getMaxEnviroSpecIBLMipCount());
	enviroSkyboxSpecIBL.viewType = TEXTURE_VIEW_TYPE_CUBE;

	RenderPassAttachment deferredLighting;
	deferredLighting.format = RESOURCE_FORMAT_R16G16B16A16_SFLOAT;

	RenderPassAttachment combinedFinal;
	combinedFinal.format = RESOURCE_FORMAT_R8G8B8A8_UNORM;

	FrameGraph &graph = *frameGraph;

	auto &gbuffer = graph.addRenderPass("gbuffer", FG_PIPELINE_GRAPHICS);
	gbuffer.addColorOutput("gbuffer0", gbuffer0);
	gbuffer.addColorOutput("gbuffer1", gbuffer1);
	gbuffer.setDepthStencilOutput("gbufferDepth", gbufferDepth, true, {0, 0});

	gbuffer.setInitFunction(std::bind(&WorldRenderer::gbufferPassInit, worldRenderer, std::placeholders::_1, std::placeholders::_2));
	gbuffer.setDescriptorUpdateFunction(std::bind(&WorldRenderer::gbufferPassDescriptorUpdate, worldRenderer, std::placeholders::_1, std::placeholders::_2));
	gbuffer.setRenderFunction(std::bind(&WorldRenderer::gbufferPassRender, worldRenderer, std::placeholders::_1, std::placeholders::_2));

	auto &sunShadowsGen = graph.addRenderPass("sunShadowsGen", FG_PIPELINE_GRAPHICS);
	sunShadowsGen.setDepthStencilOutput("sunShadows", sunShadows);
	sunShadowsGen.setColorOutputRenderLayerMethod("sunShadows", RP_LAYER_RENDER_IN_MULTIPLE_RENDERPASSES);

	sunShadowsGen.setInitFunction(std::bind(&WorldRenderer::sunShadowsPassInit, worldRenderer, std::placeholders::_1, std::placeholders::_2));
	sunShadowsGen.setDescriptorUpdateFunction(std::bind(&WorldRenderer::sunShadowsPassDescriptorUpdate, worldRenderer, std::placeholders::_1, std::placeholders::_2));
	sunShadowsGen.setRenderFunction(std::bind(&WorldRenderer::sunShadowsPassRender, worldRenderer, std::placeholders::_1, std::placeholders::_2));

	auto &skyboxGen = graph.addRenderPass("skyboxGen", FG_PIPELINE_GRAPHICS);
	skyboxGen.addColorOutput("skybox", skybox, false);

	skyboxGen.setInitFunction(std::bind(&SkyCubemapRenderer::skyboxGenPassInit, skyboxRenderer, std::placeholders::_1, std::placeholders::_2));
	skyboxGen.setDescriptorUpdateFunction(std::bind(&SkyCubemapRenderer::skyboxGenPassDescriptorUpdate, skyboxRenderer, std::placeholders::_1, std::placeholders::_2));
	skyboxGen.setRenderFunction(std::bind(&SkyCubemapRenderer::skyboxGenPassRender, skyboxRenderer, std::placeholders::_1, std::placeholders::_2));

	auto &enviroSkyboxGen = graph.addRenderPass("enviroSkyboxGen", FG_PIPELINE_GRAPHICS);
	enviroSkyboxGen.addColorOutput("enviroSkybox", enviroSkybox, false);
	enviroSkyboxGen.setColorOutputMipGenMethod("enviroSkybox", RP_MIP_GEN_POST_BLIT);

	enviroSkyboxGen.setInitFunction(std::bind(&SkyCubemapRenderer::enviroSkyboxGenPassInit, skyboxRenderer, std::placeholders::_1, std::placeholders::_2));
	enviroSkyboxGen.setDescriptorUpdateFunction(std::bind(&SkyCubemapRenderer::enviroSkyboxGenPassDescriptorUpdate, skyboxRenderer, std::placeholders::_1, std::placeholders::_2));
	enviroSkyboxGen.setRenderFunction(std::bind(&SkyCubemapRenderer::enviroSkyboxGenPassRender, skyboxRenderer, std::placeholders::_1, std::placeholders::_2));

	auto &enviroSkyboxSpecIBLGen = graph.addRenderPass("enviroSkyboxSpecIBLGen", FG_PIPELINE_COMPUTE);
	enviroSkyboxSpecIBLGen.addColorInput("enviroSkybox");
	enviroSkyboxSpecIBLGen.addColorOutput("enviroSkyboxSpecIBL", enviroSkyboxSpecIBL);

	enviroSkyboxSpecIBLGen.setInitFunction(std::bind(&SkyCubemapRenderer::enviroSkyboxSpecIBLGenPassInit, skyboxRenderer, std::placeholders::_1, std::placeholders::_2));
	enviroSkyboxSpecIBLGen.setDescriptorUpdateFunction(std::bind(&SkyCubemapRenderer::enviroSkyboxSpecIBLGenPassDescriptorUpdate, skyboxRenderer, std::placeholders::_1, std::placeholders::_2));
	enviroSkyboxSpecIBLGen.setRenderFunction(std::bind(&SkyCubemapRenderer::enviroSkyboxSpecIBLGenPassRender, skyboxRenderer, std::placeholders::_1, std::placeholders::_2));

	auto &lighting = graph.addRenderPass("lighting", FG_PIPELINE_GRAPHICS);
	lighting.addColorInputAttachment("gbuffer0");
	lighting.addColorInputAttachment("gbuffer1");
	lighting.addDepthStencilInputAttachment("gbufferDepth");
	lighting.addColorInput("skybox");
	lighting.addColorInput("enviroSkyboxSpecIBL");
	lighting.addDepthStencilInput("sunShadows");
	lighting.addColorOutput("deferredLighting", deferredLighting);

	lighting.setInitFunction(std::bind(&DeferredRenderer::lightingPassInit, deferredRenderer, std::placeholders::_1, std::placeholders::_2));
	lighting.setDescriptorUpdateFunction(std::bind(&DeferredRenderer::lightingPassDescriptorUpdate, deferredRenderer, std::placeholders::_1, std::placeholders::_2));
	lighting.setRenderFunction(std::bind(&DeferredRenderer::lightingPassRender, deferredRenderer, std::placeholders::_1, std::placeholders::_2));

	auto &combine_tonemap = graph.addRenderPass("combine_tonemap", FG_PIPELINE_GRAPHICS);
	combine_tonemap.addColorInput("deferredLighting");
	combine_tonemap.addColorOutput("combineFinal", combinedFinal);

	combine_tonemap.setInitFunction(std::bind(&PostProcess::combineTonemapPassInit, postprocess, std::placeholders::_1, std::placeholders::_2));
	combine_tonemap.setDescriptorUpdateFunction(std::bind(&PostProcess::combineTonemapPassDescriptorUpdate, postprocess, std::placeholders::_1, std::placeholders::_2));
	combine_tonemap.setRenderFunction(std::bind(&PostProcess::combineTonemapPassRender, postprocess, std::placeholders::_1, std::placeholders::_2));

	graph.setBackbufferSource("combineFinal");

	sT = engine->getTime();
	graph.build();
	printf("build took %fms\n", (engine->getTime() - sT) * 1000.0);

	//auto &gui = graph.addRenderPass("gui", FG_PIPELINE_GRAPHICS);
	//gui.addColorOutput("combineFinal", combinedFinal);

	//worldRenderer->init({engine->mainWindow->getWidth(), engine->mainWindow->getHeight()});
	//deferredRenderer->init();
	//postprocess->init();

	//deferredRenderer->setGBuffer(worldRenderer->gbufferView[0], worldRenderer->gbufferView[1], worldRenderer->gbufferView[2], {engine->mainWindow->getWidth(), engine->mainWindow->getHeight()});
	//worldRenderer->setGBufferDimensions({engine->mainWindow->getWidth(), engine->mainWindow->getHeight()});
	//deferredRenderer->setGBuffer(worldRenderer->gbufferView[0], worldRenderer->gbufferView[1], worldRenderer->gbufferView[2], {engine->mainWindow->getWidth(), engine->mainWindow->getHeight()});
	//postprocess->setInputs(worldRenderer->gbufferView[0], worldRenderer->gbufferView[1], worldRenderer->gbufferView[2], deferredRenderer->deferredOutputView, {engine->mainWindow->getWidth(), engine->mainWindow->getHeight()});

	//engine->renderer->setSwapchainTexture(engine->mainWindow, postprocess->postprocessOutputTextureView, worldRenderer->testSampler, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	engine->setGUIBackground(graph.getBackbufferSourceTextureView());

	engine->resources->setPipelineRenderPass(worldRenderer->gbufferRenderPass, worldRenderer->shadowsRenderPass);

	{
		engine->resources->loadPipelineImmediate("engine.defaultMaterial");
	}
}

void GameStateInWorld::destroy ()
{
	engine->worldHandler->unloadLevel(engine->resources->getLevelDef("Test Level"));

	engine->resources->returnMaterial("dirt");
	engine->resources->returnMaterial("slate");
	engine->resources->returnMaterial("pavingstones");
	engine->resources->returnMaterial("pbrTestMat");

	engine->resources->returnStaticMesh("bridge");
	engine->resources->returnStaticMesh("boulder");
	engine->resources->returnStaticMesh("LOD Test");
	engine->resources->returnStaticMesh("pbrTest");

	engine->resources->returnPipeline("engine.defaultMaterial");

	delete testGame;
	delete worldRenderer;
	delete deferredRenderer;
	delete postprocess;
	delete atmosphere;
	frameGraph.release();

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

	worldRenderer->update();
	skyboxRenderer->setSunDirection(engine->api->getSunDirection());
}

void GameStateInWorld::render ()
{
	frameGraph->execute();
}

void GameStateInWorld::windowResizedCallback (EventWindowResizeData &eventData, void *usrPtr)
{
	GameStateInWorld *gameState = static_cast<GameStateInWorld*>(usrPtr);

	if (eventData.window == gameState->engine->mainWindow)
	{
		float resMult = 1;// 1920 / 2560.0f;
		suvec2 scaledRes = {(uint32_t) round(eventData.width * resMult), (uint32_t) round(eventData.height * resMult)};

		//gameState->worldRenderer->setGBufferDimensions({scaledRes.x, scaledRes.y});
		//gameState->deferredRenderer->setGBuffer(gameState->worldRenderer->gbufferView[0], gameState->worldRenderer->gbufferView[1], gameState->worldRenderer->gbufferView[2], {scaledRes.x, scaledRes.y});
	//	gameState->postprocess->setInputs(gameState->worldRenderer->gbufferView[0], gameState->worldRenderer->gbufferView[1], gameState->worldRenderer->gbufferView[2], gameState->deferredRenderer->deferredOutputView, {scaledRes.x, scaledRes.y});
		
		//gameState->engine->renderer->setSwapchainTexture(gameState->engine->mainWindow, gameState->postprocess->postprocessOutputTextureView, gameState->worldRenderer->testSampler, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		//gameState->engine->renderer->setSwapchainTexture(gameState->engine->mainWindow, gameState->worldRenderer->gbuffer_AlbedoRoughnessView, gameState->presentSampler, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

