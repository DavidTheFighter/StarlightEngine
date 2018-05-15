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
 * frustum.h
 *
 *  Created on: May 12, 2018
 *      Author: David
 */
#ifndef FRUSTUM_H_
#define FRUSTUM_H_

#include <common.h>

inline bool sphereVisible (float x, float y, float z, float radius, const glm::vec4 (&frustum)[6])
{
	for (int i = 0; i < 6; i++)
		if (frustum[i][0] * x + frustum[i][1] * y + frustum[i][2] * z + frustum[i][3] <= -radius)
			return false;
	return true;
}

inline bool sphereVisible (glm::vec3 pos, float radius, const glm::vec4 (&frustum)[6])
{
	return sphereVisible(pos.x, pos.y, pos.z, radius, frustum);
}

inline bool pointVisible (glm::vec3 pos, const glm::vec4 (&frustum)[6])
{
	return sphereVisible(pos, 0, frustum);
}

inline bool pointVisible (float x, float y, float z, const glm::vec4 (&frustum)[6])
{
	return sphereVisible(x, y, z, 0, frustum);
}

inline bool cubeVisible (float x, float y, float z, float size, const glm::vec4 (&frustum)[6])
{
	for (int i = 0; i < 6; i++)
	{
		if (frustum[i][0] * (x - size) + frustum[i][1] * (y - size) + frustum[i][2] * (z - size) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x + size) + frustum[i][1] * (y - size) + frustum[i][2] * (z - size) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x - size) + frustum[i][1] * (y + size) + frustum[i][2] * (z - size) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x + size) + frustum[i][1] * (y + size) + frustum[i][2] * (z - size) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x - size) + frustum[i][1] * (y - size) + frustum[i][2] * (z + size) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x + size) + frustum[i][1] * (y - size) + frustum[i][2] * (z + size) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x - size) + frustum[i][1] * (y + size) + frustum[i][2] * (z + size) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x + size) + frustum[i][1] * (y + size) + frustum[i][2] * (z + size) + frustum[i][3] > 0)
			continue;
		return false;
	}
	return true;
}

inline bool cubeVisible (glm::vec3 pos, float size, const glm::vec4 (&frustum)[6])
{
	return cubeVisible(pos.x, pos.y, pos.z, size, frustum);
}

inline bool boxVisible (float x, float y, float z, float sizeX, float sizeY, float sizeZ, const glm::vec4 (&frustum)[6])
{
	for (int i = 0; i < 6; i++)
	{
		if (frustum[i][0] * (x - sizeX) + frustum[i][1] * (y - sizeY) + frustum[i][2] * (z - sizeZ) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x + sizeX) + frustum[i][1] * (y - sizeY) + frustum[i][2] * (z - sizeZ) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x - sizeX) + frustum[i][1] * (y + sizeY) + frustum[i][2] * (z - sizeZ) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x + sizeX) + frustum[i][1] * (y + sizeY) + frustum[i][2] * (z - sizeZ) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x - sizeX) + frustum[i][1] * (y - sizeY) + frustum[i][2] * (z + sizeZ) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x + sizeX) + frustum[i][1] * (y - sizeY) + frustum[i][2] * (z + sizeZ) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x - sizeX) + frustum[i][1] * (y + sizeY) + frustum[i][2] * (z + sizeZ) + frustum[i][3] > 0)
			continue;
		if (frustum[i][0] * (x + sizeX) + frustum[i][1] * (y + sizeY) + frustum[i][2] * (z + sizeZ) + frustum[i][3] > 0)
			continue;
		return false;
	}
	return true;
}

inline bool boxVisible (glm::vec3 pos, glm::vec3 size, const glm::vec4 (&frustum)[6])
{
	return boxVisible(pos.x, pos.y, pos.z, size.x, size.y, size.z, frustum);
}

inline void getFrustum (const glm::mat4 &mvp, glm::vec4 (&out_frustum)[6])
{
	float clip[16];

	for (int x = 0, i = 0; x < 4; x++)
	{
		for (int z = 0; z < 4; z++, i++)
		{
			clip[i] = mvp[x][z];
		}
	}

	out_frustum[0][0] = clip[3] - clip[0];
	out_frustum[0][1] = clip[7] - clip[4];
	out_frustum[0][2] = clip[11] - clip[8];
	out_frustum[0][3] = clip[15] - clip[12];

	out_frustum[0] = glm::normalize(out_frustum[0]);

	out_frustum[1][0] = clip[3] + clip[0];
	out_frustum[1][1] = clip[7] + clip[4];
	out_frustum[1][2] = clip[11] + clip[8];
	out_frustum[1][3] = clip[15] + clip[12];

	out_frustum[1] = glm::normalize(out_frustum[1]);

	out_frustum[2][0] = clip[3] + clip[1];
	out_frustum[2][1] = clip[7] + clip[5];
	out_frustum[2][2] = clip[11] + clip[9];
	out_frustum[2][3] = clip[15] + clip[13];

	out_frustum[2] = glm::normalize(out_frustum[2]);

	out_frustum[3][0] = clip[3] - clip[1];
	out_frustum[3][1] = clip[7] - clip[5];
	out_frustum[3][2] = clip[11] - clip[9];
	out_frustum[3][3] = clip[15] - clip[13];

	out_frustum[3] = glm::normalize(out_frustum[3]);

	out_frustum[4][0] = clip[3] - clip[2];
	out_frustum[4][1] = clip[7] - clip[6];
	out_frustum[4][2] = clip[11] - clip[10];
	out_frustum[4][3] = clip[15] - clip[14];

	out_frustum[4] = glm::normalize(out_frustum[4]);

	out_frustum[5][0] = clip[3] + clip[2];
	out_frustum[5][1] = clip[7] + clip[6];
	out_frustum[5][2] = clip[11] + clip[10];
	out_frustum[5][3] = clip[15] + clip[14];

	out_frustum[5] = glm::normalize(out_frustum[5]);
}

#endif /* FRUSTUM_H_ */
