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
 * WorldHandler.cpp
 * 
 * Created on: Oct 10, 2017
 *     Author: david
 */

#include "World/WorldHandler.h"

#include <Engine/StarlightEngine.h>

WorldHandler::WorldHandler (StarlightEngine *enginePtr)
{
	engine = enginePtr;
	activeLevel = nullptr;
	activeLevelData = nullptr;
}

WorldHandler::~WorldHandler ()
{

}

const uint32_t starlightHeightmapSizes[5] = {513, 257, 129, 65, 33};

std::unique_ptr<uint16_t> WorldHandler::getCellHeightmapMipData (uint32_t cellX, uint32_t cellY, uint32_t mipLevel)
{
	size_t fileLookupPos = std::numeric_limits<size_t>::max();

	for (size_t i = 0; i < activeLevelData->heightmapFileLookupTable.size(); i ++)
	{
		const std::pair<sivec2, size_t> &lookupEntry = activeLevelData->heightmapFileLookupTable[i];

		if (lookupEntry.first.x == cellX && lookupEntry.first.y == cellY)
		{
			fileLookupPos = lookupEntry.second;

			break;
		}
	}

	if (fileLookupPos == std::numeric_limits<size_t>::max())
		return std::unique_ptr<uint16_t> (nullptr);

	size_t lookupMipOffset = 0;

	switch (mipLevel)
	{
		case 4:
			lookupMipOffset += 65 * 65 * sizeof(uint16_t);
		case 3:
			lookupMipOffset += 129 * 129 * sizeof(uint16_t);
		case 2:
			lookupMipOffset += 257 * 257 * sizeof(uint16_t);
		case 1:
			lookupMipOffset += 513 * 513 * sizeof(uint16_t);
		case 0:
			break;
		default:
			printf("%s Unknown mipmap level size (%u) requested for heightmap data\n", ERR_PREFIX, mipLevel);

			return std::unique_ptr<uint16_t> (nullptr);
	}

	std::ifstream file("GameData/levels/TestLevel/heightmap.hmp", std::ios::in | std::ios::binary | std::ios::ate);

	size_t fileSize = file.tellg();
	file.seekg(fileLookupPos + lookupMipOffset);

	std::unique_ptr<uint16_t> heightmapData(new uint16_t[starlightHeightmapSizes[mipLevel] * starlightHeightmapSizes[mipLevel]]);
	file.read(reinterpret_cast<char*> (heightmapData.get()), starlightHeightmapSizes[mipLevel] * starlightHeightmapSizes[mipLevel] * sizeof(uint16_t));

	file.close();

	return heightmapData;
}

void WorldHandler::setActiveLevel (LevelDef *level)
{
	activeLevel = level;

	if (activeLevelData != nullptr)
	{
		delete activeLevelData;
	}

	activeLevelData = new LevelData();

	// Load the lookup table for the heightmap
	{
		std::ifstream file("GameData/levels/TestLevel/heightmap.hmp", std::ios::in | std::ios::binary | std::ios::ate);
		file.seekg(4 + 4);

		file.read(reinterpret_cast<char*> (&activeLevelData->heightmapFileCellCount), 4);

		for (uint32_t i = 0; i < activeLevelData->heightmapFileCellCount; i ++)
		{
			sivec2 coord = {0, 0};
			size_t fileLookupPos = 0;

			file.read(reinterpret_cast<char*> (&coord), 8);
			file.read(reinterpret_cast<char*> (&fileLookupPos), 8);

			activeLevelData->heightmapFileLookupTable.push_back(std::make_pair(coord, fileLookupPos));
		}

		file.close();
	}
}

LevelDef *WorldHandler::getActiveLevel ()
{
	return activeLevel;
}

LevelData *WorldHandler::getActiveLevelData ()
{
	return activeLevelData;
}
