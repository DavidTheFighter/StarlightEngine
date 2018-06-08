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
* WorldPhysics.h
*
* Created on: May 26, 2018
*     Author: david
*/

#ifndef WORLD_PHYSICS_WORLDPHYSICS_H_
#define WORLD_PHYSICS_WORLDPHYSICS_H_

#include <common.h>

#include <Resources/Resources.h>

#include <World/LevelData.h>

#undef NDEBUG
#undef _DEBUG

#define NDEBUG

#include <PxPhysicsAPI.h>
using namespace physx;

typedef uint64_t RBID;

typedef struct
{
	int16_t height;
	unsigned char material0Index : 7;
	unsigned char tess0Bit : 1;
	unsigned char material1Index : 7;
	unsigned char reserved : 1;
} HeightmapSample;

typedef struct
{
	bool isStatic;
	PxActor *actor;
	RBID rbid;

} RigidBody;

/*
 * I did have the "PhysicsDebugRenderData" here, among others, but moved them to "common.h" to avoid some coupling issues. 
 */

class StarlightEngine;
class WorldHandler;

class WorldPhysics
{
	public:

	WorldPhysics(StarlightEngine *enginePtr, WorldHandler *worldHandlerPtr);
	virtual ~WorldPhysics();

	void update(float delta);

	void init();
	void destroy();

	void loadLevelPhysics(LevelDef *def, LevelData *lvlData);
	void destroyLevelPhysics(LevelDef *def, LevelData *lvlData);

	RBID createHeightmapRigidBody(const HeightmapSample *samples, uint32_t cx, uint32_t cz, LevelData *lvlData, bool addToWorld = true);

	void removeRigidBody(RBID rigidBody);

	PhysicsDebugRenderData getDebugRenderData(LevelData *lvlData);

	private:
	
	PhysicsDebugRenderData physicsRenderDataCpy;

	bool destroyed;

	StarlightEngine *engine;
	WorldHandler *worldHandler;

	PxFoundation *physxFoundation;
	PxPvd *pvd;
	PxPvdTransport *transport;
	PxPhysics *physics;
	PxCooking *cooking;

	float physicsUpdateAccum;

	/*
	 * A list of all rigid bodies. Note that this list will never be sized smaller, as removed rigid bodies are reused. To find
	 * unused rigid bodies, the "freeRigidBodyIndices" vector contains all rigid body indices not currently being used.
	 */
	std::vector<RigidBody> rigidBodies;
	std::vector<size_t> freeRigidBodyIndices;
};

class SEPhysXErrorCallback : public PxErrorCallback
{
	public:
	virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line);
};

#endif /* WORLD_PHYSICS_WORLDPHYSICS_H_ */

