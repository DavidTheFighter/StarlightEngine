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
 * SortedOctree.h
 * 
 * Created on: Oct 17, 2017
 *     Author: david
 */

#ifndef WORLD_SORTEDOCTREE_H_
#define WORLD_SORTEDOCTREE_H_

#include <common.h>
#include <World/BoundingBox.h>
#include <World/Octree.h>

/*
 * An sorted octree data structure. It differs from a normal octree in that instead of
 * each cell simply containing a list of objects and their positions, each cell now
 * contains a series of lists sorted by object types.
 */
template<typename ObjectListKeyType, typename ObjectType>
class SortedOctree
{
	public:

		// The world-cell coordinates, in integer form. More of a user parameter for user reference, the octree class doesn't actually touch this member anywhere
		sivec3 cellCoords;

		// The bounding box for this cell
		BoundingBox cellBB;

		// The list of objects in this cell, with a list of objects keys, where each key is associated with a list of objects
		std::vector<std::pair<ObjectListKeyType, std::vector<ObjectType> > > objectList;
		//std::map<ObjectListKeyType, std::vector<ObjectType> > objectList;

		// The list of pending objects to be inserted. The tree is updated when the "flushTree" is called
		std::vector<std::pair<ObjectListKeyType, std::vector<ObjectType> > > pendingInsertionObjectList;
		//std::map<ObjectListKeyType, std::vector<ObjectType> > objectList;

		// A pointer to this cell's parent cell
		SortedOctree<ObjectListKeyType, ObjectType> *parent;

		// A list of this cell's children, of which each are considered "active" according to member "activeChildren"
		SortedOctree<ObjectListKeyType, ObjectType> *children[8];

		// A bitmask indicating which children are active or currently being used
		uint8_t activeChildren;

		SortedOctree (SortedOctree<ObjectListKeyType, ObjectType> *parentCell);

		void insertObject (const ObjectListKeyType &type, const ObjectType &obj);
		void insertObjects (const ObjectListKeyType &type, const std::vector<ObjectType> &objs);

		void flushTreeUpdates (const OctreeRules &rules);
		void trimTree ();
};

template<typename ObjectListKeyType, typename ObjectType>
inline SortedOctree<ObjectListKeyType, ObjectType>::SortedOctree (SortedOctree<ObjectListKeyType, ObjectType> *parentCell)
{
	parent = parentCell;
	activeChildren = 0;
	memset(children, 0, sizeof(children));
}

template<typename ObjectListKeyType, typename ObjectType>
inline void SortedOctree<ObjectListKeyType, ObjectType>::insertObject (const ObjectListKeyType &type, const ObjectType &obj)
{
	auto list = std::find(pendingInsertionObjectList.begin(), pendingInsertionObjectList.end(), type);

	if (list == pendingInsertionObjectList.end())
	{
		std::vector<ObjectType> newList(1, obj);

		pendingInsertionObjectList.push_back(std::make_pair(type, newList));
	}
	else
	{
		list->second.push_back(obj);
	}
}

template<typename ObjectListKeyType, typename ObjectType>
inline void SortedOctree<ObjectListKeyType, ObjectType>::insertObjects (const ObjectListKeyType &type, const std::vector<ObjectType> &objs)
{
	auto list = std::find(pendingInsertionObjectList.begin(), pendingInsertionObjectList.end(), type);

	if (list == pendingInsertionObjectList.end())
	{
		std::vector<ObjectType> newList(objs.begin(), objs.end());

		pendingInsertionObjectList.push_back(std::make_pair(type, newList));
	}
	else
	{
		list->second.insert(list->second.end(), objs.begin(), objs.end());
	}
}

/*
 * Adds all of the objects that are pending insertion into the tree, will
 * create new child nodes, and do everything an octree should.
 */
