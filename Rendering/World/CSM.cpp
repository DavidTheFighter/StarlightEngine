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
 * CSM.cpp
 *
 *  Created on: May 16, 2018
 *      Author: David
 */

#include "Rendering/World/CSM.h"

#include <Engine/StarlightEngine.h>

#include <Rendering/Renderer/Renderer.h>

CSM::CSM (Renderer *rendererPtr, uint32_t shadowmapSize, uint32_t numCascades, ResourceFormat format)
{
	renderer = rendererPtr;

	shadowSize = shadowmapSize;
	cascadeCount = numCascades;
	
	csmTexArray = renderer->createTexture({shadowSize, shadowSize, 1}, format, TEXTURE_USAGE_SAMPLED_BIT | (isDepthFormat(format) ? TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : TEXTURE_USAGE_COLOR_ATTACHMENT_BIT), MEMORY_USAGE_GPU_ONLY, true, 1, numCascades);
	csmTexArrayView = renderer->createTextureView(csmTexArray, TEXTURE_VIEW_TYPE_2D_ARRAY, {0, 1, 0, cascadeCount});

	for (uint32_t i = 0; i < cascadeCount; i ++)
	{
		csmTexArraySliceViews.push_back(renderer->createTextureView(csmTexArray, TEXTURE_VIEW_TYPE_2D, {0, 1, i, 1}));
		camProjMats.push_back(glm::mat4());
	}
}

CSM::~CSM ()
{
	renderer->destroyTexture(csmTexArray);
	renderer->destroyTextureView(csmTexArrayView);

	for (uint32_t i = 0; i < cascadeCount; i ++)
	{
		renderer->destroyTextureView(csmTexArraySliceViews[i]);
	}
}

void CSM::update (const glm::mat4 &centerCamViewMat, glm::vec2 fovs, const std::vector<float> &splitDistances, glm::vec3 lightDir)
{
	DEBUG_ASSERT(cascadeCount == splitDistances.size() - 1);

	float tanHalfHFOV = std::tan(fovs.x * 0.5f);
	float tanHalfVFOV = std::tan(fovs.y * 0.5f);
	glm::mat4 invCenterCamViewMat = glm::inverse(centerCamViewMat);

	camViewMat = glm::lookAt(-lightDir, glm::vec3(0), glm::vec3(0, 1, 0));

	for (uint32_t c = 0; c < cascadeCount; c++)
	{
		float xn = splitDistances[c] * tanHalfHFOV;
		float xf = splitDistances[c + 1] * tanHalfHFOV;
		float yn = splitDistances[c] * tanHalfVFOV;
		float yf = splitDistances[c + 1] * tanHalfVFOV;

		glm::vec3 frustumSphereCenter = glm::vec3(0);

		// Calculate frustum corners in view-space
		glm::vec4 frustumCorners[8] = {
				glm::vec4(xn, yn, -splitDistances[c], 1),
				glm::vec4(-xn, yn, -splitDistances[c], 1),
				glm::vec4(xn, -yn, -splitDistances[c], 1),
				glm::vec4(-xn, -yn, -splitDistances[c], 1),
				glm::vec4(xf, yf, -splitDistances[c + 1], 1),
				glm::vec4(-xf, yf, -splitDistances[c + 1], 1),
				glm::vec4(xf, -yf, -splitDistances[c + 1], 1),
				glm::vec4(-xf, -yf, -splitDistances[c + 1], 1)
		};

		// Put frustum corners from view-space into world-space
		for (int i = 0; i < 8; i ++)
		{
			frustumCorners[i] = invCenterCamViewMat * frustumCorners[i];
			frustumSphereCenter += glm::vec3(camViewMat * frustumCorners[i]);
		}

		frustumSphereCenter /= 8.0f;

		float boundingSphereRadius = 0;//glm::length(frustumCornerMax - frustumCornerMin) * 0.5f;

		// Finds the largest radius, which usually stays fairly constant (plus or minus a few 1e-2 or 1e-3)
		for (int i = 0; i < 8; i ++)
		{
			boundingSphereRadius = std::max(boundingSphereRadius, glm::distance(frustumSphereCenter, glm::vec3(camViewMat * frustumCorners[i])));
		}

		// Round off any small error
		boundingSphereRadius = std::ceil(boundingSphereRadius);

		// Step the translation in single texel increments
		float texelStep = (boundingSphereRadius * 2.0f) / float(shadowSize);

		//printf("%u - %f\n", c, texelStep);

		frustumSphereCenter = glm::floor(frustumSphereCenter / texelStep) * texelStep;

		camProjMats[c] = glm::ortho<float>(
				frustumSphereCenter.x - boundingSphereRadius, frustumSphereCenter.x + boundingSphereRadius,
				-frustumSphereCenter.y - boundingSphereRadius, -frustumSphereCenter.y + boundingSphereRadius,
				(-frustumSphereCenter.z + boundingSphereRadius * 16.0f),(-frustumSphereCenter.z - boundingSphereRadius * 16.0f));

		camProjMats[c][1][1] *= -1;
	}
}

glm::mat4 CSM::getCamProjMat (uint32_t cascadeIndex)
{
	return camProjMats[cascadeIndex];
}

glm::mat4 CSM::getCamViewMat ()
{
	return camViewMat;
}

TextureView CSM::getCSMTextureView ()
{
	return csmTexArrayView;
}

TextureView CSM::getCSMTextureSliceView (uint32_t cascadeIndex)
{
	return csmTexArraySliceViews[cascadeIndex];
}

std::vector<TextureView> CSM::getCSMTextureSliceViews ()
{
	return csmTexArraySliceViews;
}

uint32_t CSM::getCascadeCount ()
{
	return cascadeCount;
}

uint32_t CSM::getShadowSize ()
{
	return shadowSize;
}
