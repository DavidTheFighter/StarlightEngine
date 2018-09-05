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
* common.h
*
* Created on: Aug 20, 2017
*     Author: david
*/

#ifndef COMMON_H_
#define COMMON_H_

/*
 * Contains common included headers to reduce head-ache. Does not include files from this project, only std headers.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <chrono>
#include <cstring>
#include <set>
#include <thread>
#include <mutex>
#include <atomic>
#include <locale>
#include <codecvt>

#ifdef __linux__
#include <unistd.h>
#include <libshaderc/shaderc.hpp>
#include <dirent.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define GLFW_DOUBLE_PRESS 3

#include <lodepng.h>

#include <Resources/FileLoader.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define INFO_PREFIX "[StarlightEngine] [Info]"
#define ERR_PREFIX "[StarlightEngine] [Error]"
#define WARN_PREFIX "[StarlightEngine] [Warning]"

#define APP_NAME "Not sure yet"
#define ENGINE_NAME "StarlightEngine"

#define APP_VERSION_MAJOR 1
#define APP_VERSION_MINOR 0
#define APP_VERSION_REVISION 0

#define ENGINE_VERSION_MAJOR 1
#define ENGINE_VERSION_MINOR 0
#define ENGINE_VERSION_REVISION 0

#define SE_RENDER_DEBUG_MARKERS 1
#define D3D12_DEBUG_COMPATIBILITY_CHECKS 1
#define VULKAN_DEBUG_COMPATIBILITY_CHECKS 1

#define LEVEL_CELL_SIZE 256

#define SECONDS_IN_DAY (24 * 60 * 60)

#define M_SQRT_2 1.4142135624

#define DEBUG_ASSERT(x) if (!(x)) { printf("%s Assertion \"%s\" failed @ file %s, line %i\n", ERR_PREFIX, #x, __FILE__, __LINE__); throw std::runtime_error("failed assertion"); }

#ifndef M_PI
#define M_PI 3.1415926
#endif

/*
 * For some reasons, simple POD structs are faster than glm's vecs, so I'm
 * using my own where I just need to store data, and no do any ops on it.
 * The 's' prefix of the type indicates 'simple'.
 */

typedef struct simple_float_vector_2
{
		float x, y;
} svec2;

typedef struct simple_float_vector_3
{
		float x, y, z;
} svec3;

typedef struct simple_float_vector_4
{
		float x, y, z, w;

		inline bool operator== (const simple_float_vector_4 &vec0)
		{
			return vec0.x == this->x && vec0.y == this->y && vec0.z == this->z && vec0.w == this->w;
		}
} svec4;

typedef struct simple_integer_vector_2
{
		int32_t x, y;
} sivec2;

typedef struct simple_integer_vector_3
{
		int32_t x, y, z;

		bool operator< (const simple_integer_vector_3 &other) const
		{
			if (x < other.x)
				return true;

			if (x > other.x)
				return false;

			if (y < other.y)
				return true;

			if (y > other.y)
				return false;

			if (z < other.z)
				return true;

			if (z > other.z)
				return false;

			return false;
		}

		bool operator== (const simple_integer_vector_3 &other) const
		{
			return x == other.x && y == other.y && z == other.z;
		}
} sivec3;

typedef struct simple_integer_vector_4
{
		int32_t x, y, z, w;
} sivec4;

typedef struct simple_unsigned_integer_vector_2
{
		uint32_t x, y;
} suvec2;

typedef struct simple_unsigned_integer_vector_3
{
		uint32_t x, y, z;
} suvec3;

typedef struct simple_unsigned_integer_vector_4
{
		uint32_t x, y, z, w;
} suvec4;

typedef struct
{
	svec3 pos0;
	uint32_t color0;
	svec3 pos1;
	uint32_t color1;
} PhysicsDebugLine;

typedef struct
{
	uint32_t numLines;
	std::vector<PhysicsDebugLine> lines;
} PhysicsDebugRenderData;

struct Camera
{
		glm::vec3 position;
		glm::vec2 lookAngles;
};

