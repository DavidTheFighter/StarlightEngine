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
 * VulkanDescriptorPool.cpp
 * 
 * Created on: Oct 7, 2017
 *     Author: david
 */

#include "Rendering/Vulkan/VulkanDescriptorPool.h"

#include <Rendering/Vulkan/VulkanEnums.h>
#include <Rendering/Vulkan/VulkanObjects.h>
#include <Rendering/Vulkan/VulkanRenderer.h>
#include <Rendering/Vulkan/VulkanPipelines.h>

DescriptorSet VulkanDescriptorPool::allocateDescriptorSet ()
{
	return allocateDescriptorSets(1)[0];
}

bool VulkanRenderer_tryAllocFromDescriptorPoolObject (VulkanRenderer *renderer, VulkanDescriptorPool *pool, uint32_t poolObjIndex, VulkanDescriptorSet* &outSet)
{
	VulkanDescriptorPoolObject &poolObj = pool->descriptorPools[poolObjIndex];

	if (poolObj.unusedPoolSets.size() == 0)
		return false;

	// TODO Change vulkan descriptor sets to allocate in chunks rather than invididually

	bool setIsAllocated = poolObj.unusedPoolSets.back().second;

	if (setIsAllocated)
	{
		outSet = poolObj.unusedPoolSets.back().first;

		DEBUG_ASSERT(outSet != nullptr);

		poolObj.usedPoolSets.push_back(poolObj.unusedPoolSets.back().first);
		poolObj.unusedPoolSets.pop_back();
	}
	else
	{
		VulkanDescriptorSet *vulkanDescSet = new VulkanDescriptorSet();
		vulkanDescSet->descriptorPoolObjectIndex = poolObjIndex;

		VkDescriptorSetLayout descLayout = renderer->pipelineHandler->createDescriptorSetLayout(pool->layoutBindings);

		VkDescriptorSetAllocateInfo descSetAllocInfo = {};
		descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAllocInfo.descriptorPool = poolObj.pool;
		descSetAllocInfo.descriptorSetCount = 1;
		descSetAllocInfo.pSetLayouts = &descLayout;

		VK_CHECK_RESULT(vkAllocateDescriptorSets(renderer->device, &descSetAllocInfo, &vulkanDescSet->setHandle));

		poolObj.usedPoolSets.push_back(vulkanDescSet);
		poolObj.unusedPoolSets.pop_back();

		outSet = vulkanDescSet;
	}

	return true;
}


std::vector<DescriptorSet> VulkanDescriptorPool::allocateDescriptorSets (uint32_t setCount)
{
	DEBUG_ASSERT(setCount > 0);

	std::vector<DescriptorSet> vulkanSets = std::vector<DescriptorSet>(setCount);

	for (uint32_t i = 0; i < setCount; i ++)
	{
		//printf("-- %u --\n", i);
		VulkanDescriptorSet *vulkanSet = nullptr;

		// Iterate over each pool object to see if there's one we can alloc from
		for (size_t p = 0; p < descriptorPools.size(); p ++)
		{
			if (VulkanRenderer_tryAllocFromDescriptorPoolObject(renderer, this, p, vulkanSet))
			{
				break;
			}
		}

		// If the set is still nullptr, then we'll have to create a new pool to allocate from
		if (vulkanSet == nullptr)
		{
			descriptorPools.push_back(renderer->createDescPoolObject(vulkanPoolSizes, poolBlockAllocSize));

			VulkanRenderer_tryAllocFromDescriptorPoolObject(renderer, this, descriptorPools.size() - 1, vulkanSet);
		}

		vulkanSets[i] = vulkanSet;
	}

	return vulkanSets;
}

void VulkanDescriptorPool::freeDescriptorSet (DescriptorSet set)
{
	freeDescriptorSets({set});
}

void VulkanDescriptorPool::freeDescriptorSets (const std::vector<DescriptorSet> sets)
{
	for (size_t i = 0; i < sets.size(); i ++)
	{
		VulkanDescriptorSet *vulkanDescSet = static_cast<VulkanDescriptorSet*> (sets[i]);

		VulkanDescriptorPoolObject &poolObj = descriptorPools[vulkanDescSet->descriptorPoolObjectIndex];
		poolObj.unusedPoolSets.push_back(std::make_pair(vulkanDescSet, true));

		auto it = std::find(poolObj.usedPoolSets.begin(), poolObj.usedPoolSets.end(), vulkanDescSet);

		DEBUG_ASSERT(it != poolObj.usedPoolSets.end());

		poolObj.usedPoolSets.erase(it);
	}
}

