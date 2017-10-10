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
 * d3d11_common.h
 * 
 * Created on: Oct 9, 2017
 *     Author: David
 */

#ifndef RENDERING_D3D11_D3D11_COMMON_H_
#define RENDERING_D3D11_D3D11_COMMON_H_

#ifdef _WIN32

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <d3d11.h>
#include <d3dcompiler.h>

inline std::string getHRESULTString (HRESULT hr)
{
	switch (hr)
	{
		case S_OK:
			return "S_OK";
		case E_NOTIMPL:
			return "E_NOTIMPL";
		case E_NOINTERFACE:
			return "E_NOINTERFACE";
		case E_ABORT:
			return "E_ABORT";
		case E_FAIL:
			return "E_FAIL";
		case E_INVALIDARG:
			return "E_INVALIDARG";
		default:
			return "Unknown HRESULT";
	}
}

#define CHECK_HRESULT(x)																																								\
{																																														\
	HRESULT check_hresult_var_hr = (x);																																					\
	if (check_hresult_var_hr != S_OK)																																					\
	{																																													\
		printf("%s HRESULT is \"%s\", in file \"%s\", at line %i, function: %s(..)\n", ERR_PREFIX, getHRESULTString(check_hresult_var_hr).c_str(), __FILE__, __LINE__, __FUNCTION__);		\
		throw std::runtime_error("d3d11 error");																																		\
	}																																													\
}

#endif

#endif /* RENDERING_D3D11_D3D11_COMMON_H_ */
