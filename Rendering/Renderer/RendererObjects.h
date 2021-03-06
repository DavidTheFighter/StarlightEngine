/*
 * RendererObjects.h
 *
 *  Created on: Sep 15, 2017
 *      Author: david
 */

#ifndef RENDERING_RENDERER_RENDEREROBJECTS_H_
#define RENDERING_RENDERER_RENDEREROBJECTS_H_

#include <common.h>

typedef struct RendererTexture
{
		uint32_t width, height, depth;
		ResourceFormat textureFormat;
} RendererTexture;

typedef struct RendererTextureView
{
	RendererTexture *parentTexture;
} RendererTextureView;

typedef struct RendererSampler
{
} RendererSampler;

typedef struct TextureSubresourceRange
{
		uint32_t baseMipLevel;
		uint32_t levelCount;
		uint32_t baseArrayLayer;
		uint32_t layerCount;
} TextureSubresourceRange;

typedef struct TextureSubresourceLayers
{
	    uint32_t              mipLevel;
	    uint32_t              baseArrayLayer;
	    uint32_t              layerCount;
} TextureSubresourceLayers;

typedef struct RendererStagingBuffer
{
	size_t bufferSize;
} RendererStagingBuffer;

typedef struct RendererBuffer
{
	size_t bufferSize;
	BufferUsageType usage;
	bool canBeTransferSrc;
	bool canBeTransferDst;
} RendererBuffer;

// Note at least for now I'm disregarding stencil operations
typedef struct RendererAttachmentDescription
{
		ResourceFormat format;
		AttachmentLoadOp loadOp;
		AttachmentStoreOp storeOp;
		TextureLayout initialLayout;
		TextureLayout finalLayout;
} AttachmentDescription;

typedef struct RendererAttachmentReference
{
		uint32_t attachment;
		TextureLayout layout;
} AttachmentReference;

typedef struct RendererSupassDependency
{
		uint32_t srcSubpasss;
		uint32_t dstSubpass;
		PipelineStageFlags srcStageMask;
		PipelineStageFlags dstStageMask;
		AccessFlags srcAccessMask;
		AccessFlags dstAccessMask;
		bool byRegionDependency;
} SubpassDependency;

typedef struct RendererSubpassDescription
{
		PipelineBindPoint bindPoint;
		std::vector<AttachmentReference> colorAttachments;
		std::vector<AttachmentReference> inputAttachments;
		AttachmentReference *depthStencilAttachment;
		std::vector<uint32_t> preserveAttachments;
} SubpassDescription;

typedef struct RendererRenderPass
{
		std::vector<AttachmentDescription> attachments;
		std::vector<SubpassDescription> subpasses;
		std::vector<SubpassDependency> subpassDependencies;
} RendererRenderPass;

typedef struct RendererDescriptorSetLayoutBinding
{
		uint32_t binding;
		DescriptorType descriptorType;
		uint32_t descriptorCount;
		ShaderStageFlags stageFlags;
		// TODO Add immutable sampler functionality, but don't forget the descriptor set layout cache also needs to be updated for this!!!!!!!!

		bool operator< (const RendererDescriptorSetLayoutBinding &other)
		{
			return binding < other.binding;
		}

		bool operator== (const RendererDescriptorSetLayoutBinding &other)
		{
			return binding == other.binding && descriptorType == other.descriptorType && descriptorCount == other.descriptorCount && stageFlags == other.stageFlags;
		}

		bool operator!= (const RendererDescriptorSetLayoutBinding &other)
		{
			return !(operator==(other));
		}
} DescriptorSetLayoutBinding;

/*
typedef struct RendererDescriptorSetLayout
{
		std::vector<DescriptorSetLayoutBinding> bindings;
} RendererDescriptorSetLayout;
*/

typedef struct RendererShaderModule
{
		ShaderStageFlagBits stage;

} RendererShaderModule;

typedef struct RendererViewport
{
		float x;
		float y;
		float width;
		float height;
		float minDepth;
		float maxDepth;
} Viewport;

typedef struct RendererScissor
{
		int32_t x;
		int32_t y;
		uint32_t width;
		uint32_t height;
} Scissor;

//f//

typedef struct RendererPushConstantRange
{
		size_t offset;
		size_t size;
		PipelineStageFlags stageFlags;
} PushConstantRange;

typedef struct RendererPipelineShaderStage
{
		RendererShaderModule *module;
		const char *entry;
		// Also eventually specialization constants
} PipelineShaderStage;

typedef struct RendererVertexInputBinding
{
		uint32_t binding;
		uint32_t stride;
		VertexInputRate inputRate;
} VertexInputBinding;

typedef struct RendererVertexInputAttrib
{
		uint32_t binding;
		uint32_t location;
		ResourceFormat format;
		uint32_t offset;
} VertexInputAttrib;

typedef struct RendererPipelineVertexInputInfo
{
		std::vector<VertexInputBinding> vertexInputBindings;
		std::vector<VertexInputAttrib> vertexInputAttribs;
} PipelineVertexInputInfo;

typedef struct RendererPipelineInputAssemblyInfo
{
		PrimitiveTopology topology;
		bool primitiveRestart;
} PipelineInputAssemblyInfo;

typedef struct RendererPipelineTessellationInfo
{
		uint32_t patchControlPoints;
} PipelineTessellationInfo;

typedef struct RendererPipelineViewportInfo
{
		std::vector<Viewport> viewports;
		std::vector<Scissor> scissors;
} PipelineViewportInfo;

