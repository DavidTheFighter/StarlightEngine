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
 * Octree.h
 * 
 * Created on: Oct 13, 2017
 *     Author: david
 */

#ifndef WORLD_OCTREE_H_
#define WORLD_OCTREE_H_

#include <common.h>
#include <World/BoundingBox.h>

/*
 * Specifies how an octree behaves when organizing data.
 */
typedef struct OctreeRules
{
		uint32_t minCellSize; // Preferably a factor of the octree's cell sizes, which will preferably be a factor of 2
		uint64_t maxObjectListSize; // The maximum number of objects in the cell before it begins subdividing
} OctreeRules;

/*
 * An octree data structure. Can use any type as an object, but the object type class
 * must have a function named "octreeGetPosition" that returns the position of that
 * object, as an svec4 (only the xyz components need to be valid, the w can be whatever).
 */
template<typename ObjectType>
class Octree
{
	public:

		// The world-cell coordinates, in integer form. More of a user parameter for user reference, the octree class doesn't actually touch this member anywhere
		sivec3 cellCoords;

		// The bounding box for this cell
		BoundingBox cellBB;

		// The list of objects in this cell
		std::vector<ObjectType> objectList;

		// The list of pending objects to be inserted. The tree is updated when the "flushTree" is called
		std::vector<ObjectType> pendingInsertionObjectList;

		// A pointer to this cell's parent cell
		Octree<ObjectType> *parent;

		// A list of this cell's children, of which each are considered "active" according to member "activeChildren"
		Octree<ObjectType> *children[8];

		// A bitmask indicating which children are active or currently being used
		uint8_t activeChildren;

		Octree(Octree<ObjectType> *parentCell);

		void insertObject (const ObjectType &obj);
		void insertObjects (const std::vector<ObjectType> &objs);

		void flushTreeUpdates (const OctreeRules &rules);
		void trimTree ();
};

template <typename ObjectType>
inline Octree<ObjectType>::Octree(Octree *parentCell)
{
	parent = parentCell;
	activeChildren = 0;
	memset(children, 0, sizeof(children));
}

template <typename ObjectType>
inline void Octree<ObjectType>::insertObject (const ObjectType &obj)
{
	pendingInsertionObjectList.push_back(obj);
}

template <typename ObjectType>
inline void Octree<ObjectType>::insertObjects (const std::vector<ObjectType> &objs)
{
	pendingInsertionObjectList.insert(pendingInsertionObjectList.end(), objs.begin(), objs.end());
}

/*
 * Adds all of the objects that are pending insertion into the tree, will
 * create new child nodes, and do everything an octree should.
 */
template <typename ObjectType>
inline void Octree<ObjectType>::flushTreeUpdates (const OctreeRules &rules)
{
	// If there aren't any objects pending insertion, then we don't need to do anything
	if (pendingInsertionObjectList.size() == 0)
		return;

	objectList.insert(objectList.end(), pendingInsertionObjectList.begin(), pendingInsertionObjectList.end());
	pendingInsertionObjectList.clear();

	// If this node is the smallest a node can be according to the rules, then we won't do any more
	if ((uint32_t) cellBB.sizeX() <= rules.minCellSize || (uint32_t) cellBB.sizeY() <= rules.minCellSize || (uint32_t) cellBB.sizeZ() <= rules.minCellSize)
		return;

	// If we have more objects in our cell than the rules say we can, then we'll recursively split our tree until each cell follows those rules
	if (objectList.size() > (size_t) rules.maxObjectListSize)
	{
		// A list of objects for each new child node
		std::vector<ObjectType> octList[8];
		std::vector<size_t> objectDelist;

		svec4 center = cellBB.min;
		center.x += cellBB.sizeX() * 0.5f;
		center.y += cellBB.sizeY() * 0.5f;
		center.z += cellBB.sizeZ() * 0.5f;

		// The bounding boxes for all of the child nodes
		BoundingBox octants[8];

		octants[0] = {cellBB.min, center};
		octants[1] = {{center.x, cellBB.min.y, cellBB.min.z}, {cellBB.max.x, center.y, center.z}};
		octants[2] = {{center.x, cellBB.min.y, center.z}, {cellBB.max.x, center.y, cellBB.max.z}};
		octants[3] = {{cellBB.min.x, cellBB.min.y, center.z}, {center.x, center.y, cellBB.max.z}};
		octants[4] = {{cellBB.min.x, center.y, cellBB.min.z}, {center.x, cellBB.max.y, center.z}};
		octants[5] = {{center.x, center.y, cellBB.min.z}, {cellBB.max.x, cellBB.max.y, center.z}};
		octants[6] = {center, cellBB.max};
		octants[7] = {{cellBB.min.x, center.y, center.z}, {center.x, cellBB.max.y, cellBB.max.z}};

		// Check each object to see if it fits in a child node, and if so, add it to the queue
		for (size_t i = 0; i < objectList.size(); i ++)
		{
			svec4 objectBoundingSphere = objectList[i].octreeGetBoundingSphere();

			for (int a = 0; a < 8; a ++)
			{
				if (octants[a].containsBoundingSphere(objectBoundingSphere))
				{
					octList[a].push_back(objectList[i]);
					objectDelist.push_back(i);

					break;
				}
			}
		}

		// Remove all objects from this node that are being pushed down to a child node
		while (objectDelist.size() > 0)
		{
			objectList.erase(objectList.begin() + objectDelist.back());
			objectDelist.pop_back();
		}

		// Push the queued objects down to their child nodes
		for (int a = 0; a < 8; a ++)
		{
			if (octList[a].size() > 0)
			{
				// If this child hasn't been created yet, create it
				if (!(activeChildren & (1 << a)))
				{
					activeChildren |= (1 << a);

					children[a] = new Octree(this);
					children[a]->cellBB = octants[a];
				}

				children[a]->pendingInsertionObjectList.insert(children[a]->pendingInsertionObjectList.end(), octList[a].begin(), octList[a].end());
				children[a]->flushTreeUpdates(rules);
			}
		}
	}
}

/*
 * Searches the tree for empty nodes, and deletes them.
 */
template <typename ObjectType>
inline void Octree<ObjectType>::trimTree()
{
	for (int a = 0; a < 8; a ++)
	{
		if (activeChildren & (1 << a))
		{
			DEBUG_ASSERT(children[a] != nullptr);

			// If this child has no active children itself, and no objects in it's list, then delete it
			if (children[a]->activeChildren == 0 && children[a]->objectList.size() == 0)
			{
				// Unset the bitmask flag for this child
				activeChildren &= ~(1 << a);

				delete children[a];
				children[a] = nullptr;
			}
			// If the child is active, then let that child trim it's children as well
			else
			{
				children[a]->trimTree();
			}
		}
	}
}

#endif /* WORLD_OCTREE_H_ */
