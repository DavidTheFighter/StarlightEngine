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
#include <World/Octree.h>

/*
 * The data for a static object. Much of the misc data for stuff like
 * rendering is stored using fly-weight organization.
 */
typedef struct LevelStaticObject
{
		svec4 position_scale; 	// xyz - position, w - scale
		svec4 rotation; 		// quaternion rotation
		svec4 boundingSphereRadius_padding; // x - bounding sphere radius WITH scaling already applied, yzw - padding

		size_t meshDefUniqueNameHash;
		size_t materialDefUniqueNameHash;

		inline svec4 octreeGetBoundingSphere()
		{
			return {position_scale.x, position_scale.y, position_scale.z, boundingSphereRadius_padding.x};
		}

} LevelStaticObject;

class LevelData
{
	public:

		std::map<sivec3, size_t> activeStaticObjectCells_map; // Maps cell coords to the index of a cell in member "activeStaticObjectCells"
		std::vector<Octree<LevelStaticObject> > activeStaticObjectCells;


		LevelData ();
		virtual ~LevelData ();

		void insertStaticObject (const LevelStaticObject &obj);
		void insertStaticObjects (const std::vector<LevelStaticObject> &objs);
};

#endif /* WORLD_LEVELDATA_H_ */