typedef struct RendererPipelineRasterizationInfo
{
		bool depthClampEnable;
		bool rasterizerDiscardEnable;
		bool clockwiseFrontFace;
		bool enableOutOfOrderRasterization;
		PolygonMode polygonMode;
		CullModeFlags cullMode;
		float lineWidth;
		// Also some depth bias stuff I might implement later

} PipelineRasterizationInfo;

typedef struct RendererPipelineMultisampleInfo
{
		// As of now multisampling is NOT supported
} PipelineMultisampleInfo;

typedef struct RendererPipelineDepthStencilInfo
{
		bool enableDepthTest;
		bool enableDepthWrite;
		CompareOp depthCompareOp;
		bool depthBoundsTestEnable;
		float minDepthBounds;
		float maxDepthBounds;

		// Note as of now I'm not supporting stencil operations
} PipelineDepthStencilInfo;

typedef struct RendererPipelineColorBlendAttachment
{
		bool blendEnable;
		BlendFactor srcColorBlendFactor;
		BlendFactor dstColorBlendFactor;
		BlendOp colorBlendOp;
		BlendFactor srcAlphaBlendFactor;
		BlendFactor dstAlphaBlendFactor;
		BlendOp alphaBlendOp;
		ColorComponentFlags colorWriteMask;
} PipelineColorBlendAttachment;

typedef struct RendererPipelineColorBlendInfo
{
		bool logicOpEnable;
		LogicOp logicOp;
		std::vector<PipelineColorBlendAttachment> attachments;
		float blendConstants[4];

} PipelineColorBlendInfo;

typedef struct RendererPipelineDynamicStateInfo
{
		std::vector<DynamicState> dynamicStates;
} PipelineDynamicStateInfo;

typedef struct RendererGraphicsPipelineInfo
{
		std::vector<PipelineShaderStage> stages;
		PipelineVertexInputInfo vertexInputInfo;
		PipelineInputAssemblyInfo inputAssemblyInfo;
		PipelineTessellationInfo tessellationInfo;
		PipelineViewportInfo viewportInfo;
		PipelineRasterizationInfo rasterizationInfo;
		PipelineMultisampleInfo multisampleInfo;
		PipelineDepthStencilInfo depthStencilInfo;
		PipelineColorBlendInfo colorBlendInfo;
		PipelineDynamicStateInfo dynamicStateInfo;

		std::vector<PushConstantRange> inputPushConstantRanges;
		std::vector<std::vector<DescriptorSetLayoutBinding> > inputSetLayouts;

		//PipelineInputLayout inputLayout;
		//RendererRenderPass *renderPass;
		//uint32_t subpass;

} GraphicsPipelineInfo;

typedef struct RendererComputePipelineInfo
{
	PipelineShaderStage shader;

	std::vector<PushConstantRange> inputPushConstantRanges;
	std::vector<std::vector<DescriptorSetLayoutBinding> > inputSetLayouts;

} ComputePipelineInfo;

typedef struct RendererDescriptorSet
{

} RendererDescriptorSet;

typedef struct RendererPipeline
{

} RendererPipeline;

typedef struct RendererFramebuffer
{

} RendererFramebuffer;

typedef struct RendererDescriptorImageInfo
{
		RendererSampler *sampler;
		RendererTextureView *view;
		TextureLayout layout;

} DescriptorImageInfo;

typedef struct RendererDescriptorBufferInfo
{
		RendererBuffer *buffer;
		size_t offset;
		size_t range;
} DescriptorBufferInfo;

typedef struct RendererDescriptorWriteInfo
{
		RendererDescriptorSet *dstSet;
		uint32_t dstBinding;
		uint32_t dstArrayElement;
		uint32_t descriptorCount;
		DescriptorType descriptorType;

		std::vector<DescriptorImageInfo> imageInfo;
		std::vector<DescriptorBufferInfo> bufferInfo;

} DescriptorWriteInfo;

typedef union VRendererClearColorValue {
    float       float32[4];
    int32_t     int32[4];
    uint32_t    uint32[4];
} ClearColorValue;

typedef struct RendererClearDepthStencilValue {
    float       depth;
    uint32_t    stencil;
} ClearDepthStencilValue;

typedef union RendererClearValue {
    ClearColorValue           color;
    ClearDepthStencilValue    depthStencil;
} ClearValue;

typedef struct TextureBlitInfo
{
		TextureSubresourceLayers srcSubresource;
		sivec3 srcOffsets[2];
		TextureSubresourceLayers dstSubresource;
		sivec3 dstOffsets[2];
} TextureBlitInfo;

//f//

typedef struct RendererFence
{
} RendererFence;

typedef struct RendererSemaphore
{

} RendererSemaphore;

typedef RendererRenderPass *RenderPass;
typedef RendererPipeline *Pipeline;
//typedef RendererDescriptorSetLayout *DescriptorSetLayout;
typedef RendererDescriptorSet *DescriptorSet;
typedef RendererShaderModule *ShaderModule;
typedef RendererFramebuffer *Framebuffer;
typedef RendererTexture *Texture;
typedef RendererTextureView *TextureView;
typedef RendererSampler *Sampler;
typedef RendererBuffer *Buffer;
//typedef RendererCommandPool *CommandPool;
typedef RendererStagingBuffer *StagingBuffer;
typedef RendererFence *Fence;
typedef RendererSemaphore *Semaphore;

#include <Rendering/Renderer/RendererCommandPool.h>
#include <Rendering/Renderer/RendererCommandBuffer.h>
#include <Rendering/Renderer/RendererDescriptorPool.h>

#endif /* RENDERING_RENDERER_RENDEREROBJECTS_H_ */
