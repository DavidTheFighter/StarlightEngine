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
 * BoundingBox.h
 * 
 * Created on: Oct 14, 2017
 *     Author: david
 */

#ifndef WORLD_BOUNDINGBOX_H_
#define WORLD_BOUNDINGBOX_H_

#include <common.h>

class BoundingBox
{
	public:

		// The min and max coordinates, 64-bit aligned (the w component doesn't do anything in either vector)
		svec4 min, max;

		/*
		 * Checks whether a position is in this bounding box
		 */
		inline bool containsObject (const svec4 &position)
		{
			return position.x >= min.x && position.x < max.x && position.y >= min.y && position.y < max.y && position.z >= min.z && position.z < max.z;
		}

		/*
		 * Checks if this bounding box FULLLY contains another bounding box.
		 */
		inline bool containsBoundingBox (const BoundingBox &bb)
		{
			return bb.min.x >= min.x && bb.min.y >= min.y && bb.min.z >= min.z && bb.max.x <= max.x && bb.max.y <= max.y && bb.max.z <= max.z;
		}

		/*
		 * Checks if this bounding box FULLY contains a bounding sphere. xyz - position, w - radius
		 */
		inline bool containsBoundingSphere (const svec4 &sphere)
		{
			return sphere.x - sphere.w >= min.x && sphere.y - sphere.w >= min.y && sphere.z - sphere.w >= min.z && sphere.x + sphere.w <= max.x && sphere.y + sphere.w <= max.y && sphere.z + sphere.w <= max.z;
		}

		/*
		 * Gets the size of the x dimension of this box
		 */
		inline float sizeX ()
		{
			return max.x - min.x;
		}

		/*
		 * Gets the size of the y dimension of this box
		 */
		inline float sizeY ()
		{
			return max.y - min.y;
		}

		/*
		 * Gets the size of the z dimension of this box
		 */
		inline float sizeZ ()
		{
			return max.z - min.z;
		}

		inline glm::vec3 getCenter ()
		{
			return glm::vec3((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f);
		}
};

#endif /* WORLD_BOUNDINGBOX_H_ */
