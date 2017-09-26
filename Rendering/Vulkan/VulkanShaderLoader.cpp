/*/vulkan/temp-swapchain.glsl
 * VulkanShaderLoader.cpp
 *
 *  Created on: Sep 14, 2017
 *      Author: david
 *
 *  Note: For future reference, the shaderc lib has a bug I don't feel like reporting. The compilation result
 *  returns iterators, and the cend() iterator by default returns an incorrect value. By default, it returns:
 *  "cbegin() + compilationLength() / sizeof(uint32_t). I have no idea why it does this, because it's incorrect.
 *  It should be changed to "cbegin() + compilationLength()". Dunno why this is, but whatever :D.
 */

#include "Rendering/Vulkan/VulkanShaderLoader.h"

shaderc_shader_kind getShaderKindFromShaderStage(VkShaderStageFlagBits stage);
std::string getShaderStageMacroString (VkShaderStageFlagBits stage);

std::vector<uint32_t> VulkanShaderLoader::compileGLSL (shaderc::Compiler &compiler, const std::string &file, VkShaderStageFlagBits stages)
{
	std::vector<char> glslSource = readFile(file);

	shaderc::CompileOptions opts;
	opts.AddMacroDefinition(getShaderStageMacroString(stages));

	shaderc::SpvCompilationResult spvComp = compiler.CompileGlslToSpv(std::string(glslSource.data(), glslSource.data() + glslSource.size()), getShaderKindFromShaderStage(stages), file.c_str(), opts);

	if (spvComp.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		printf("%s Failed to compile GLSL shader: %s, shaderc returned: %u, gave message: %s\n", ERR_PREFIX, file.c_str(), spvComp.GetCompilationStatus(), spvComp.GetErrorMessage().c_str());

		throw std::runtime_error("shaderc error - failed compilation");
	}

	return std::vector<uint32_t> (spvComp.cbegin(), spvComp.cend());
}

VkShaderModule VulkanShaderLoader::createVkShaderModule (const VkDevice &device, const std::vector<uint32_t> &spirv)
{
	VkShaderModuleCreateInfo moduleCreateInfo = {};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = static_cast<uint32_t> (spirv.size());
	moduleCreateInfo.pCode = spirv.data();

	VkShaderModule module;
	VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &module));

	return module;
}

shaderc_shader_kind getShaderKindFromShaderStage(VkShaderStageFlagBits stage)
{
	switch (stage)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:
			return shaderc_vertex_shader;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
			return shaderc_tess_control_shader;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
			return shaderc_tess_evaluation_shader;
		case VK_SHADER_STAGE_GEOMETRY_BIT:
			return shaderc_geometry_shader;
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return shaderc_fragment_shader;
		case VK_SHADER_STAGE_COMPUTE_BIT:
			return shaderc_compute_shader;
		default:
			return shaderc_glsl_infer_from_source;
	}
}

std::string getShaderStageMacroString (VkShaderStageFlagBits stage)
{
	switch (stage)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:
			return "SHADER_STAGE_VERTEX";
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
			return "SHADER_STAGE_TESSELLATION_CONTROL";
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
			return "SHADER_STAGE_TESSELLATION_EVALUATION";
		case VK_SHADER_STAGE_GEOMETRY_BIT:
			return "SHADER_STAGE_GEOMETRY";
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return "SHADER_STAGE_FRAGMENT";
		case VK_SHADER_STAGE_COMPUTE_BIT:
			return "SHADER_STAGE_COMPUTE";
		case VK_SHADER_STAGE_ALL:
			return "SHADER_STAGE_ALL";
		default:
			return "SHADER_STAGE_UNDEFINED";
	}
}
