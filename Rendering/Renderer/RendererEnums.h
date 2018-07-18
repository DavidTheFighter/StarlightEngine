/*
 * TextureFormats.h
 *
 *  Created on: Sep 15, 2017
 *      Author: david
 */

#ifndef RENDERING_RENDERER_RENDERERENUMS_H_
#define RENDERING_RENDERER_RENDERERENUMS_H_

typedef enum ShaderSourceLanguage
{
	SHADER_LANGUAGE_GLSL = 0,
	SHADER_LANGUAGE_HLSL = 1,
	SHADER_LANGUAGE_MAX_ENUM = 0x7FFFFFFF
} ShaderSourceLanguage;

typedef enum MemoryUsage
{
    MEMORY_USAGE_UNKNOWN = 0,
    MEMORY_USAGE_GPU_ONLY = 1,
    MEMORY_USAGE_CPU_ONLY = 2,
    MEMORY_USAGE_CPU_TO_GPU = 3,
    //MEMORY_USAGE_GPU_TO_CPU = 4,
    MEMORY_USAGE_MAX_ENUM = 0x7FFFFFFF
} MemoryUsage;

typedef enum CommandPoolFlagBits
{
	COMMAND_POOL_TRANSIENT_BIT = 0x00000001,
	COMMAND_POOL_RESET_COMMAND_BUFFER_BIT = 0x00000002,
	COMMAND_POOL_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} CommandPoolFlagBits;

typedef uint32_t CommandPoolFlags;

typedef enum CommandBufferLevel
{
	COMMAND_BUFFER_LEVEL_PRIMARY = 0,
	COMMAND_BUFFER_LEVEL_SECONDARY = 1,
	COMMAND_BUFFER_LEVEL_MAX_ENUM = 0x7FFFFFFF
} CommandBufferLevel;

typedef enum AttachmentLoadOp
{
	ATTACHMENT_LOAD_OP_LOAD = 0,
	ATTACHMENT_LOAD_OP_CLEAR = 1,
	ATTACHMENT_LOAD_OP_DONT_CARE = 2,
	ATTACHMENT_LOAD_OP_MAX_ENUM = 0x7FFFFFFF
} AttachmentLoadOp;

typedef enum AttachmentStoreOp
{
	ATTACHMENT_STORE_OP_STORE = 0,
	ATTACHMENT_STORE_OP_DONT_CARE = 1,
	ATTACHMENT_STORE_OP_MAX_ENUM = 0x7FFFFFFF
} AttachmentStoreOp;

typedef enum PipelineBindPoint
{
	PIPELINE_BIND_POINT_GRAPHICS = 0,
	PIPELINE_BIND_POINT_COMPUTE = 1,
	PIPELINE_BIND_POINT_MAX_ENUM = 0x7FFFFFFF
} PipelineBindPoint;

typedef enum DescriptorType
{
    DESCRIPTOR_TYPE_SAMPLER = 0,
    DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
    DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
    DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
    DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
    DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
    DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
    DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
    DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
    DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
    DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,
    DESCRIPTOR_TYPE_MAX_ENUM = 0x7FFFFFFF
} DescriptorType;

typedef enum PipelineStageFlagBits
{
    PIPELINE_STAGE_TOP_OF_PIPE_BIT = 0x00000001,
    PIPELINE_STAGE_DRAW_INDIRECT_BIT = 0x00000002,
    PIPELINE_STAGE_VERTEX_INPUT_BIT = 0x00000004,
    PIPELINE_STAGE_VERTEX_SHADER_BIT = 0x00000008,
    PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT = 0x00000010,
    PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT = 0x00000020,
    PIPELINE_STAGE_GEOMETRY_SHADER_BIT = 0x00000040,
    PIPELINE_STAGE_FRAGMENT_SHADER_BIT = 0x00000080,
    PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT = 0x00000100,
    PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT = 0x00000200,
    PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = 0x00000400,
    PIPELINE_STAGE_COMPUTE_SHADER_BIT = 0x00000800,
    PIPELINE_STAGE_TRANSFER_BIT = 0x00001000,
    PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x00002000,
    PIPELINE_STAGE_HOST_BIT = 0x00004000,
    PIPELINE_STAGE_ALL_GRAPHICS_BIT = 0x00008000,
    PIPELINE_STAGE_ALL_COMMANDS_BIT = 0x00010000,
    PIPELINE_STAGE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} PipelineStageFlagBits;

typedef uint32_t PipelineStageFlags;

