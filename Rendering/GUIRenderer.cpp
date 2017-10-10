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
 * GUIRenderer.cpp
 * 
 * Created on: Oct 1, 2017
 *     Author: david
 */

#include "Rendering/GUIRenderer.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
//#define NK_INCLUDE_DEFAULT_FONT

#include <nuklear.h>

#include <Rendering/Renderer/Renderer.h>
#include <Engine/StarlightEngine.h>
#include <Resources/ResourceManager.h>

#include <Input/Window.h>

GUIRenderer::GUIRenderer (Renderer *engineRenderer)
{
	renderer = engineRenderer;

	guiGraphicsPipelineInputLayout = nullptr;
	guiGraphicsPipeline = nullptr;
	guiRenderPass = nullptr;

	testGUICommandPool = nullptr;

	testSampler = nullptr;

	guiVertexStreamBuffer = nullptr;
	guiIndexStreamBuffer = nullptr;

	fontAtlas = nullptr;
	fontAtlasView = nullptr;
	fontAtlasDescriptor = nullptr;

	guiTextureDescriptorPool = nullptr;
	whiteTexture = nullptr;
	whiteTextureView = nullptr;
	whiteTextureDescriptor = nullptr;

	temp_engine = nullptr;
}

GUIRenderer::~GUIRenderer ()
{

}

// -- Temp globals -- //
struct nk_context ctx;
struct nk_color background;

void GUIRenderer::writeTestGUI ()
{
	nk_input_begin(&ctx);
	int cursorX = (int) temp_engine->mainWindow->getCursorX(), cursorY = (int) temp_engine->mainWindow->getCursorY();
	nk_input_motion(&ctx, cursorX, cursorY);
	nk_input_button(&ctx, NK_BUTTON_LEFT, cursorX, cursorY, temp_engine->mainWindow->isMouseButtonPressed(0));
	nk_input_button(&ctx, NK_BUTTON_MIDDLE, cursorX, cursorY, temp_engine->mainWindow->isMouseButtonPressed(1));
	nk_input_button(&ctx, NK_BUTTON_RIGHT, cursorX, cursorY, temp_engine->mainWindow->isMouseButtonPressed(2));
	//
	nk_input_end(&ctx);

	float windowHeight = std::max((float) temp_engine->mainWindow->getHeight(), 1.0f);

	nk_window_set_bounds(&ctx, "Hello!", nk_rect(0, 0, 250.0f, windowHeight));

	if (nk_begin(&ctx, "Hello!", nk_rect(0, 0, 250, 250), NK_WINDOW_BORDER | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE))
	{
		enum
		{
			EASY,
			HARD
		};
		static int op = EASY;
		static int property = 20;
		nk_layout_row_static(&ctx, 30, 80, 1);
		if (nk_button_label(&ctx, "button"))
			fprintf(stdout, "button pressed\n");

		nk_layout_row_dynamic(&ctx, 30, 2);
		if (nk_option_label(&ctx, "easy", op == EASY))
			op = EASY;
		if (nk_option_label(&ctx, "hard", op == HARD))
			op = HARD;

		nk_layout_row_dynamic(&ctx, 25, 1);
		nk_property_int(&ctx, "Compression:", 0, &property, 100, 10, 1);
	}
	nk_end(&ctx);
}

/*
 * A draw command plus some extra data for this engine to batch/order the
 * draw commands in an efficient manner.
 */
struct nk_draw_command_extended
{
		struct nk_draw_command nkCmd; // The original nk_draw_command
		uint32_t indexOffset;  // The offset to the index buffer given
		uint32_t cmdIndex;     // The index of when the command was given
};

const struct nk_draw_vertex_layout_element vertex_layout[] = {{NK_VERTEX_POSITION, NK_FORMAT_FLOAT, offsetof(nk_vertex, vertex)}, {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, offsetof(nk_vertex, uv)}, {NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, offsetof(nk_vertex, color)}, {NK_VERTEX_LAYOUT_END}};

