/*
 * common.h
 *
 *  Created on: Aug 20, 2017
 *      Author: david
 */

#ifndef COMMON_H_
#define COMMON_H_

/*
 * Contains common included headers to reduce head-ache. Does not include files from this project, only std headers.
 */

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <stdarg.h>
#include <math.h>

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

#ifdef __linux__
#include <unistd.h>
#endif

#define GLFW_DOUBLE_PRESS 3

#include <lodepng.h>
#include <libshaderc/shaderc.hpp>

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

#define COMPILER_BARRIER() asm volatile("" ::: "memory")

#define DEBUG_ASSERT(x) if (!(x)) { printf("%s Assertion failed @ file %s, line %i\n", ERR_PREFIX, __FILE__, __LINE__); throw std::runtime_error("failed assertion"); }

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
} svec4;

typedef struct simple_integer_vector_2
{
		int32_t x, y;
} sivec2;

typedef struct simple_integer_vector_3
{
		int32_t x, y, z;
} sivec3;

typedef struct simple_integer_vector_4
{
		int32_t x, y, z, w;
} sivec4;

template<typename T0>
inline std::string toString (T0 arg)
{
	std::stringstream ss;

	ss << arg;

	std::string str;

	ss >> str;

	return str;
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

inline std::vector<char> readFile (const std::string& filename)
{
	std::ifstream file("/media/david/Main Disk/Programming/StarlightEngineDev/StarlightEngine/" + filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

inline uint64_t secondsToNanoseconds (const uint64_t& nanoseconds)
{
	return nanoseconds * uint64_t(1e9);
}

inline uint64_t millisecondsToNanoseconds (const uint64_t& nanoseconds)
{
	return nanoseconds * uint64_t(1e6);
}

inline std::string getShaderCCompilationStatusString(shaderc_compilation_status status)
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

#endif /* COMMON_H_ */