typedef enum ShaderStageFlagBits
{
    SHADER_STAGE_VERTEX_BIT = 0x00000001,
    SHADER_STAGE_TESSELLATION_CONTROL_BIT = 0x00000002,
    SHADER_STAGE_TESSELLATION_EVALUATION_BIT = 0x00000004,
    SHADER_STAGE_GEOMETRY_BIT = 0x00000008,
    SHADER_STAGE_FRAGMENT_BIT = 0x00000010,
    SHADER_STAGE_COMPUTE_BIT = 0x00000020,
    SHADER_STAGE_ALL_GRAPHICS = 0x0000001F,
    SHADER_STAGE_ALL = 0x7FFFFFFF,
    SHADER_STAGE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} ShaderStageFlagBits;

typedef uint32_t ShaderStageFlags;

typedef enum VertexInputRate
{
    VERTEX_INPUT_RATE_VERTEX = 0,
    VERTEX_INPUT_RATE_INSTANCE = 1,
    VERTEX_INPUT_RATE_MAX_ENUM = 0x7FFFFFFF
} VertexInputRate;

typedef enum PrimitiveTopology
{
    PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
    PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
    PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
    PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
    PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5,
    PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY = 6,
    PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY = 7,
    PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY = 8,
    PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY = 9,
    PRIMITIVE_TOPOLOGY_PATCH_LIST = 10,
    PRIMITIVE_TOPOLOGY_MAX_ENUM = 0x7FFFFFFF
} PrimitiveTopology;

typedef enum PolygonMode
{
    POLYGON_MODE_FILL = 0,
    POLYGON_MODE_LINE = 1,
    POLYGON_MODE_POINT = 2,
    POLYGON_MODE_MAX_ENUM = 0x7FFFFFFF
} PolygonMode;

typedef enum CullModeFlagBits
{
    CULL_MODE_NONE = 0,
    CULL_MODE_FRONT_BIT = 0x00000001,
    CULL_MODE_BACK_BIT = 0x00000002,
    CULL_MODE_FRONT_AND_BACK = 0x00000003,
    CULL_MODE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} CullModeFlagBits;

typedef uint32_t CullModeFlags;

typedef enum CompareOp
{
    COMPARE_OP_NEVER = 0,
    COMPARE_OP_LESS = 1,
    COMPARE_OP_EQUAL = 2,
    COMPARE_OP_LESS_OR_EQUAL = 3,
    COMPARE_OP_GREATER = 4,
    COMPARE_OP_NOT_EQUAL = 5,
    COMPARE_OP_GREATER_OR_EQUAL = 6,
    COMPARE_OP_ALWAYS = 7,
    COMPARE_OP_MAX_ENUM = 0x7FFFFFFF
} CompareOp;

typedef enum LogicOp
{
    LOGIC_OP_CLEAR = 0,
    LOGIC_OP_AND = 1,
    LOGIC_OP_AND_REVERSE = 2,
    LOGIC_OP_COPY = 3,
    LOGIC_OP_AND_INVERTED = 4,
    LOGIC_OP_NO_OP = 5,
    LOGIC_OP_XOR = 6,
    LOGIC_OP_OR = 7,
    LOGIC_OP_NOR = 8,
    LOGIC_OP_EQUIVALENT = 9,
    LOGIC_OP_INVERT = 10,
    LOGIC_OP_OR_REVERSE = 11,
    LOGIC_OP_COPY_INVERTED = 12,
    LOGIC_OP_OR_INVERTED = 13,
    LOGIC_OP_NAND = 14,
    LOGIC_OP_SET = 15,
    LOGIC_OP_MAX_ENUM = 0x7FFFFFFF
} LogicOp;

typedef enum BlendFactor
{
    BLEND_FACTOR_ZERO = 0,
    BLEND_FACTOR_ONE = 1,
    BLEND_FACTOR_SRC_COLOR = 2,
    BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3,
    BLEND_FACTOR_DST_COLOR = 4,
    BLEND_FACTOR_ONE_MINUS_DST_COLOR = 5,
    BLEND_FACTOR_SRC_ALPHA = 6,
    BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7,
    BLEND_FACTOR_DST_ALPHA = 8,
    BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
    BLEND_FACTOR_CONSTANT_COLOR = 10,
    BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR = 11,
    BLEND_FACTOR_CONSTANT_ALPHA = 12,
    BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA = 13,
    BLEND_FACTOR_SRC_ALPHA_SATURATE = 14,
    BLEND_FACTOR_SRC1_COLOR = 15,
    BLEND_FACTOR_ONE_MINUS_SRC1_COLOR = 16,
    BLEND_FACTOR_SRC1_ALPHA = 17,
    BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA = 18,
    BLEND_FACTOR_MAX_ENUM = 0x7FFFFFFF
} BlendFactor;