void GUIRenderer::recordGUIRenderCommandList (CommandBuffer cmdBuffer, Framebuffer framebuffer, uint32_t renderWidth, uint32_t renderHeight)
{
	nk_convert_config cfg = {};
	cfg.shape_AA = NK_ANTI_ALIASING_ON;
	cfg.line_AA = NK_ANTI_ALIASING_ON;
	cfg.vertex_layout = vertex_layout;
	cfg.vertex_size = sizeof(nk_vertex);
	cfg.vertex_alignment = NK_ALIGNOF(nk_vertex);
	cfg.circle_segment_count = 22;
	cfg.curve_segment_count = 22;
	cfg.arc_segment_count = 22;
	cfg.global_alpha = 1.0f;
	cfg.null.texture.ptr = nullptr;

	nk_buffer cmds, verts, indices;
	nk_buffer_init_default(&cmds);
	nk_buffer_init_default(&verts);
	nk_buffer_init_default(&indices);

	nk_convert(&ctx, &cmds, &verts, &indices, &cfg);

	renderer->mapBuffer(guiVertexStreamBuffer, verts.allocated, verts.memory.ptr);
	renderer->mapBuffer(guiIndexStreamBuffer, indices.allocated, indices.memory.ptr);

	std::vector<ClearValue> clearValues = std::vector<ClearValue>(2);
	clearValues[0].color =
	{	0, 0.4f, 1.0f, 1};
	clearValues[1].depthStencil =
	{	1, 0};

	cmdBuffer->beginDebugRegion("Render GUI", glm::vec4(0.929f, 0.22f, 1.0f, 1.0f));
	cmdBuffer->beginRenderPass(guiRenderPass, framebuffer, {0, 0, renderWidth, renderHeight}, clearValues, SUBPASS_CONTENTS_INLINE);
	cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, guiGraphicsPipeline);

	cmdBuffer->setViewports(0, {{0, 0, (float) renderWidth, (float) renderHeight}});

	glm::mat4 orthoProj = glm::ortho<float>(0, renderWidth, 0, renderHeight);

	cmdBuffer->pushConstants(guiGraphicsPipelineInputLayout, SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &orthoProj[0][0]);

	cmdBuffer->bindIndexBuffer(guiIndexStreamBuffer, 0, false);
	cmdBuffer->bindVertexBuffers(0, {guiVertexStreamBuffer}, {0});

	// The list of draw commands, immediately sorted by texture
	std::map<void*, std::vector<nk_draw_command_extended> > drawCommands;

	uint32_t offset = 0;
	uint32_t cmdIndex = 0;

	const nk_draw_command *cmd;
	nk_draw_foreach(cmd, &ctx, &cmds)
	{
		if (!cmd->elem_count)
			continue;

		nk_draw_command_extended cmd_extended;
		cmd_extended.nkCmd = *cmd;
		cmd_extended.indexOffset = offset;
		cmd_extended.cmdIndex = cmdIndex;

		drawCommands[cmd->texture.ptr].push_back(cmd_extended);

		offset += cmd->elem_count;
		cmdIndex ++;
	}

	uint32_t drawCommandCount = cmdIndex;

	DescriptorSet *currentDrawCommandTexture = nullptr;
	cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, guiGraphicsPipelineInputLayout, 0, {whiteTextureDescriptor});

	for (auto drawCommandIt = drawCommands.begin(); drawCommandIt != drawCommands.end(); drawCommandIt ++)
	{
		for (size_t i = 0; i < drawCommandIt->second.size(); i ++)
		{
			cmd = &drawCommandIt->second[i].nkCmd;
			offset = drawCommandIt->second[i].indexOffset;
			cmdIndex = drawCommandIt->second[i].cmdIndex;

			//printf("Cmd - %p %u %u\n", cmd->texture.ptr, offset, cmd->elem_count);

			Scissor cmdScissor;
			cmdScissor.x = (int32_t) glm::clamp(cmd->clip_rect.x, 0.0f, (float) renderWidth);
			cmdScissor.y = (int32_t) glm::clamp(cmd->clip_rect.y, 0.0f, (float) renderHeight);
			cmdScissor.width = (uint32_t) glm::clamp(cmd->clip_rect.w, 0.0f, (float) renderWidth);
			cmdScissor.height = (uint32_t) glm::clamp(cmd->clip_rect.h, 0.0f, (float) renderHeight);

			float depth = 1.0f - (cmdIndex / float(drawCommandCount));

			if (currentDrawCommandTexture != cmd->texture.ptr)
			{
				if (cmd->texture.ptr == nullptr)
				{
					cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, guiGraphicsPipelineInputLayout, 0, {whiteTextureDescriptor});
				}
				else
				{
					cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, guiGraphicsPipelineInputLayout, 0, {static_cast<DescriptorSet> (cmd->texture.ptr)});
				}

				currentDrawCommandTexture = static_cast<DescriptorSet*> (cmd->texture.ptr);
			}

			cmdBuffer->setScissors(0, {cmdScissor});
			cmdBuffer->pushConstants( guiGraphicsPipelineInputLayout, SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(float), &depth);

			cmdBuffer->drawIndexed(cmd->elem_count, 1, offset, 0, 0);
		}
	}

	cmdBuffer->endRenderPass();
	cmdBuffer->endDebugRegion();

	nk_buffer_free(&cmds);
	nk_buffer_free(&verts);
	nk_buffer_free(&indices);

	nk_clear(&ctx);
}

