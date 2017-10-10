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
 * D3D11Enums.h
 * 
 * Created on: Oct 9, 2017
 *     Author: David
 */

#ifndef RENDERING_D3D11_D3D11ENUMS_H_
#define RENDERING_D3D11_D3D11ENUMS_H_

#ifdef _WIN32

inline UINT toD3D11BindFlags (BufferUsageFlags flags)
{
	UINT d3dFlags = 0;
	d3dFlags |= (flags & BUFFER_USAGE_VERTEX_BUFFER_BIT) ? D3D11_BIND_VERTEX_BUFFER : 0;
	d3dFlags |= (flags & BUFFER_USAGE_INDEX_BUFFER_BIT) ? D3D11_BIND_INDEX_BUFFER : 0;
	d3dFlags |= (flags & BUFFER_USAGE_UNIFORM_BUFFER_BIT) ? D3D11_BIND_CONSTANT_BUFFER : 0;

	return d3dFlags;
}

#endif

#endif /* RENDERING_D3D11_D3D11ENUMS_H_ */