typedef enum BlendOp
{
    BLEND_OP_ADD = 0,
    BLEND_OP_SUBTRACT = 1,
    BLEND_OP_REVERSE_SUBTRACT = 2,
    BLEND_OP_MIN = 3,
    BLEND_OP_MAX = 4,
    BLEND_OP_MAX_ENUM = 0x7FFFFFFF
} BlendOp;

typedef enum ColorComponentFlagBits
{
    COLOR_COMPONENT_R_BIT = 0x00000001,
    COLOR_COMPONENT_G_BIT = 0x00000002,
    COLOR_COMPONENT_B_BIT = 0x00000004,
    COLOR_COMPONENT_A_BIT = 0x00000008,
    COLOR_COMPONENT_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} ColorComponentFlagBits;

typedef uint32_t ColorComponentFlags;

typedef enum DynamicState
{
    DYNAMIC_STATE_VIEWPORT = 0,
    DYNAMIC_STATE_SCISSOR = 1,
    DYNAMIC_STATE_LINE_WIDTH = 2,
    DYNAMIC_STATE_DEPTH_BIAS = 3,
    DYNAMIC_STATE_BLEND_CONSTANTS = 4,
    DYNAMIC_STATE_DEPTH_BOUNDS = 5,
    DYNAMIC_STATE_STENCIL_COMPARE_MASK = 6,
    DYNAMIC_STATE_STENCIL_WRITE_MASK = 7,
    DYNAMIC_STATE_STENCIL_REFERENCE = 8,
    DYNAMIC_STATE_MAX_ENUM = 0x7FFFFFFF
} DynamicState;

typedef enum AccessFlagBits
{
    ACCESS_INDIRECT_COMMAND_READ_BIT = 0x00000001,
    ACCESS_INDEX_READ_BIT = 0x00000002,
    ACCESS_VERTEX_ATTRIBUTE_READ_BIT = 0x00000004,
    ACCESS_UNIFORM_READ_BIT = 0x00000008,
    ACCESS_INPUT_ATTACHMENT_READ_BIT = 0x00000010,
    ACCESS_SHADER_READ_BIT = 0x00000020,
    ACCESS_SHADER_WRITE_BIT = 0x00000040,
    ACCESS_COLOR_ATTACHMENT_READ_BIT = 0x00000080,
    ACCESS_COLOR_ATTACHMENT_WRITE_BIT = 0x00000100,
    ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT = 0x00000200,
    ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = 0x00000400,
    ACCESS_TRANSFER_READ_BIT = 0x00000800,
    ACCESS_TRANSFER_WRITE_BIT = 0x00001000,
    ACCESS_HOST_READ_BIT = 0x00002000,
    ACCESS_HOST_WRITE_BIT = 0x00004000,
    ACCESS_MEMORY_READ_BIT = 0x00008000,
    ACCESS_MEMORY_WRITE_BIT = 0x00010000,
    ACCESS_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} AccessFlagBits;

typedef uint32_t AccessFlags;

typedef enum QueueType
{
	QUEUE_TYPE_GRAPHICS = 0,
	QUEUE_TYPE_COMPUTE = 1,
	QUEUE_TYPE_TRANSFER = 2,
	QUEUE_TYPE_MAX_ENUM = 0x7FFFFFFF
} QueueType;

typedef enum CommandBufferUsageFlagBits
{
    COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT = 0x00000001,
    COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT = 0x00000002,
    COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT = 0x00000004,
    COMMAND_BUFFER_USAGE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} CommandBufferUsageFlagBits;

typedef uint32_t CommandBufferUsageFlags;

typedef enum SubpassContents
{
    SUBPASS_CONTENTS_INLINE = 0,
    SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS = 1,
    SUBPASS_CONTENTS_MAX_ENUM = 0x7FFFFFFF
} SubpassContents;

typedef enum BufferUsageType
{
   // BUFFER_USAGE_TRANSFER_SRC_BIT = 0x00000001,
   // BUFFER_USAGE_TRANSFER_DST_BIT = 0x00000002,
    //BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT = 0x00000004,
    //BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT = 0x00000008,
    BUFFER_USAGE_UNIFORM_BUFFER = 0x00000010,
    //BUFFER_USAGE_STORAGE_BUFFER_BIT = 0x00000020,
    BUFFER_USAGE_INDEX_BUFFER = 0x00000040,
    BUFFER_USAGE_VERTEX_BUFFER = 0x00000080,
    BUFFER_USAGE_INDIRECT_BUFFER = 0x00000100,
    BUFFER_USAGE_MAX_ENUM = 0x7FFFFFFF
} BufferUsageType;