#include <GLFW/glfw3.h>

void GUIRenderer::init ()
{
	testGUICommandPool = renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, COMMAND_POOL_TRANSIENT_BIT);

	testSampler = renderer->createSampler();
	guiTextureDescriptorPool = renderer->createDescriptorPool({{0, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT}}, 8);

	nk_init_default(&ctx, NULL);
	//clipboard stuff

	nk_font_atlas atlas;

	nk_font_atlas_init_default(&atlas);
	nk_font_atlas_begin(&atlas);

    struct nk_font_config cfg = nk_font_config(512);
    cfg.oversample_h = 4;
    cfg.oversample_v = 4;
	nk_font *font_clean = nk_font_atlas_add_from_file(&atlas, std::string(temp_engine->getWorkingDir() + "GameData/fonts/Cousine-Regular.ttf").c_str(), 18, &cfg);

	int atlasWidth, atlasHeight;
	const void *imageData = nk_font_atlas_bake(&atlas, &atlasWidth, &atlasHeight, NK_FONT_ATLAS_RGBA32);

	{
		fontAtlas = renderer->createTexture({(float) atlasWidth, (float) atlasHeight, 1.0f}, RESOURCE_FORMAT_R8G8B8A8_UNORM, TEXTURE_USAGE_TRANSFER_DST_BIT | TEXTURE_USAGE_SAMPLED_BIT, MEMORY_USAGE_GPU_ONLY, false);
		fontAtlasView = renderer->createTextureView(fontAtlas);

		StagingBuffer stagingBuffer = renderer->createAndMapStagingBuffer(atlasWidth * atlasHeight * sizeof(uint8_t) * 4, imageData);

		CommandBuffer fontAtlasUploadCommandBuffer = renderer->beginSingleTimeCommand(testGUICommandPool);

		fontAtlasUploadCommandBuffer->transitionTextureLayout(fontAtlas, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL);
		fontAtlasUploadCommandBuffer->stageBuffer(stagingBuffer, fontAtlas);
		fontAtlasUploadCommandBuffer->transitionTextureLayout(fontAtlas, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		renderer->endSingleTimeCommand(fontAtlasUploadCommandBuffer, testGUICommandPool, QUEUE_TYPE_GRAPHICS);

		renderer->destroyStagingBuffer(stagingBuffer);

		fontAtlasDescriptor = guiTextureDescriptorPool->allocateDescriptorSet();

		RendererDescriptorWriteInfo write0 = {};
		write0.dstSet = fontAtlasDescriptor;
		write0.descriptorCount = 1;
		write0.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write0.imageInfo =
		{
			{	testSampler, fontAtlasView, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};

		renderer->writeDescriptorSets({write0});
	}

	{
		whiteTexture = temp_engine->resources->loadTextureImmediate(temp_engine->getWorkingDir() + "GameData/textures/test-png.png");
		whiteTextureView = renderer->createTextureView(whiteTexture->texture);

		whiteTextureDescriptor = guiTextureDescriptorPool->allocateDescriptorSet();

		RendererDescriptorWriteInfo write0 = {};
		write0.dstSet = whiteTextureDescriptor;
		write0.descriptorCount = 1;
		write0.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write0.imageInfo =
		{
			{	testSampler, whiteTextureView, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};

		renderer->writeDescriptorSets({write0});
	}

	{
	}

	nk_font_atlas_end(&atlas, nk_handle_ptr(fontAtlasDescriptor), nullptr);

	nk_style_set_font(&ctx, &font_clean->handle);

	createRenderPass();
	createGraphicsPipeline();

	guiVertexStreamBuffer = renderer->createBuffer(512 * 1024, BUFFER_USAGE_VERTEX_BUFFER_BIT, MEMORY_USAGE_CPU_TO_GPU, false);
	guiIndexStreamBuffer = renderer->createBuffer(512 * 1024, BUFFER_USAGE_INDEX_BUFFER_BIT, MEMORY_USAGE_CPU_TO_GPU, false);
}

void GUIRenderer::destroy ()
{
	renderer->destroyBuffer(guiVertexStreamBuffer);
	renderer->destroyBuffer(guiIndexStreamBuffer);

	temp_engine->resources->returnTexture(whiteTexture);
	renderer->destroyTextureView(whiteTextureView);
	//guiTextureDescriptorPool->freeDescriptorSet(whiteTextureDescriptor);

	renderer->destroyTexture(fontAtlas);
	renderer->destroyTextureView(fontAtlasView);
	//guiTextureDescriptorPool->freeDescriptorSet(fontAtlasDescriptor);

	renderer->destroyCommandPool(testGUICommandPool);
	renderer->destroyDescriptorPool(guiTextureDescriptorPool);

	renderer->destroySampler(testSampler);

	renderer->destroyPipelineInputLayout(guiGraphicsPipelineInputLayout);
	renderer->destroyPipeline(guiGraphicsPipeline);
	renderer->destroyRenderPass(guiRenderPass);

	nk_free(&ctx);
}

void GUIRenderer::createRenderPass ()
{
	AttachmentDescription colorAttachment0 = {}, depthAttachment = {};
	colorAttachment0.format = RESOURCE_FORMAT_R8G8B8A8_UNORM;
	colorAttachment0.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
	colorAttachment0.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colorAttachment0.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment0.storeOp = ATTACHMENT_STORE_OP_STORE;

	depthAttachment.format = RESOURCE_FORMAT_D32_SFLOAT;
	depthAttachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	depthAttachment.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = ATTACHMENT_STORE_OP_STORE;

	AttachmentReference colorAttachment0Ref = {}, depthAttachmentRef;
	colorAttachment0Ref.attachment = 0;
	colorAttachment0Ref.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	SubpassDescription subpass0 = {};
	subpass0.bindPoint = PIPELINE_BIND_POINT_GRAPHICS;
	subpass0.colorAttachments =
	{	colorAttachment0Ref};
	subpass0.depthStencilAttachment = &depthAttachmentRef;

	guiRenderPass = renderer->createRenderPass({colorAttachment0, depthAttachment}, {subpass0}, {});
}

void GUIRenderer::createGraphicsPipeline ()
{
	ShaderModule vertShader = renderer->createShaderModule(temp_engine->getWorkingDir() + "GameData/shaders/vulkan/nuklear-gui.glsl", SHADER_STAGE_VERTEX_BIT);
	ShaderModule fragShader = renderer->createShaderModule(temp_engine->getWorkingDir() + "GameData/shaders/vulkan/nuklear-gui.glsl", SHADER_STAGE_FRAGMENT_BIT);

	VertexInputBinding bindingDesc;
	bindingDesc.binding = 0;
	bindingDesc.stride = static_cast<uint32_t>(sizeof(nk_vertex));
	bindingDesc.inputRate = VERTEX_INPUT_RATE_VERTEX;

	PipelineShaderStage vertShaderStage = {};
	vertShaderStage.entry = "main";
	vertShaderStage.module = vertShader;

	PipelineShaderStage fragShaderStage = {};
	fragShaderStage.entry = "main";
	fragShaderStage.module = fragShader;

	std::vector<VertexInputAttrib> attribDesc = std::vector<VertexInputAttrib>(3);
	attribDesc[0].binding = 0;
	attribDesc[0].location = 0;
	attribDesc[0].format = RESOURCE_FORMAT_R32G32_SFLOAT;
	attribDesc[0].offset = static_cast<uint32_t>(offsetof(nk_vertex, vertex));

	attribDesc[1].binding = 0;
	attribDesc[1].location = 1;
	attribDesc[1].format = RESOURCE_FORMAT_R32G32_SFLOAT;
	attribDesc[1].offset = static_cast<uint32_t>(offsetof(nk_vertex, uv));

	attribDesc[2].binding = 0;
	attribDesc[2].location = 2;
	attribDesc[2].format = RESOURCE_FORMAT_R32G32B32A32_SFLOAT;
	attribDesc[2].offset = static_cast<uint32_t>(offsetof(nk_vertex, color));

	PipelineVertexInputInfo vertexInput = {};
	vertexInput.vertexInputAttribs = attribDesc;
	vertexInput.vertexInputBindings =
	{	bindingDesc};

	PipelineInputAssemblyInfo inputAssembly = {};
	inputAssembly.primitiveRestart = false;
	inputAssembly.topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	PipelineViewportInfo viewportInfo = {};
	viewportInfo.scissors =
	{
		{	0, 0, 1920, 1080}};
	viewportInfo.viewports =
	{
		{	0, 0, 1920, 1080}};

	PipelineRasterizationInfo rastInfo = {};
	rastInfo.clockwiseFrontFace = true;
	rastInfo.cullMode = CULL_MODE_BACK_BIT;
	rastInfo.lineWidth = 1;
	rastInfo.polygonMode = POLYGON_MODE_FILL;
	rastInfo.rasterizerDiscardEnable = false;

	PipelineDepthStencilInfo depthInfo = {};
	depthInfo.enableDepthTest = true;
	depthInfo.enableDepthWrite = true;
	depthInfo.minDepthBounds = 0;
	depthInfo.maxDepthBounds = 1;
	depthInfo.depthCompareOp = COMPARE_OP_LESS_OR_EQUAL;

	PipelineColorBlendAttachment colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = true;
	colorBlendAttachment.colorWriteMask = COLOR_COMPONENT_R_BIT | COLOR_COMPONENT_G_BIT | COLOR_COMPONENT_B_BIT | COLOR_COMPONENT_A_BIT;

	colorBlendAttachment.colorBlendOp = BLEND_OP_ADD;
	colorBlendAttachment.alphaBlendOp = BLEND_OP_ADD;

	colorBlendAttachment.srcAlphaBlendFactor = BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = BLEND_FACTOR_ZERO;

	colorBlendAttachment.srcColorBlendFactor = BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

	PipelineColorBlendInfo colorBlend = {};
	colorBlend.attachments =
	{	colorBlendAttachment};
	colorBlend.logicOpEnable = false;
	colorBlend.logicOp = LOGIC_OP_COPY;
	colorBlend.blendConstants[0] = 1.0f;
	colorBlend.blendConstants[1] = 1.0f;
	colorBlend.blendConstants[2] = 1.0f;
	colorBlend.blendConstants[3] = 1.0f;

	PipelineDynamicStateInfo dynamicState = {};
	dynamicState.dynamicStates =
	{	DYNAMIC_STATE_VIEWPORT, DYNAMIC_STATE_SCISSOR};

	PipelineInfo info = {};
	info.stages =
	{	vertShaderStage, fragShaderStage};
	info.vertexInputInfo = vertexInput;
	info.inputAssemblyInfo = inputAssembly;
	info.viewportInfo = viewportInfo;
	info.rasterizationInfo = rastInfo;
	info.depthStencilInfo = depthInfo;
	info.colorBlendInfo = colorBlend;
	info.dynamicStateInfo = dynamicState;

	guiGraphicsPipelineInputLayout = renderer->createPipelineInputLayout({{	0, sizeof(glm::mat4) + sizeof(float), SHADER_STAGE_VERTEX_BIT}}, {{{0, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT}}});
	guiGraphicsPipeline = renderer->createGraphicsPipeline(info, guiGraphicsPipelineInputLayout, guiRenderPass, 0);

	renderer->destroyShaderModule(vertShader);
	renderer->destroyShaderModule(fragShader);
}