inline std::wstring utf8_to_utf16(const std::string& utf8)
{
	std::vector<unsigned long> unicode;
	size_t i = 0;
	while (i < utf8.size())
	{
		unsigned long uni;
		size_t todo;
		bool error = false;
		unsigned char ch = utf8[i++];
		if (ch <= 0x7F)
		{
			uni = ch;
			todo = 0;
		}
		else if (ch <= 0xBF)
		{
			throw std::logic_error("not a UTF-8 string");
		}
		else if (ch <= 0xDF)
		{
			uni = ch & 0x1F;
			todo = 1;
		}
		else if (ch <= 0xEF)
		{
			uni = ch & 0x0F;
			todo = 2;
		}
		else if (ch <= 0xF7)
		{
			uni = ch & 0x07;
			todo = 3;
		}
		else
		{
			throw std::logic_error("not a UTF-8 string");
		}
		for (size_t j = 0; j < todo; ++j)
		{
			if (i == utf8.size())
				throw std::logic_error("not a UTF-8 string");
			unsigned char ch = utf8[i++];
			if (ch < 0x80 || ch > 0xBF)
				throw std::logic_error("not a UTF-8 string");
			uni <<= 6;
			uni += ch & 0x3F;
		}
		if (uni >= 0xD800 && uni <= 0xDFFF)
			throw std::logic_error("not a UTF-8 string");
		if (uni > 0x10FFFF)
			throw std::logic_error("not a UTF-8 string");
		unicode.push_back(uni);
	}
	std::wstring utf16;
	for (size_t i = 0; i < unicode.size(); ++i)
	{
		unsigned long uni = unicode[i];
		if (uni <= 0xFFFF)
		{
			utf16 += (wchar_t) uni;
		}
		else
		{
			uni -= 0x10000;
			utf16 += (wchar_t) ((uni >> 10) + 0xD800);
			utf16 += (wchar_t) ((uni & 0x3FF) + 0xDC00);
		}
	}
	return utf16;
}

template<typename T0>
inline std::string toString (T0 arg)
{
	std::stringstream ss;

	ss << arg;

	std::string str;

	ss >> str;

	return str;
}

template<typename T0>
inline void appendVector(std::vector<T0> &to, const std::vector<T0> &vectorToBeAppended)
{
	to.insert(to.end(), vectorToBeAppended.begin(), vectorToBeAppended.end());
}

/*
 * Gets the parent directory of the file. Does not include a separtor at the end of the string, e.g. "C:\Users\Someone\Documents"
 */
inline std::string getDirectoryOfFile(const std::string &file)
{
	size_t i = file.length() - 1;

	while (i > 0 && file[i] != '/' && file[i] != '\\')
	{
		i--;
	}

	return file.substr(0, i);
}

inline size_t stringHash (const std::string &str)
{
	return std::hash<std::string> {} (str);
}

inline std::vector<std::string> split (const std::string &s, char delim)
{
	std::stringstream ss(s);
	std::string item;
	std::vector<std::string> elems;

	while (getline(ss, item, delim))
	{
		elems.push_back(item);
	}

	return elems;
}

inline void writeFile (const std::string &filename, const std::vector<char> &data)
{
	std::ofstream file(filename, std::ios::out | std::ios::binary);

	if (!file.is_open())
	{
		printf("%s Failed to open file: %s for writing\n", ERR_PREFIX, filename.c_str());

		throw std::runtime_error("failed to open file for writing!");
	}

	file.write(reinterpret_cast<const char*> (data.data()), data.size());
	file.close();
}

inline void writeFileStr (const std::string &filename, const std::string &data)
{
	std::ofstream file(filename, std::ios::out | std::ios::binary);

	if (!file.is_open())
	{
		printf("%s Failed to open file: %s for writing\n", ERR_PREFIX, filename.c_str());

		throw std::runtime_error("failed to open file for writing!");
	}

	file.write(data.c_str(), data.length());
	file.close();
}

inline void seqmemcpy (char *to, const void *from, size_t size, size_t &offset)
{
	memcpy(to + offset, from, size);
	offset += size;
}

inline uint64_t secondsToNanoseconds (const uint64_t& nanoseconds)
{
	return nanoseconds * uint64_t(1e9);
}

inline uint64_t millisecondsToNanoseconds (const uint64_t& nanoseconds)
{
	return nanoseconds * uint64_t(1e6);
}

#ifdef __linux__
	inline std::string getShaderCCompilationStatusString (shaderc_compilation_status status)
	{
		switch (status)
		{
			case shaderc_compilation_status_success:
				return "shaderc_compilation_status_success";
			case shaderc_compilation_status_invalid_stage:
				return "shaderc_compilation_status_invalid_stage";
			case shaderc_compilation_status_compilation_error:
				return "shaderc_compilation_status_compilation_error";
			case shaderc_compilation_status_internal_error:
				return "shaderc_compilation_status_internal_error";
			case shaderc_compilation_status_null_result_object:
				return "shaderc_compilation_status_null_result_object";
			case shaderc_compilation_status_invalid_assembly:
				return "shaderc_compilation_status_invalid_assembly";
			default:
				return "Dunno";
		}
	}
#endif

#endif /* COMMON_H_ */
