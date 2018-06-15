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
* WorldPhysics.cpp
*
* Created on: May 26, 2018
*     Author: david
*/

#include "WorldPhysics.h"

#include <Engine/StarlightEngine.h>

#include <Game/API/SEAPI.h>

#include <World/WorldHandler.h>

static PxDefaultAllocator physxAllocatorCallback;
static SEPhysXErrorCallback physxErrorCallback;
#undef NDEBUG
#undef _DEBUG

WorldPhysics::WorldPhysics(StarlightEngine *enginePtr, WorldHandler *worldHandlerPtr)
{
	engine = enginePtr;
	worldHandler = worldHandlerPtr;
	destroyed = false;

	physxFoundation = nullptr;
	pvd = nullptr;
	transport = nullptr;
	physics = nullptr;
	cooking = nullptr;

	physicsUpdateAccum = 0;
	physicsRenderDataCpy = {};
}

WorldPhysics::~WorldPhysics()
{
	if (!destroyed)
		destroy();
}

void WorldPhysics::update(float delta)
{
	const float updateFreq = 1 / 60.0f;
	LevelData *activeLvlData = worldHandler->getActiveLevelData();

	physicsUpdateAccum += delta;

	while (physicsUpdateAccum > delta)
	{
		physicsUpdateAccum -= delta;

		// Do our own buffering of debug render data, as they can't be fetched while simulating
		if (bool(engine->api->getDebugVariable("physics")))
		{
			const PxRenderBuffer &rb = activeLvlData->physScene->getRenderBuffer();

			uint32_t oldNumLines = physicsRenderDataCpy.numLines;
			physicsRenderDataCpy.numLines = rb.getNbLines();

			if (oldNumLines != physicsRenderDataCpy.numLines)
			{
				physicsRenderDataCpy.lines = std::vector<PhysicsDebugLine>(physicsRenderDataCpy.numLines);
			}

			memcpy(physicsRenderDataCpy.lines.data(), rb.getLines(), physicsRenderDataCpy.numLines * sizeof(PhysicsDebugLine));
		}

		//double sT = glfwGetTime();
		activeLvlData->physScene->simulate(updateFreq);
		activeLvlData->physScene->fetchResults(true);
		//printf("Physics took: %f ms\n", (glfwGetTime() - sT) * 1000.0);
	}
}

void WorldPhysics::init()
{
	printf("%s Initializing physics...\n", INFO_PREFIX);

	physxFoundation = PxCreateFoundation(PX_FOUNDATION_VERSION, physxAllocatorCallback, physxErrorCallback);

	if (!physxFoundation)
	{
		printf("%s Failed to create a PxFoundation!\n", ERR_PREFIX);
		exit(-1);
	}

	PxTolerancesScale tolScale = PxTolerancesScale();

	pvd = PxCreatePvd(*physxFoundation);
	transport = physx::PxDefaultPvdFileTransportCreate(std::string(engine->getWorkingDir() + "pvdTransport.pxd2").c_str());
	pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *physxFoundation, tolScale, true, pvd);

	if (!physics)
	{
		printf("%s Failed to create a PxPhysics object!\n", ERR_PREFIX);
		exit(-1);
	}

	cooking = PxCreateCooking(PX_PHYSICS_VERSION, *physxFoundation, PxCookingParams(tolScale));

	if (!cooking)
	{
		printf("%s Failed to create a PxCooking object!\n", ERR_PREFIX);
		exit(-1);
	}

	
}

