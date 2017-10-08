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
 * VulkanDescriptorPool.h
 * 
 * Created on: Oct 7, 2017
 *     Author: david
 */

#ifndef RENDERING_VULKAN_VULKANDESCRIPTORPOOL_H_
#define RENDERING_VULKAN_VULKANDESCRIPTORPOOL_H_

#include <common.h>
#include <Rendering/Vulkan/vulkan_common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

struct VulkanDescriptorSet;

struct VulkanDescriptorPoolObject
{
		VkDescriptorPool pool; // This pool object's vulkan pool handle

		std::vector<std::pair<VulkanDescriptorSet*, bool> > unusedPoolSets;
		std::vector<VulkanDescriptorSet*> usedPoolSets;
};

class VulkanRenderer;

class VulkanDescriptorPool : public RendererDescriptorPool
{
	public:

		bool canFreeSetFromPool; // If individual sets can be freed, i.e. the pool was created w/ VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
		uint32_t poolBlockAllocSize; // The .maxSets value for each pool created, should not exceed 1024 (arbitrary, but usefully arbitrary :D)

		std::vector<DescriptorSetLayoutBinding> layoutBindings; // The layout bindings of each set the pool allocates
		std::vector<VkDescriptorPoolSize> vulkanPoolSizes; // Just so that it's faster/easier to create new vulkan descriptor pool objects

		// A list of all the local pool/blocks that have been created & their own allocation data
		std::vector<VulkanDescriptorPoolObject> descriptorPools;

		VulkanRenderer *renderer;

		DescriptorSet allocateDescriptorSet ();
		std::vector<DescriptorSet> allocateDescriptorSets (uint32_t setCount);

		void freeDescriptorSet (DescriptorSet set);
		void freeDescriptorSets (const std::vector<DescriptorSet> sets);
};

#endif /* RENDERING_VULKAN_VULKANDESCRIPTORPOOL_H_ */
