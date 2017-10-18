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
 * D3D11DescriptorPool.h
 * 
 * Created on: Oct 9, 2017
 *     Author: David
 */

#ifdef _WIN32
#ifndef RENDERING_D3D11_D3D11DESCRIPTORPOOL_H_
#define RENDERING_D3D11_D3D11DESCRIPTORPOOL_H_

#include <common.h>
#include <Rendering/D3D11/d3d11_common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

class D3D11DescriptorPool : public RendererDescriptorPool
{
	public:
		D3D11DescriptorPool ();
		virtual ~D3D11DescriptorPool ();

		DescriptorSet allocateDescriptorSet ();
		std::vector<DescriptorSet> allocateDescriptorSets (uint32_t setCount);

		void freeDescriptorSet (DescriptorSet set);
		void freeDescriptorSets (const std::vector<DescriptorSet> sets);
};

#endif /* RENDERING_D3D11_D3D11DESCRIPTORPOOL_H_ */
#endif