RBID WorldPhysics::createHeightmapRigidBody(const HeightmapSample *samples, uint32_t cx, uint32_t cz, LevelData *lvlData, bool addToWorld)
{
	PxHeightFieldDesc hfDesc = {};
	hfDesc.format = PxHeightFieldFormat::eS16_TM;
	hfDesc.nbColumns = 257;
	hfDesc.nbRows = 257;
	hfDesc.samples.data = reinterpret_cast<const PxHeightFieldSample*> (samples);
	hfDesc.samples.stride = sizeof(PxHeightFieldSample);

	PxMaterial *mat = physics->createMaterial(0.5, 0.5, 0.5);

	PxHeightField *heightField = cooking->createHeightField(hfDesc, physics->getPhysicsInsertionCallback());
	PxHeightFieldGeometry heightFieldGeom = PxHeightFieldGeometry(heightField, PxMeshGeometryFlags(), 4096.0f / 32768.0f, 1, 1);
	PxRigidStatic *act = physics->createRigidStatic(PxTransform(PxVec3(cx * LEVEL_CELL_SIZE, -4096.0f, cz * LEVEL_CELL_SIZE)));
	PxShape *shape = physics->createShape(heightFieldGeom, *mat, true);
	act->attachShape(*shape);

	lvlData->physScene->addActor(*act);

	RigidBody *rigidBodyData = new RigidBody();
	rigidBodyData->isStatic = true;
	rigidBodyData->actor = act;
	rigidBodyData->rbid = reinterpret_cast<RBID>(act);

	act->userData = rigidBodyData;
	
	return rigidBodyData->rbid;
}

void WorldPhysics::removeRigidBody(RBID rigidBody)
{
	PxActor *act = reinterpret_cast<PxActor*>(rigidBody);
	RigidBody *rigidBodyData = reinterpret_cast<RigidBody*>(act->userData);
	
	act->getScene()->removeActor(*act);

	delete rigidBodyData;
}

void WorldPhysics::loadLevelPhysics(LevelDef *def, LevelData *lvlData)
{
	PxSceneDesc desc = PxSceneDesc(PxTolerancesScale());

	lvlData->physScene = physics->createScene(desc);

	if (false)
	{
		lvlData->physScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
		lvlData->physScene->setVisualizationParameter(PxVisualizationParameter::eWORLD_AXES, 1.0f);
		lvlData->physScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
		lvlData->physScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_STATIC, 1.0f);
		lvlData->physScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_DYNAMIC, 1.0f);

		lvlData->physScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_AABBS, 1.0f);
	}

	// Test object
	PxMaterial *mat = physics->createMaterial(0.5, 0.5, 0.5);
	PxShape *shape = physics->createShape(PxSphereGeometry(10), *mat, true);
	PxRigidStatic *act = physics->createRigidStatic(PxTransform());
	act->attachShape(*shape);

	lvlData->physScene->addActor(*act);
}


void WorldPhysics::destroyLevelPhysics(LevelDef *def, LevelData *lvlData)
{
	lvlData->physScene->release();
}

void WorldPhysics::destroy()
{
	physics->release();
	cooking->release();
	physxFoundation->release();

	destroyed = true;
}

PhysicsDebugRenderData WorldPhysics::getDebugRenderData(LevelData *lvlData)
{
	return physicsRenderDataCpy;
}

inline std::string WorldPhysics_getPxErrorCodeStr(PxErrorCode::Enum code)
{
	switch (code)
	{
		case PxErrorCode::eNO_ERROR:
			return "eNO_ERROR";
		case PxErrorCode::eDEBUG_INFO:
			return "eDEBUG_INFO";
		case PxErrorCode::eDEBUG_WARNING:
			return "eDEBUG_WARNING";
		case PxErrorCode::eINVALID_PARAMETER:
			return "eINVALID_PARAMETER";
		case PxErrorCode::eINVALID_OPERATION:
			return "eINVALID_OPERATION";
		case PxErrorCode::eOUT_OF_MEMORY:
			return "eOUT_OF_MEMORY";
		case PxErrorCode::eINTERNAL_ERROR:
			return "eINTERNAL_ERROR";
		case PxErrorCode::eABORT:
			return "eABORT";
		case PxErrorCode::ePERF_WARNING:
			return "ePERF_WARNING";
		case PxErrorCode::eMASK_ALL:
			return "eMASK_ALL";
		default:
			return "Unknown code...";
	}
}

void SEPhysXErrorCallback::reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
{
	printf("%s PhysX reported an error; File: \"%s\", line: %u, code: %s, message: %s\n", ERR_PREFIX, file, line, WorldPhysics_getPxErrorCodeStr(code).c_str(), message);
}