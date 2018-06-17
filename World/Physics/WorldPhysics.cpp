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

#undef NDEBUG
#undef _DEBUG

#define NDEBUG

#include <PxPhysicsAPI.h>
using namespace physx;

class SEPhysXErrorCallback : public PxErrorCallback
{
	public:
	virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line);
};

static PxDefaultAllocator physxAllocatorCallback;
static SEPhysXErrorCallback physxErrorCallback;

WorldPhysics::WorldPhysics()
{
	destroyed = false;

	physxFoundation = nullptr;
	physics = nullptr;
	cooking = nullptr;

	physicsUpdateAccum = 0;

	sceneIDMapCounter = 0;
}

WorldPhysics::~WorldPhysics()
{
	if (!destroyed)
		destroy();
}

void WorldPhysics::updateScenePhysics(float delta, uint32_t sceneID)
{
	const float updateFreq = 1 / 60.0f;

	auto sceneMapIt = sceneIDMap.find(sceneID);

	if (sceneMapIt == sceneIDMap.end())
	{
		printf("%s Tried to update a scene that does not exist, gave scene id: %u\n", ERR_PREFIX, sceneID);

		return;
	}

	PxScene *scene = sceneMapIt->second;

	physicsUpdateAccum += delta;

	while (physicsUpdateAccum > delta)
	{
		physicsUpdateAccum -= delta;

		// Do our own buffering of debug render data, as they can't be fetched while simulating
		if (sceneDebugVisToggle[sceneID])
		{
			const PxRenderBuffer &rb = scene->getRenderBuffer();
			PhysicsDebugRenderData &physicsRenderDataCpy = sceneDebugVisInfo[sceneID];

			uint32_t oldNumLines = physicsRenderDataCpy.numLines;
			physicsRenderDataCpy.numLines = rb.getNbLines();

			if (oldNumLines != physicsRenderDataCpy.numLines)
			{
				physicsRenderDataCpy.lines = std::vector<PhysicsDebugLine>(physicsRenderDataCpy.numLines);
			}

			memcpy(physicsRenderDataCpy.lines.data(), rb.getLines(), physicsRenderDataCpy.numLines * sizeof(PhysicsDebugLine));
		}

		//double sT = glfwGetTime();
		scene->simulate(updateFreq);
		scene->fetchResults(true);
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

	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *physxFoundation, tolScale, true);

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

uint64_t WorldPhysics::createHeightmapRigidBody(const HeightmapSample *samples, uint32_t cx, uint32_t cz, uint32_t sceneID, bool addToWorld)
{
	auto sceneMapIt = sceneIDMap.find(sceneID);

	if (sceneMapIt == sceneIDMap.end())
	{
		printf("%s Tried to cook a heightmap and add it to a scene that does not exist, gave scene id: %u\n", ERR_PREFIX, sceneID);

		return std::numeric_limits<uint64_t>::max();
	}

	PxScene *scene = sceneMapIt->second;

	PxHeightFieldDesc hfDesc = {};
	hfDesc.format = PxHeightFieldFormat::eS16_TM;
	hfDesc.nbColumns = 129;
	hfDesc.nbRows = 129;
	hfDesc.samples.data = reinterpret_cast<const PxHeightFieldSample*> (samples);
	hfDesc.samples.stride = sizeof(PxHeightFieldSample);

	PxMaterial *mat = physics->createMaterial(0.5, 0.5, 0.5);

	PxHeightField *heightField = cooking->createHeightField(hfDesc, physics->getPhysicsInsertionCallback());
	PxHeightFieldGeometry heightFieldGeom = PxHeightFieldGeometry(heightField, PxMeshGeometryFlags(), 4096.0f / 32768.0f, 2, 2);
	PxRigidStatic *act = physics->createRigidStatic(PxTransform(PxVec3(cx * LEVEL_CELL_SIZE, -4096.0f, cz * LEVEL_CELL_SIZE)));
	PxShape *shape = physics->createShape(heightFieldGeom, *mat, true);
	act->attachShape(*shape);

	scene->addActor(*act);

	RigidBody *rigidBodyData = new RigidBody();
	rigidBodyData->isStatic = true;
	rigidBodyData->actor = act;
	rigidBodyData->rbid = reinterpret_cast<uint64_t>(act);

	act->userData = rigidBodyData;
	
	return rigidBodyData->rbid;
}

void WorldPhysics::removeRigidBody(uint64_t rigidBody)
{
	PxActor *act = reinterpret_cast<PxActor*>(rigidBody);
	RigidBody *rigidBodyData = reinterpret_cast<RigidBody*>(act->userData);
	
	act->getScene()->removeActor(*act);

	delete rigidBodyData;
}

void WorldPhysics::setSceneDebugVisualization(uint32_t sceneID, bool shouldVisualize)
{
	auto sceneMapIt = sceneIDMap.find(sceneID);

	if (sceneMapIt == sceneIDMap.end())
	{
		printf("%s Tried to enable debug visualization on a scene that does not exist, gave scene id: %u\n", ERR_PREFIX, sceneID);

		return;
	}

	PxScene *scene = sceneMapIt->second;
	float paramsValue = -1;

	if (!sceneDebugVisToggle[sceneID] && shouldVisualize)
	{
		paramsValue = 1;
	}
	else if (sceneDebugVisToggle[sceneID] && !shouldVisualize)
	{
		paramsValue = 0;
	}

	if (paramsValue > -1)
	{
		scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, paramsValue);
		scene->setVisualizationParameter(PxVisualizationParameter::eWORLD_AXES, paramsValue);
		scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, paramsValue);
		scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_STATIC, paramsValue);
		scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_DYNAMIC, paramsValue);

		scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_AABBS, paramsValue);
	}

	sceneDebugVisToggle[sceneID] = shouldVisualize;
}

uint32_t WorldPhysics::createPhysicsScene()
{
	PxSceneDesc desc = PxSceneDesc(PxTolerancesScale());
	PxScene *scene = physics->createScene(desc);
	uint32_t sceneID = sceneIDMapCounter;

	sceneIDMap[sceneID] = scene;
	sceneDebugVisToggle[sceneID] = false;
	sceneIDMapCounter++;
	
	return sceneID;
}

void WorldPhysics::destroyPhysicsScene(uint32_t sceneID)
{
	auto sceneMapIt = sceneIDMap.find(sceneID);

	if (sceneMapIt == sceneIDMap.end())
	{
		printf("%s Tried to destroy a scene that does not exist, gave scene id: %u\n", ERR_PREFIX, sceneID);

		return;
	}

	sceneMapIt->second->release();

	sceneIDMap.erase(sceneMapIt);
}

void WorldPhysics::destroy()
{
	for (auto it = sceneIDMap.begin(); it != sceneIDMap.end(); it++)
	{
		printf("%s Destroying an undeleted scene on shutdown, clean up yo mess! Scene ID: %u\n", WARN_PREFIX, it->first);

		destroyPhysicsScene(it->first);
	}

	physics->release();
	cooking->release();
	physxFoundation->release();

	destroyed = true;
}

const PhysicsDebugRenderData &WorldPhysics::getDebugRenderData(uint32_t sceneID)
{
	return sceneDebugVisInfo[sceneID];
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