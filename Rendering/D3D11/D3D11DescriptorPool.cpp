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
 * D3D11DescriptorPool.cpp
 * 
 * Created on: Oct 9, 2017
 *     Author: David
 */

#include "Rendering/D3D11/D3D11DescriptorPool.h"

#include <Rendering/D3D11/D3D11Objects.h>

D3D11DescriptorPool::D3D11DescriptorPool ()
{
	
}

D3D11DescriptorPool::~D3D11DescriptorPool ()
{

}

DescriptorSet D3D11DescriptorPool::allocateDescriptorSet ()
{
	return allocateDescriptorSets(1)[0];
}

std::vector<DescriptorSet> D3D11DescriptorPool::allocateDescriptorSets (uint32_t setCount)
{
	std::vector<DescriptorSet> sets;

	for (uint32_t i = 0; i < setCount; i ++)
	{
		sets.push_back(new D3D11DescriptorSet());
	}

	return sets;
}

void D3D11DescriptorPool::freeDescriptorSet (DescriptorSet set)
{
}

void D3D11DescriptorPool::freeDescriptorSets (const std::vector<DescriptorSet> sets)
{
}
