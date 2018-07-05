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
* D3D12DescriptorPool.cpp
*
* Created on: Jul 7, 2018
*     Author: david
*/

#ifndef RENDERING_D3D12_D3D12OBJECTS_H_
#define RENDERING_D3D12_D3D12OBJECTS_H_

struct D3D12Texture : public RendererTexture
{
};

struct D3D12TextureView : public RendererTextureView
{
};

struct D3D12Sampler : public RendererSampler
{
};

struct D3D12RenderPass : public RendererRenderPass
{
};

struct D3D12Framebuffer : public RendererFramebuffer
{
};

struct D3D12Pipeline : public RendererPipeline
{
};

struct D3D12DescriptorSet : public RendererDescriptorSet
{
};

struct D3D12ShaderModule : public RendererShaderModule
{
};

struct D3D12StagingBuffer : public RendererStagingBuffer
{
};

struct D3D12Buffer : public RendererBuffer
{
};

struct D3D12Fence : public RendererFence
{
};

struct D3D12Semaphore : public RendererSemaphore
{
};

#endif /* RENDERING_D3D12_D3D12OBJECTS_H_*/