//typedef uint32_t BufferUsageFlags;

typedef enum TextureLayout
{
    TEXTURE_LAYOUT_UNDEFINED = 0,
    TEXTURE_LAYOUT_GENERAL = 1,
    TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2,
    TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
    TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4,
    TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5,
    TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL = 6,
    TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL = 7,
    TEXTURE_LAYOUT_PREINITIALIZED = 8,
    TEXTURE_LAYOUT_MAX_ENUM = 0x7FFFFFFF
} TextureLayout;

typedef enum TextureUsageFlagBits
{
	TEXTURE_USAGE_TRANSFER_SRC_BIT = 0x00000001,
	TEXTURE_USAGE_TRANSFER_DST_BIT = 0x00000002,
	TEXTURE_USAGE_SAMPLED_BIT = 0x00000004,
	TEXTURE_USAGE_STORAGE_BIT = 0x00000008,
	TEXTURE_USAGE_COLOR_ATTACHMENT_BIT = 0x00000010,
	TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020,
	TEXTURE_USAGE_TRANSIENT_ATTACHMENT_BIT = 0x00000040,
	TEXTURE_USAGE_INPUT_ATTACHMENT_BIT = 0x00000080,
	TEXTURE_USAGE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} TextureUsageFlagBits;

typedef uint32_t TextureUsageFlags;

typedef enum TextureType
{
	TEXTURE_TYPE_1D = 0,
	TEXTURE_TYPE_2D = 1,
	TEXTURE_TYPE_3D = 2,
	TEXTURE_TYPE_MAX_ENUM
} TextureType;

typedef enum TextureViewType
{
	TEXTURE_VIEW_TYPE_1D = 0,
	TEXTURE_VIEW_TYPE_2D = 1,
	TEXTURE_VIEW_TYPE_3D = 2,
	TEXTURE_VIEW_TYPE_CUBE = 3,
	TEXTURE_VIEW_TYPE_1D_ARRAY = 4,
	TEXTURE_VIEW_TYPE_2D_ARRAY = 5,
	TEXTURE_VIEW_TYPE_CUBE_ARRAY = 6,
	TEXTURE_VIEW_TYPE_MAX_ENUM
} TextureViewType;

typedef enum SamplerFilter
{
	SAMPLER_FILTER_NEAREST = 0,
	SAMPLER_FILTER_LINEAR = 1,
	SAMPLER_FILTER_MAX_ENUM = 0x7FFFFFFF
} SamplerFilter;

typedef enum SamplerMipmapMode
{
	SAMPLER_MIPMAP_MODE_NEAREST = 0,
	SAMPLER_MIPMAP_MODE_LINEAR = 1,
	SAMPLER_MIPMAP_MODE_MAX_ENUM = 0x7FFFFFFF
} SamplerMipmapMode;

typedef enum SamplerAddressMode
{
	SAMPLER_ADDRESS_MODE_REPEAT = 0,
	SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1,
	SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
	SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3,
	SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4,
	SAMPLER_ADDRESS_MODE_MAX_ENUM = 0x7FFFFFFF
} SamplerAddressMode;

