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
 * LevelData.cpp
 * 
 * Created on: Oct 13, 2017
 *     Author: david
 */

#include "World/LevelData.h"

LevelData::LevelData ()
{
	
}

LevelData::~LevelData ()
{

}

const OctreeRules defaultOctreeRules = {1, 16};

void LevelData::insertStaticObject (const LevelStaticObjectType &objType, const LevelStaticObject &obj)
{
	int32_t cellCoordX = (int32_t) std::floor(obj.position_scale.x / float(LEVEL_CELL_SIZE));
	int32_t cellCoordY = (int32_t) std::floor(obj.position_scale.y / float(LEVEL_CELL_SIZE));
	int32_t cellCoordZ = (int32_t) std::floor(obj.position_scale.z / float(LEVEL_CELL_SIZE));

	sivec3 cellCoords = {cellCoordX, cellCoordY, cellCoordZ};

	auto cellIt = activeStaticObjectCells_map.find(cellCoords);
	size_t index;

	if (cellIt == activeStaticObjectCells_map.end())
	{
		SortedOctree<LevelStaticObjectType, LevelStaticObject> cell(nullptr);
		cell.cellBB = {{cellCoordX * float(LEVEL_CELL_SIZE), cellCoordY * float(LEVEL_CELL_SIZE), cellCoordZ * float(LEVEL_CELL_SIZE)}, {cellCoordX * float(LEVEL_CELL_SIZE) + float(LEVEL_CELL_SIZE), cellCoordY * float(LEVEL_CELL_SIZE) + float(LEVEL_CELL_SIZE), cellCoordZ * float(LEVEL_CELL_SIZE) + float(LEVEL_CELL_SIZE)}};

		activeStaticObjectCells.push_back(cell);
		activeStaticObjectCells_map[cellCoords] = activeStaticObjectCells.size() - 1;

		index = activeStaticObjectCells.size() - 1;
	}
	else
	{
		index = cellIt->second;
	}

	SortedOctree<LevelStaticObjectType, LevelStaticObject> &cell = activeStaticObjectCells[index];
	cell.insertObject(objType, obj);
	cell.flushTreeUpdates(defaultOctreeRules);
}

void LevelData::insertStaticObjects (const LevelStaticObjectType &objType, const std::vector<LevelStaticObject> &objs)
{
	std::vector<size_t> affectedCells;

	for (size_t i = 0; i < objs.size(); i ++)
	{
		int32_t cellCoordX = (int32_t) std::floor(objs[i].position_scale.x / float(LEVEL_CELL_SIZE));
		int32_t cellCoordY = (int32_t) std::floor(objs[i].position_scale.y / float(LEVEL_CELL_SIZE));
		int32_t cellCoordZ = (int32_t) std::floor(objs[i].position_scale.z / float(LEVEL_CELL_SIZE));

		sivec3 cellCoords = {cellCoordX, cellCoordY, cellCoordZ};

		auto cellIt = activeStaticObjectCells_map.find(cellCoords);

		size_t index;

		if (cellIt == activeStaticObjectCells_map.end())
		{
			SortedOctree<LevelStaticObjectType, LevelStaticObject> cell(nullptr);
			cell.cellBB = {{cellCoordX * float(LEVEL_CELL_SIZE), cellCoordY * float(LEVEL_CELL_SIZE), cellCoordZ * float(LEVEL_CELL_SIZE)}, {cellCoordX * float(LEVEL_CELL_SIZE) + float(LEVEL_CELL_SIZE), cellCoordY * float(LEVEL_CELL_SIZE) + float(LEVEL_CELL_SIZE), cellCoordZ * float(LEVEL_CELL_SIZE) + float(LEVEL_CELL_SIZE)}};

			activeStaticObjectCells.push_back(cell);
			activeStaticObjectCells_map[cellCoords] = activeStaticObjectCells.size() - 1;

			index = activeStaticObjectCells.size() - 1;
		}
		else
		{
			index = cellIt->second;
		}

		SortedOctree<LevelStaticObjectType, LevelStaticObject> &cell = activeStaticObjectCells[index];
		cell.insertObject(objType, objs[i]);

		if (std::find(affectedCells.begin(), affectedCells.end(), index) == affectedCells.end())
		{
			affectedCells.push_back(index);
		}
	}

	for (size_t i = 0; i < affectedCells.size(); i ++)
	{
		activeStaticObjectCells[affectedCells[i]].flushTreeUpdates(defaultOctreeRules);
	}
}
