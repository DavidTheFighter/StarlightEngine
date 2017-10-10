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
 * D3D11Objects.h
 * 
 * Created on: Oct 9, 2017
 *     Author: David
 */

#ifndef RENDERING_D3D11_D3D11OBJECTS_H_
#define RENDERING_D3D11_D3D11OBJECTS_H_

#include <common.h>
#include <Rendering/D3D11/d3d11_common.h>

struct D3D11DescriptorSet : public RendererDescriptorSet
{

};

#include <Rendering/D3D11/D3D11CommandPool.h>
#include <Rendering/D3D11/D3D11CommandBuffer.h>
#include <Rendering/D3D11/D3D11DescriptorPool.h>

#endif /* RENDERING_D3D11_D3D11OBJECTS_H_ */