typedef enum ResourceFormat
{
	RESOURCE_FORMAT_UNDEFINED = 0,
	//RESOURCE_FORMAT_R4G4_UNORM_PACK8 = 1,
	//RESOURCE_FORMAT_R4G4B4A4_UNORM_PACK16 = 2,
	RESOURCE_FORMAT_B4G4R4A4_UNORM_PACK16 = 3,
	//RESOURCE_FORMAT_R5G6B5_UNORM_PACK16 = 4,
	RESOURCE_FORMAT_B5G6R5_UNORM_PACK16 = 5,
	//RESOURCE_FORMAT_R5G5B5A1_UNORM_PACK16 = 6,
	RESOURCE_FORMAT_B5G5R5A1_UNORM_PACK16 = 7,
	//RESOURCE_FORMAT_A1R5G5B5_UNORM_PACK16 = 8,
	RESOURCE_FORMAT_R8_UNORM = 9,
	RESOURCE_FORMAT_R8_SNORM = 10,
	//RESOURCE_FORMAT_R8_USCALED = 11,
	//RESOURCE_FORMAT_R8_SSCALED = 12,
	RESOURCE_FORMAT_R8_UINT = 13,
	RESOURCE_FORMAT_R8_SINT = 14,
	//RESOURCE_FORMAT_R8_SRGB = 15,
	RESOURCE_FORMAT_R8G8_UNORM = 16,
	RESOURCE_FORMAT_R8G8_SNORM = 17,
	//RESOURCE_FORMAT_R8G8_USCALED = 18,
	//RESOURCE_FORMAT_R8G8_SSCALED = 19,
	RESOURCE_FORMAT_R8G8_UINT = 20,
	RESOURCE_FORMAT_R8G8_SINT = 21,
	//RESOURCE_FORMAT_R8G8_SRGB = 22,
	//RESOURCE_FORMAT_R8G8B8_UNORM = 23,
	//RESOURCE_FORMAT_R8G8B8_SNORM = 24,
	//RESOURCE_FORMAT_R8G8B8_USCALED = 25,
	//RESOURCE_FORMAT_R8G8B8_SSCALED = 26,
	//RESOURCE_FORMAT_R8G8B8_UINT = 27,
	//RESOURCE_FORMAT_R8G8B8_SINT = 28,
	//RESOURCE_FORMAT_R8G8B8_SRGB = 29,
	//RESOURCE_FORMAT_B8G8R8_UNORM = 30,
	//RESOURCE_FORMAT_B8G8R8_SNORM = 31,
	//RESOURCE_FORMAT_B8G8R8_USCALED = 32,
	//RESOURCE_FORMAT_B8G8R8_SSCALED = 33,
	//RESOURCE_FORMAT_B8G8R8_UINT = 34,
	//RESOURCE_FORMAT_B8G8R8_SINT = 35,
	//RESOURCE_FORMAT_B8G8R8_SRGB = 36,
	RESOURCE_FORMAT_R8G8B8A8_UNORM = 37,
	RESOURCE_FORMAT_R8G8B8A8_SNORM = 38,
	//RESOURCE_FORMAT_R8G8B8A8_USCALED = 39,
	//RESOURCE_FORMAT_R8G8B8A8_SSCALED = 40,
	RESOURCE_FORMAT_R8G8B8A8_UINT = 41,
	RESOURCE_FORMAT_R8G8B8A8_SINT = 42,
	RESOURCE_FORMAT_R8G8B8A8_SRGB = 43,
	RESOURCE_FORMAT_B8G8R8A8_UNORM = 44,
	//RESOURCE_FORMAT_B8G8R8A8_SNORM = 45,
	//RESOURCE_FORMAT_B8G8R8A8_USCALED = 46,
	//RESOURCE_FORMAT_B8G8R8A8_SSCALED = 47,
	//RESOURCE_FORMAT_B8G8R8A8_UINT = 48,
	//RESOURCE_FORMAT_B8G8R8A8_SINT = 49,
	RESOURCE_FORMAT_B8G8R8A8_SRGB = 50,
	//RESOURCE_FORMAT_A8B8G8R8_UNORM_PACK32 = 51,
	//RESOURCE_FORMAT_A8B8G8R8_SNORM_PACK32 = 52,
	//RESOURCE_FORMAT_A8B8G8R8_USCALED_PACK32 = 53,
	//RESOURCE_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54,
	//RESOURCE_FORMAT_A8B8G8R8_UINT_PACK32 = 55,
	//RESOURCE_FORMAT_A8B8G8R8_SINT_PACK32 = 56,
	//RESOURCE_FORMAT_A8B8G8R8_SRGB_PACK32 = 57,
	RESOURCE_FORMAT_A2R10G10B10_UNORM_PACK32 = 58,
	//RESOURCE_FORMAT_A2R10G10B10_SNORM_PACK32 = 59,
	//RESOURCE_FORMAT_A2R10G10B10_USCALED_PACK32 = 60,
	//RESOURCE_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61,
	RESOURCE_FORMAT_A2R10G10B10_UINT_PACK32 = 62,
	//RESOURCE_FORMAT_A2R10G10B10_SINT_PACK32 = 63,
	//RESOURCE_FORMAT_A2B10G10R10_UNORM_PACK32 = 64,
	//RESOURCE_FORMAT_A2B10G10R10_SNORM_PACK32 = 65,
	//RESOURCE_FORMAT_A2B10G10R10_USCALED_PACK32 = 66,
	//RESOURCE_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67,
	//RESOURCE_FORMAT_A2B10G10R10_UINT_PACK32 = 68,
	//RESOURCE_FORMAT_A2B10G10R10_SINT_PACK32 = 69,
	RESOURCE_FORMAT_R16_UNORM = 70,
	RESOURCE_FORMAT_R16_SNORM = 71,
	//RESOURCE_FORMAT_R16_USCALED = 72,
	//RESOURCE_FORMAT_R16_SSCALED = 73,
	RESOURCE_FORMAT_R16_UINT = 74,
	RESOURCE_FORMAT_R16_SINT = 75,
	RESOURCE_FORMAT_R16_SFLOAT = 76,
	RESOURCE_FORMAT_R16G16_UNORM = 77,
	RESOURCE_FORMAT_R16G16_SNORM = 78,
	//RESOURCE_FORMAT_R16G16_USCALED = 79,
	//RESOURCE_FORMAT_R16G16_SSCALED = 80,
	RESOURCE_FORMAT_R16G16_UINT = 81,
	RESOURCE_FORMAT_R16G16_SINT = 82,
	RESOURCE_FORMAT_R16G16_SFLOAT = 83,
	//RESOURCE_FORMAT_R16G16B16_UNORM = 84,
	//RESOURCE_FORMAT_R16G16B16_SNORM = 85,
	//RESOURCE_FORMAT_R16G16B16_USCALED = 86,
	//RESOURCE_FORMAT_R16G16B16_SSCALED = 87,
	//RESOURCE_FORMAT_R16G16B16_UINT = 88,
	//RESOURCE_FORMAT_R16G16B16_SINT = 89,
	//RESOURCE_FORMAT_R16G16B16_SFLOAT = 90,
	RESOURCE_FORMAT_R16G16B16A16_UNORM = 91,
	RESOURCE_FORMAT_R16G16B16A16_SNORM = 92,
	//RESOURCE_FORMAT_R16G16B16A16_USCALED = 93,
	//RESOURCE_FORMAT_R16G16B16A16_SSCALED = 94,
	RESOURCE_FORMAT_R16G16B16A16_UINT = 95,
	RESOURCE_FORMAT_R16G16B16A16_SINT = 96,
	RESOURCE_FORMAT_R16G16B16A16_SFLOAT = 97,
	RESOURCE_FORMAT_R32_UINT = 98,
	RESOURCE_FORMAT_R32_SINT = 99,
	RESOURCE_FORMAT_R32_SFLOAT = 100,
	RESOURCE_FORMAT_R32G32_UINT = 101,
	RESOURCE_FORMAT_R32G32_SINT = 102,
	RESOURCE_FORMAT_R32G32_SFLOAT = 103,
	RESOURCE_FORMAT_R32G32B32_UINT = 104,
	RESOURCE_FORMAT_R32G32B32_SINT = 105,
	RESOURCE_FORMAT_R32G32B32_SFLOAT = 106,
	RESOURCE_FORMAT_R32G32B32A32_UINT = 107,
	RESOURCE_FORMAT_R32G32B32A32_SINT = 108,
	RESOURCE_FORMAT_R32G32B32A32_SFLOAT = 109,
	RESOURCE_FORMAT_R64_UINT_VULKAN_ONLY = 110,
	RESOURCE_FORMAT_R64_SINT_VULKAN_ONLY = 111,
	RESOURCE_FORMAT_R64_SFLOAT_VULKAN_ONLY = 112,
	RESOURCE_FORMAT_R64G64_UINT_VULKAN_ONLY = 113,
	RESOURCE_FORMAT_R64G64_SINT_VULKAN_ONLY = 114,
	RESOURCE_FORMAT_R64G64_SFLOAT_VULKAN_ONLY = 115,
	RESOURCE_FORMAT_R64G64B64_UINT_VULKAN_ONLY = 116,
	RESOURCE_FORMAT_R64G64B64_SINT_VULKAN_ONLY = 117,
	RESOURCE_FORMAT_R64G64B64_SFLOAT_VULKAN_ONLY = 118,
	RESOURCE_FORMAT_R64G64B64A64_UINT_VULKAN_ONLY = 119,
	RESOURCE_FORMAT_R64G64B64A64_SINT_VULKAN_ONLY = 120,
	RESOURCE_FORMAT_R64G64B64A64_SFLOAT_VULKAN_ONLY = 121,
	RESOURCE_FORMAT_B10G11R11_UFLOAT_PACK32 = 122,
	RESOURCE_FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123,
	RESOURCE_FORMAT_D16_UNORM = 124,
	//RESOURCE_FORMAT_X8_D24_UNORM_PACK32 = 125,
	RESOURCE_FORMAT_D32_SFLOAT = 126,
	//RESOURCE_FORMAT_S8_UINT = 127,
	//RESOURCE_FORMAT_D16_UNORM_S8_UINT = 128,
	RESOURCE_FORMAT_D24_UNORM_S8_UINT = 129,
	//RESOURCE_FORMAT_D32_SFLOAT_S8_UINT = 130,
	//RESOURCE_FORMAT_BC1_RGB_UNORM_BLOCK = 131,
	//RESOURCE_FORMAT_BC1_RGB_SRGB_BLOCK = 132,
	RESOURCE_FORMAT_BC1_RGBA_UNORM_BLOCK = 133,
	RESOURCE_FORMAT_BC1_RGBA_SRGB_BLOCK = 134,
	RESOURCE_FORMAT_BC2_UNORM_BLOCK = 135,
	RESOURCE_FORMAT_BC2_SRGB_BLOCK = 136,
	RESOURCE_FORMAT_BC3_UNORM_BLOCK = 137,
	RESOURCE_FORMAT_BC3_SRGB_BLOCK = 138,
	RESOURCE_FORMAT_BC4_UNORM_BLOCK = 139,
	RESOURCE_FORMAT_BC4_SNORM_BLOCK = 140,
	RESOURCE_FORMAT_BC5_UNORM_BLOCK = 141,
	RESOURCE_FORMAT_BC5_SNORM_BLOCK = 142,
	RESOURCE_FORMAT_BC6H_UFLOAT_BLOCK = 143,
	RESOURCE_FORMAT_BC6H_SFLOAT_BLOCK = 144,
	RESOURCE_FORMAT_BC7_UNORM_BLOCK = 145,
	RESOURCE_FORMAT_BC7_SRGB_BLOCK = 146,
	RESOURCE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK_VULKAN_ONLY = 147,
	RESOURCE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK_VULKAN_ONLY = 148,
	RESOURCE_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK_VULKAN_ONLY = 149,
	RESOURCE_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK_VULKAN_ONLY = 150,
	RESOURCE_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK_VULKAN_ONLY = 151,
	RESOURCE_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK_VULKAN_ONLY = 152,
	RESOURCE_FORMAT_EAC_R11_UNORM_BLOCK_VULKAN_ONLY = 153,
	RESOURCE_FORMAT_EAC_R11_SNORM_BLOCK_VULKAN_ONLY = 154,
	RESOURCE_FORMAT_EAC_R11G11_UNORM_BLOCK_VULKAN_ONLY = 155,
	RESOURCE_FORMAT_EAC_R11G11_SNORM_BLOCK_VULKAN_ONLY = 156,
	RESOURCE_FORMAT_ASTC_4x4_UNORM_BLOCK_VULKAN_ONLY = 157,
	RESOURCE_FORMAT_ASTC_4x4_SRGB_BLOCK_VULKAN_ONLY = 158,
	RESOURCE_FORMAT_ASTC_5x4_UNORM_BLOCK_VULKAN_ONLY = 159,
	RESOURCE_FORMAT_ASTC_5x4_SRGB_BLOCK_VULKAN_ONLY = 160,
	RESOURCE_FORMAT_ASTC_5x5_UNORM_BLOCK_VULKAN_ONLY = 161,
	RESOURCE_FORMAT_ASTC_5x5_SRGB_BLOCK_VULKAN_ONLY = 162,
	RESOURCE_FORMAT_ASTC_6x5_UNORM_BLOCK_VULKAN_ONLY = 163,
	RESOURCE_FORMAT_ASTC_6x5_SRGB_BLOCK_VULKAN_ONLY = 164,
	RESOURCE_FORMAT_ASTC_6x6_UNORM_BLOCK_VULKAN_ONLY = 165,
	RESOURCE_FORMAT_ASTC_6x6_SRGB_BLOCK_VULKAN_ONLY = 166,
	RESOURCE_FORMAT_ASTC_8x5_UNORM_BLOCK_VULKAN_ONLY = 167,
	RESOURCE_FORMAT_ASTC_8x5_SRGB_BLOCK_VULKAN_ONLY = 168,
	RESOURCE_FORMAT_ASTC_8x6_UNORM_BLOCK_VULKAN_ONLY = 169,
	RESOURCE_FORMAT_ASTC_8x6_SRGB_BLOCK_VULKAN_ONLY = 170,
	RESOURCE_FORMAT_ASTC_8x8_UNORM_BLOCK_VULKAN_ONLY = 171,
	RESOURCE_FORMAT_ASTC_8x8_SRGB_BLOCK_VULKAN_ONLY = 172,
	RESOURCE_FORMAT_ASTC_10x5_UNORM_BLOCK_VULKAN_ONLY = 173,
	RESOURCE_FORMAT_ASTC_10x5_SRGB_BLOCK_VULKAN_ONLY = 174,
	RESOURCE_FORMAT_ASTC_10x6_UNORM_BLOCK_VULKAN_ONLY = 175,
	RESOURCE_FORMAT_ASTC_10x6_SRGB_BLOCK_VULKAN_ONLY = 176,
	RESOURCE_FORMAT_ASTC_10x8_UNORM_BLOCK_VULKAN_ONLY = 177,
	RESOURCE_FORMAT_ASTC_10x8_SRGB_BLOCK_VULKAN_ONLY = 178,
	RESOURCE_FORMAT_ASTC_10x10_UNORM_BLOCK_VULKAN_ONLY = 179,
	RESOURCE_FORMAT_ASTC_10x10_SRGB_BLOCK_VULKAN_ONLY = 180,
	RESOURCE_FORMAT_ASTC_12x10_UNORM_BLOCK_VULKAN_ONLY = 181,
	RESOURCE_FORMAT_ASTC_12x10_SRGB_BLOCK_VULKAN_ONLY = 182,
	RESOURCE_FORMAT_ASTC_12x12_UNORM_BLOCK_VULKAN_ONLY = 183,
	RESOURCE_FORMAT_ASTC_12x12_SRGB_BLOCK_VULKAN_ONLY = 184,
	RESOURCE_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG_VULKAN_ONLY = 1000054000,
	RESOURCE_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG_VULKAN_ONLY = 1000054001,
	RESOURCE_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG_VULKAN_ONLY = 1000054002,
	RESOURCE_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG_VULKAN_ONLY = 1000054003,
	RESOURCE_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG_VULKAN_ONLY = 1000054004,
	RESOURCE_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG_VULKAN_ONLY = 1000054005,
	RESOURCE_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG_VULKAN_ONLY = 1000054006,
	RESOURCE_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG_VULKAN_ONLY = 1000054007,
	RESOURCE_FORMAT_MAX_ENUM = 0x7FFFFFFF
} ResourceFormat;

