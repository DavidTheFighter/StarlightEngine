/*
 * VulkanShaderLoader.h
 *
 *  Created on: Sep 14, 2017
 *      Author: david
 */

#ifndef RENDERING_VULKAN_VULKANSHADERLOADER_H_
#define RENDERING_VULKAN_VULKANSHADERLOADER_H_

#include <common.h>
#include <Rendering/Vulkan/vulkan_common.h>
#include <Rendering/Renderer/RendererEnums.h>

class VulkanShaderLoader
{
	public:

#ifdef __linux__
		static std::vector<uint32_t> compileGLSL(shaderc::Compiler &compiler, const std::string &file, VkShaderStageFlagBits stages);
		static std::vector<uint32_t> compileGLSLFromSource(shaderc::Compiler &compiler, const std::string &source, const std::string &sourceName, VkShaderStageFlagBits stages);
		static std::vector<uint32_t> compileGLSLFromSource(shaderc::Compiler &compiler, const std::vector<char> &source, const std::string &sourceName, VkShaderStageFlagBits stages);
#elif defined(_WIN32)
		static std::vector<uint32_t> compileGLSL(const std::string &file, VkShaderStageFlagBits stages, ShaderSourceLanguage lang, const std::string &entryPoint);
		static std::vector<uint32_t> compileGLSLFromSource(const std::string &source, const std::string &sourceName, VkShaderStageFlagBits stages, ShaderSourceLanguage lang, const std::string &entryPoint);
		static std::vector<uint32_t> compileGLSLFromSource(const std::vector<char> &source, const std::string &sourceName, VkShaderStageFlagBits stages, ShaderSourceLanguage lang, const std::string &entryPoint);
#endif

		static VkShaderModule createVkShaderModule (const VkDevice &device, const std::vector<uint32_t> &spirv);
};

#endif /* RENDERING_VULKAN_VULKANSHADERLOADER_H_ */
