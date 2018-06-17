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

namespace physx
{
	class PxFoundation;
	class PxPhysics;
	class PxCooking;
	class PxScene;
	class PxActor;
	class PxErrorCallback;

}

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
	physx::PxActor *actor;
	uint64_t rbid;

} RigidBody;

/*
 * I did have the "PhysicsDebugRenderData" here, among others, but moved them to "common.h" to avoid some coupling issues. 
 */

class WorldPhysics
{
	public:

	WorldPhysics();
	virtual ~WorldPhysics();

	void updateScenePhysics(float delta, uint32_t sceneID);

	void init();
	void destroy();

	/*
	 * Creates a new scene/world/cell/level/whatever_you_wanna_call_it, and returns it's ID. Ready to have objects and stuff
	 * added to it.
	 */
	uint32_t createPhysicsScene();

	/*
	 * Destroys a scene, releasing it from memory and removing all interal references to it. Send flowers and press F to pay respects.
	 */
	void destroyPhysicsScene(uint32_t sceneID);

	/*
	 * Cooks a heightmap cell and adds it to the scene.
	 */
	uint64_t createHeightmapRigidBody(const HeightmapSample *samples, uint32_t cx, uint32_t cz, uint32_t sceneID, bool addToWorld = true);

	void removeRigidBody(uint64_t rigidBody);

	void setSceneDebugVisualization(uint32_t sceneID, bool shouldVisualize);

	const PhysicsDebugRenderData &getDebugRenderData(uint32_t sceneID);

	private:
	
	std::map<uint32_t, physx::PxScene*> sceneIDMap;
	std::map<uint32_t, bool> sceneDebugVisToggle; // A map of scene IDs to a bool if they should have debug visualization running
	std::map<uint32_t, PhysicsDebugRenderData> sceneDebugVisInfo; // A map of scene IDs to actual visualization data, only valid if they are in debug mode

	uint32_t sceneIDMapCounter;

	bool destroyed;

	physx::PxFoundation *physxFoundation;
	physx::PxPhysics *physics;
	physx::PxCooking *cooking;

	float physicsUpdateAccum;

	/*
	 * A list of all rigid bodies. Note that this list will never be sized smaller, as removed rigid bodies are reused. To find
	 * unused rigid bodies, the "freeRigidBodyIndices" vector contains all rigid body indices not currently being used.
	 */
	std::vector<RigidBody> rigidBodies;
	std::vector<size_t> freeRigidBodyIndices;
};

#endif /* WORLD_PHYSICS_WORLDPHYSICS_H_ */