typedef enum RendererObjectType
{
	    OBJECT_TYPE_UNKNOWN = 0,
	    //OBJECT_TYPE_INSTANCE = 1,
	    //OBJECT_TYPE_PHYSICAL_DEVICE = 2,
	    //OBJECT_TYPE_DEVICE = 3,
	    //OBJECT_TYPE_QUEUE = 4,
	    OBJECT_TYPE_SEMAPHORE = 5,
	    OBJECT_TYPE_COMMAND_BUFFER = 6,
	    OBJECT_TYPE_FENCE = 7,
	    //OBJECT_TYPE_DEVICE_MEMORY = 8,
	    OBJECT_TYPE_BUFFER = 9,
	    OBJECT_TYPE_TEXTURE = 10,
	    OBJECT_TYPE_EVENT = 11,
	    //OBJECT_TYPE_QUERY_POOL = 12,
	    OBJECT_TYPE_BUFFER_VIEW = 13,
	    OBJECT_TYPE_TEXTURE_VIEW = 14,
	    OBJECT_TYPE_SHADER_MODULE = 15,
	    //OBJECT_TYPE_PIPELINE_CACHE = 16,
	    OBJECT_TYPE_PIPELINE_LAYOUT = 17,
	    OBJECT_TYPE_RENDER_PASS = 18,
	    OBJECT_TYPE_PIPELINE = 19,
	    //OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT = 20,
	    OBJECT_TYPE_SAMPLER = 21,
	    //OBJECT_TYPE_DESCRIPTOR_POOL = 22,
	    OBJECT_TYPE_DESCRIPTOR_SET = 23,
	    OBJECT_TYPE_FRAMEBUFFER = 24,
	    OBJECT_TYPE_COMMAND_POOL = 25,
	    //OBJECT_TYPE_SURFACE_KHR = 26,
	    //OBJECT_TYPE_SWAPCHAIN_KHR = 27,
	    //OBJECT_TYPE_DEBUG_REPORT_CALLBACK = 28,
	    //OBJECT_TYPE_DISPLAY_KHR = 29,
	    //OBJECT_TYPE_DISPLAY_MODE_KHR = 30,
	    //OBJECT_TYPE_OBJECT_TABLE_NVX = 31,
	    //OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX = 32,
	    //OBJECT_TYPE_VALIDATION_CACHE = 33,
	    OBJECT_TYPE_MAX_ENUM = 0x7FFFFFFF
} RendererObjectType;

inline bool isDepthFormat (ResourceFormat format)
{
	switch (format)
	{
		case RESOURCE_FORMAT_D16_UNORM:
		//case RESOURCE_FORMAT_X8_D24_UNORM_PACK32:
		case RESOURCE_FORMAT_D32_SFLOAT:
		//case RESOURCE_FORMAT_D16_UNORM_S8_UINT:
		case RESOURCE_FORMAT_D24_UNORM_S8_UINT:
		//case RESOURCE_FORMAT_D32_SFLOAT_S8_UINT:
			return true;
		default:
			return false;
	}
}

#endif /* RENDERING_RENDERER_RENDERERENUMS_H_ */