template<typename ObjectListKeyType, typename ObjectType>
inline void SortedOctree<ObjectListKeyType, ObjectType>::flushTreeUpdates (const OctreeRules &rules)
{
	// If there aren't any objects pending insertion, then we don't need to do anything
	size_t pendingListProcessCount = 0;

	for (size_t i = 0; i < pendingInsertionObjectList.size(); i ++)
	{
		std::vector<ObjectType> &pendingList = pendingInsertionObjectList[i].second;

		if (pendingList.size() == 0)
			continue;

		pendingListProcessCount ++;

		auto typeObjectListIt = std::find(objectList.begin(), objectList.end(), pendingInsertionObjectList[i].first);

		if (typeObjectListIt == objectList.end())
		{
			std::vector<ObjectType> newObjectList(pendingList.begin(), pendingList.end());

			objectList.push_back(std::make_pair(pendingInsertionObjectList[i].first, newObjectList));
		}
		else
		{
			typeObjectListIt->second.insert(typeObjectListIt->second.end(), pendingList.begin(), pendingList.end());
		}

		pendingList.clear();
	}

	// If there aren't any objects pending insertion, then we don't need to do anything
	if (pendingListProcessCount == 0)
	{
		return;
	}

	// If this node is the smallest a node can be according to the rules, then we won't do any more
	if ((uint32_t) cellBB.sizeX() <= rules.minCellSize || (uint32_t) cellBB.sizeY() <= rules.minCellSize || (uint32_t) cellBB.sizeZ() <= rules.minCellSize)
		return;

	svec4 center = cellBB.min;
	center.x += cellBB.sizeX() * 0.5f;
	center.y += cellBB.sizeY() * 0.5f;
	center.z += cellBB.sizeZ() * 0.5f;

	// The bounding boxes for all of the child nodes
	BoundingBox octants[8];

	octants[0] =
	{	cellBB.min, center};
	octants[1] =
	{
		{	center.x, cellBB.min.y, cellBB.min.z},
		{	cellBB.max.x, center.y, center.z}};
	octants[2] =
	{
		{	center.x, cellBB.min.y, center.z},
		{	cellBB.max.x, center.y, cellBB.max.z}};
	octants[3] =
	{
		{	cellBB.min.x, cellBB.min.y, center.z},
		{	center.x, center.y, cellBB.max.z}};
	octants[4] =
	{
		{	cellBB.min.x, center.y, cellBB.min.z},
		{	center.x, cellBB.max.y, center.z}};
	octants[5] =
	{
		{	center.x, center.y, cellBB.min.z},
		{	cellBB.max.x, cellBB.max.y, center.z}};
	octants[6] =
	{	center, cellBB.max};
	octants[7] =
	{
		{	cellBB.min.x, center.y, center.z},
		{	center.x, cellBB.max.y, cellBB.max.z}};

	uint8_t updateChildrenBitmask = 0;

	for (size_t keyType = 0; keyType < objectList.size(); keyType ++)
	{
		ObjectListKeyType &listKey = objectList[keyType].first;
		std::vector<ObjectType> &typeObjectList = objectList[keyType].second;

		// If we have more objects in our cell than the rules say we can, then we'll recursively split our tree until each cell follows those rules
		if (typeObjectList.size() > (size_t) rules.maxObjectListSize)
		{
			// A list of objects for each new child node
			std::vector<ObjectType> octList[8];
			std::vector<size_t> objectDelist;

			// Check each object to see if it fits in a child node, and if so, add it to the queue
			for (size_t i = 0; i < typeObjectList.size(); i ++)
			{
				ObjectType &obj = typeObjectList[i];
				svec4 objectBoundingSphere = {obj.octreeGetPositionScale().x, obj.octreeGetPositionScale().y, obj.octreeGetPositionScale().z, listKey.octreeGetBoundingSphereRadius() * obj.octreeGetPositionScale().w};

				for (int a = 0; a < 8; a ++)
				{
					if (octants[a].containsBoundingSphere(objectBoundingSphere))
					{
						octList[a].push_back(typeObjectList[i]);
						objectDelist.push_back(i);

						break;
					}
				}
			}

			// Remove all objects from this node that are being pushed down to a child node
			while (objectDelist.size() > 0)
			{
				typeObjectList.erase(typeObjectList.begin() + objectDelist.back());
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

						children[a] = new SortedOctree<ObjectListKeyType, ObjectType>(this);
						children[a]->cellBB = octants[a];
					}

					updateChildrenBitmask |= (1 << a);

					auto pendingList = std::find(children[a]->pendingInsertionObjectList.begin(), children[a]->pendingInsertionObjectList.end(), listKey);

					if (pendingList == children[a]->pendingInsertionObjectList.end())
					{
						std::vector<ObjectType> newPendingList(octList[a].begin(), octList[a].end());

						children[a]->pendingInsertionObjectList.push_back(std::make_pair(listKey, newPendingList));
					}
					else
					{
						pendingList->second.insert(pendingList->second.end(), octList[a].begin(), octList[a].end());
					}

					//children[a]->pendingInsertionObjectList.insert(children[a]->pendingInsertionObjectList.end(), octList[a].begin(), octList[a].end());
					//children[a]->flushTreeUpdates(rules);
				}
			}
		}
	}

	// After we've added all the pending changes to the child nodes, let's update them
	for (int a = 0; a < 8; a ++)
	{
		if (updateChildrenBitmask & (1 << a))
		{
			children[a]->flushTreeUpdates(rules);
		}
	}
}

/*
 * Searches the tree for empty nodes, and deletes them.
 */
template<typename ObjectListKeyType, typename ObjectType>
inline void SortedOctree<ObjectListKeyType, ObjectType>::trimTree ()
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

#endif /* WORLD_SORTEDOCTREE_H_ */
