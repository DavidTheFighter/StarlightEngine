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
 * WorldHandler.h
 * 
 * Created on: Oct 10, 2017
 *     Author: david
 */

#ifndef WORLD_WORLDHANDLER_H_
#define WORLD_WORLDHANDLER_H_

#include <common.h>
#include <Resources/Resources.h>
#include <World/LevelData.h>

class StarlightEngine;
class WorldPhysics;

class WorldHandler
{
	public:

		StarlightEngine *engine;
		WorldPhysics *worldPhysics;

		WorldHandler (StarlightEngine *enginePtr);
		virtual ~WorldHandler ();

		void init();
		void destroy();

		void loadLevel(LevelDef *level);
		void unloadLevel(LevelDef *level);
		void setActiveLevel (LevelDef *level);

		LevelData *getLevelData(LevelDef *level);

		LevelDef *getActiveLevel ();
		LevelData *getActiveLevelData ();

		/*
		 * Passthrough for WorldPhysics::getRenderDebugData(..)
		 */
		PhysicsDebugRenderData getDebugRenderData(LevelData *lvlData);

		std::unique_ptr<uint16_t> getCellHeightmapMipData(uint32_t cellX, uint32_t cellY, uint32_t mipLevel);
		std::unique_ptr<uint16_t> getCellHeightmapMipData(LevelData *lvlData, uint32_t cellX, uint32_t cellY, uint32_t mipLevel);

	private:

		bool destroyed;

		LevelDef *activeLevel;
		LevelData *activeLevelData;

		std::map<LevelDef*, LevelData*> loadedLevels; // List of loaded levels, w/ full data & ready to be simulated
};

#endif /* WORLD_WORLDHANDLER_H_ */
