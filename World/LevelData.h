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
 * LevelData.h
 * 
 * Created on: Oct 13, 2017
 *     Author: david
 */

#ifndef WORLD_LEVELDATA_H_
#define WORLD_LEVELDATA_H_

#include <common.h>
#include <World/SortedOctree.h>

/*
 * The data for a static object. Much of the misc data for stuff like
 * rendering is stored using fly-weight organization.
 */
typedef struct LevelStaticObject
{
		svec4 position_scale; 	// xyz - position, w - scale
		svec4 rotation; 		// quaternion rotation

		inline svec4 octreeGetPositionScale()
		{
			return position_scale;
		}

} LevelStaticObject;

typedef struct LevelStaticObjectType
{
		size_t meshDefUniqueNameHash;
		size_t materialDefUniqueNameHash;

		// x - bounding sphere radius w/o scaling, y - maximum displayed LOD distance, zw - padding
		svec4 boundingSphereRadius_maxLodDist_padding;

		inline float octreeGetBoundingSphereRadius()
		{
			return boundingSphereRadius_maxLodDist_padding.x;
		}

		inline bool operator== (const LevelStaticObjectType &arg0)
		{
			return arg0.materialDefUniqueNameHash == this->materialDefUniqueNameHash && arg0.meshDefUniqueNameHash == this->meshDefUniqueNameHash &&
					arg0.boundingSphereRadius_maxLodDist_padding.x == this->boundingSphereRadius_maxLodDist_padding.x &&
					arg0.boundingSphereRadius_maxLodDist_padding.y == this->boundingSphereRadius_maxLodDist_padding.y; // We only compare these two components, as the rest are padding
		}

		inline bool operator== (const std::pair<LevelStaticObjectType, std::vector<LevelStaticObject> > &arg0)
		{
			return operator== (arg0.first);
		}

} LevelStaticObjectType;

inline bool operator== (const LevelStaticObjectType &arg0, const LevelStaticObjectType &arg1)
{
	return arg0.materialDefUniqueNameHash == arg1.materialDefUniqueNameHash && arg0.meshDefUniqueNameHash == arg1.meshDefUniqueNameHash &&
			arg0.boundingSphereRadius_maxLodDist_padding.x == arg1.boundingSphereRadius_maxLodDist_padding.x &&
			arg0.boundingSphereRadius_maxLodDist_padding.y == arg1.boundingSphereRadius_maxLodDist_padding.y; // We only compare these two components, as the rest are padding
}

inline bool operator== (const std::pair<LevelStaticObjectType, std::vector<LevelStaticObject> > &arg0, const LevelStaticObjectType &arg1)
{
	return operator== (arg0.first, arg1);
}

class LevelData
{
	public:

		std::map<sivec3, size_t> activeStaticObjectCells_map; // Maps cell coords to the index of a cell in member "activeStaticObjectCells"
		std::vector<SortedOctree<LevelStaticObjectType, LevelStaticObject> > activeStaticObjectCells;

		uint32_t heightmapFileCellCount;
		std::vector<std::pair<sivec2, size_t> > heightmapFileLookupTable;

		LevelData ();
		virtual ~LevelData ();

		void insertStaticObject (const LevelStaticObjectType &objType, const LevelStaticObject &obj);
		void insertStaticObjects (const LevelStaticObjectType &objType, const std::vector<LevelStaticObject> &objs);
};

#endif /* WORLD_LEVELDATA_H_ */
