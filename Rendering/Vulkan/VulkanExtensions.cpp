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
 * VulkanExtensions.cpp
 * 
 * Created on: Sep 25, 2017
 *     Author: david
 */

#include <common.h>
#include <Rendering/Vulkan/vulkan_common.h>

void VulkanExtensions::getProcAddresses (VkDevice device)
{
	if (SE_VULKAN_DEBUG_MARKERS && enabled_VK_EXT_debug_marker)
	{
		DebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT) vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
		DebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT) vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
		CmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT) vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
		CmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT) vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
		CmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT) vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
	}
}

#if SE_VULKAN_DEBUG_MARKERS

bool VulkanExtensions::enabled_VK_EXT_debug_marker = false;

PFN_vkDebugMarkerSetObjectTagEXT VulkanExtensions::DebugMarkerSetObjectTagEXT = VK_NULL_HANDLE;
PFN_vkDebugMarkerSetObjectNameEXT VulkanExtensions::DebugMarkerSetObjectNameEXT = VK_NULL_HANDLE;
PFN_vkCmdDebugMarkerBeginEXT VulkanExtensions::CmdDebugMarkerBeginEXT = VK_NULL_HANDLE;
PFN_vkCmdDebugMarkerEndEXT VulkanExtensions::CmdDebugMarkerEndEXT = VK_NULL_HANDLE;
PFN_vkCmdDebugMarkerInsertEXT VulkanExtensions::CmdDebugMarkerInsertEXT = VK_NULL_HANDLE;

VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectTagEXT (VkDevice device, const VkDebugMarkerObjectTagInfoEXT* pTagInfo)
{
	if (!VulkanExtensions::enabled_VK_EXT_debug_marker)
		return VK_SUCCESS;

	return VulkanExtensions::DebugMarkerSetObjectTagEXT(device, pTagInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectNameEXT (VkDevice device, const VkDebugMarkerObjectNameInfoEXT* pNameInfo)
{
	if (!VulkanExtensions::enabled_VK_EXT_debug_marker)
		return VK_SUCCESS;

	return VulkanExtensions::DebugMarkerSetObjectNameEXT(device, pNameInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerBeginEXT (VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo)
{
	if (!VulkanExtensions::enabled_VK_EXT_debug_marker)
		return;

	VulkanExtensions::CmdDebugMarkerBeginEXT(commandBuffer, pMarkerInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerEndEXT (VkCommandBuffer commandBuffer)
{
	if (!VulkanExtensions::enabled_VK_EXT_debug_marker)
		return;

	VulkanExtensions::CmdDebugMarkerEndEXT(commandBuffer);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerInsertEXT (VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo)
{
	if (!VulkanExtensions::enabled_VK_EXT_debug_marker)
		return;

	VulkanExtensions::CmdDebugMarkerInsertEXT(commandBuffer, pMarkerInfo);
}

#else

VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectTagEXT (VkDevice device, const VkDebugMarkerObjectTagInfoEXT* pTagInfo)
{
	return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectNameEXT (VkDevice device, const VkDebugMarkerObjectNameInfoEXT* pNameInfo)
{
	return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerBeginEXT (VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo)
{
}

VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerEndEXT (VkCommandBuffer commandBuffer)
{
}

VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerInsertEXT (VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo)
{
}

#endif
