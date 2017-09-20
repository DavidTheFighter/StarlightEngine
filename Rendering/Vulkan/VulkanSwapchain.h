/*
 * VulkanSwapchain.h
 *
 *  Created on: Sep 13, 2017
 *      Author: david
 */

#ifndef RENDERING_VULKAN_VULKANSWAPCHAIN_H_
#define RENDERING_VULKAN_VULKANSWAPCHAIN_H_

#include <common.h>
#include <Rendering/Vulkan/vulkan_common.h>

typedef struct SwapchainSupportDetails
{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
} SwapchainSupportDetails;

class VulkanRenderer;

class VulkanSwapchain
{
	public:

		VkSwapchainKHR swapchain;

		VkFormat swapchainImageFormat;
		VkExtent2D swapchainExtent;

		SwapchainSupportDetails swapchainDetails;

		VulkanSwapchain (VulkanRenderer* vulkanRendererParent);
		virtual ~VulkanSwapchain ();

		void initSwapchain();
		void presentToSwapchain();

		void createSwapchain();
		void destroySwapchain();
		void recreateSwapchain();

		void setSwapchainSourceImage(VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout);

		SwapchainSupportDetails querySwapchainSupport (VkPhysicalDevice device, VkSurfaceKHR surface);

	private:

		VkCommandPool swapchainCommandPool;

		VkSemaphore swapchainNextImageAvailableSemaphore;
		VkSemaphore swapchainDoneRenderingSemaphore;

		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;
		std::vector<VkFramebuffer> swapchainFramebuffers;
		std::vector<VkCommandBuffer> swapchainCommandBuffers;

		VkImage swapchainDummySourceImage;
		VkImageView swapchainDummySourceImageView;
		VkSampler swapchainDummySampler;

		VkDescriptorPool swapchainDescriptorPool;
		VkDescriptorSetLayout swapchainDescriptorLayout;
		VkDescriptorSet swapchainDescriptorSet;

		VkShaderModule swapchainVertShader;
		VkShaderModule swapchainFragShader;

		VkRenderPass swapchainRenderPass;
		VkPipelineLayout swapchainPipelineLayout;
		VkPipeline swapchainPipeline;

		VulkanRenderer* renderer;

		void createDescriptors();
		void createRenderPass();
		void createGraphicsPipeline();
		void createFramebuffers();
		void createSyncPrimitives();
		void prerecordCommandBuffers();

		VkSurfaceFormatKHR chooseSwapSurfaceFormat (const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode (const std::vector<VkPresentModeKHR> availablePresentModes);
		VkExtent2D chooseSwapExtent (const VkSurfaceCapabilitiesKHR& capabilities, uint32_t windowWidth, uint32_t windowHeight);
};

#endif /* RENDERING_VULKAN_VULKANSWAPCHAIN_H_ */
