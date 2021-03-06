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

#include "Rendering/UI/GUIRenderer.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
 //#define NK_INCLUDE_DEFAULT_FONT
 //#define NK_BUTTON_TRIGGER_ON_RELEASE

#include <nuklear.h>

#include <Rendering/Renderer/Renderer.h>
#include <Engine/StarlightEngine.h>
#include <Resources/ResourceManager.h>

#include <Input/Window.h>

GUIRenderer::GUIRenderer(Renderer *engineRenderer)
{
	renderer = engineRenderer;

	guiGraphicsPipeline = nullptr;

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

GUIRenderer::~GUIRenderer()
{

}

void GUIRenderer::writeTestGUI(struct nk_context &ctx)
{
	float windowWidth = std::max((float) temp_engine->mainWindow->getWidth(), 1.0f);
	float windowHeight = std::max((float) temp_engine->mainWindow->getHeight(), 1.0f);

	struct nk_color table[NK_COLOR_COUNT];

	table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
	table[NK_COLOR_WINDOW] = nk_rgba(57, 67, 71, 0);
	table[NK_COLOR_HEADER] = nk_rgba(51, 51, 56, 220);
	table[NK_COLOR_BORDER] = nk_rgba(46, 46, 46, 255);
	table[NK_COLOR_BUTTON] = nk_rgba(48, 83, 111, 255);
	table[NK_COLOR_BUTTON_HOVER] = nk_rgba(58, 93, 121, 255);
	table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(63, 98, 126, 255);
	table[NK_COLOR_TOGGLE] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 56, 255);
	table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(48, 83, 111, 255);
	table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
	table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
	table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, 245);
	table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
	table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
	table[NK_COLOR_PROPERTY] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_EDIT] = nk_rgba(50, 58, 61, 225);
	table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
	table[NK_COLOR_COMBO] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_CHART] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
	table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
	table[NK_COLOR_SCROLLBAR] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(48, 83, 111, 255);
	table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
	table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
	table[NK_COLOR_TAB_HEADER] = nk_rgba(48, 83, 111, 255);
	nk_style_from_table(&ctx, table);

	if (nk_begin(&ctx, "Hello!", nk_rect(0, 0, 250.0f, windowHeight), NK_WINDOW_BACKGROUND | NK_WINDOW_NO_SCROLLBAR))
	{
		enum
		{
			EASY, HARD
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
	nk_style_default(&ctx);

	if (nk_begin(&ctx, "Hello!other", nk_rect(windowWidth - 250, 250, 250, windowHeight), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		enum
		{
			EASY, HARD
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

const struct nk_draw_vertex_layout_element vertex_layout[] = {{
		NK_VERTEX_POSITION, NK_FORMAT_FLOAT, offsetof(nk_vertex, vertex) }, {
		NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, offsetof(nk_vertex, uv) }, {
		NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, offsetof(nk_vertex,
				color) }, { NK_VERTEX_LAYOUT_END }};

void GUIRenderer::recordGUIRenderCommandList(CommandBuffer cmdBuffer, struct nk_context &ctx, uint32_t renderWidth, uint32_t renderHeight)
{
	nk_convert_config cfg = { };
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

	void *data = renderer->mapBuffer(guiVertexStreamBuffer);
	memcpy(data, verts.memory.ptr, verts.allocated);
	renderer->unmapBuffer(guiVertexStreamBuffer);

	data = renderer->mapBuffer(guiIndexStreamBuffer);
	memcpy(data, indices.memory.ptr, indices.allocated);
	renderer->unmapBuffer(guiIndexStreamBuffer);

	cmdBuffer->bindPipeline(PIPELINE_BIND_POINT_GRAPHICS, guiGraphicsPipeline);

	glm::mat4 orthoProj = glm::ortho<float>(0, renderWidth, 0, renderHeight);

	cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &orthoProj[0][0]);

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
		cmdIndex++;
	}

	uint32_t drawCommandCount = cmdIndex;

	DescriptorSet *currentDrawCommandTexture = nullptr;
	cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS, 0, {whiteTextureDescriptor});

	for (auto drawCommandIt = drawCommands.begin();
		drawCommandIt != drawCommands.end(); drawCommandIt++)
	{
		for (size_t i = 0; i < drawCommandIt->second.size(); i++)
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

			float depth = 1.0f - ((cmdIndex + 1) / float(drawCommandCount));

			if (currentDrawCommandTexture != cmd->texture.ptr)
			{
				if (cmd->texture.ptr == nullptr)
				{
					cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS,
						0, {whiteTextureDescriptor});
				}
				else
				{
					cmdBuffer->bindDescriptorSets(PIPELINE_BIND_POINT_GRAPHICS,
						0,
						{static_cast<DescriptorSet>(cmd->texture.ptr)});
				}

				currentDrawCommandTexture =
					static_cast<DescriptorSet*>(cmd->texture.ptr);
			}

			cmdBuffer->setScissors(0, {cmdScissor});
			cmdBuffer->pushConstants(SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4),
				sizeof(float), &depth);

			cmdBuffer->drawIndexed(cmd->elem_count, 1, offset, 0, 0);
		}
	}

	nk_buffer_free(&cmds);
	nk_buffer_free(&verts);
	nk_buffer_free(&indices);
}

void GUIRenderer::init(struct nk_context &ctx)
{
	testGUICommandPool = renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, COMMAND_POOL_TRANSIENT_BIT);

	testSampler = renderer->createSampler();
	guiTextureDescriptorPool = renderer->createDescriptorPool({{ 0, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT }}, 8);

	//clipboard stuff

	nk_font_atlas atlas;

	nk_font_atlas_init_default(&atlas);
	nk_font_atlas_begin(&atlas);

	struct nk_font_config cfg = nk_font_config(512);
	cfg.oversample_h = 4;
	cfg.oversample_v = 4;

	std::vector<char> fontData = FileLoader::instance()->readFileBuffer("GameData/fonts/Cousine-Regular.ttf");
	nk_font *font_clean = nk_font_atlas_add_from_memory(&atlas, fontData.data(), fontData.size(), 18, &cfg);
	
	int atlasWidth, atlasHeight;
	const void *imageData = nk_font_atlas_bake(&atlas, &atlasWidth, &atlasHeight, NK_FONT_ATLAS_RGBA32);

	{
		fontAtlas = renderer->createTexture({(uint32_t) atlasWidth, (uint32_t) atlasHeight, 1}, RESOURCE_FORMAT_R8G8B8A8_UNORM, TEXTURE_USAGE_TRANSFER_DST_BIT | TEXTURE_USAGE_SAMPLED_BIT, MEMORY_USAGE_GPU_ONLY, false);
		fontAtlasView = renderer->createTextureView(fontAtlas);

		StagingBuffer stagingBuffer = renderer->createAndFillStagingBuffer(
			atlasWidth * atlasHeight * sizeof(uint8_t) * 4, imageData);

		CommandBuffer fontAtlasUploadCommandBuffer =
			renderer->beginSingleTimeCommand(testGUICommandPool);

		fontAtlasUploadCommandBuffer->transitionTextureLayout(fontAtlas, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL);
		fontAtlasUploadCommandBuffer->stageBuffer(stagingBuffer, fontAtlas);
		fontAtlasUploadCommandBuffer->transitionTextureLayout(fontAtlas, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		renderer->endSingleTimeCommand(fontAtlasUploadCommandBuffer, testGUICommandPool, QUEUE_TYPE_GRAPHICS);

		renderer->destroyStagingBuffer(stagingBuffer);

		fontAtlasDescriptor = guiTextureDescriptorPool->allocateDescriptorSet();

		RendererDescriptorWriteInfo write0 = { };
		write0.dstSet = fontAtlasDescriptor;
		write0.descriptorCount = 1;
		write0.descriptorType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write0.imageInfo = {{testSampler, fontAtlasView, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};

		renderer->writeDescriptorSets({write0});
	}

	{
		whiteTexture = temp_engine->resources->loadTextureImmediate("GameData/textures/test-png.png");
		whiteTextureView = renderer->createTextureView(whiteTexture->texture);

		whiteTextureDescriptor =
			guiTextureDescriptorPool->allocateDescriptorSet();

		RendererDescriptorWriteInfo write0 = { };
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

	createGraphicsPipeline();

	guiVertexStreamBuffer = renderer->createBuffer(512 * 1024, BUFFER_USAGE_VERTEX_BUFFER, false, false, MEMORY_USAGE_CPU_TO_GPU, false);
	guiIndexStreamBuffer = renderer->createBuffer(512 * 1024, BUFFER_USAGE_INDEX_BUFFER, false, false, MEMORY_USAGE_CPU_TO_GPU, false);
}

void GUIRenderer::destroy()
{
	renderer->destroyBuffer(guiVertexStreamBuffer);
	renderer->destroyBuffer(guiIndexStreamBuffer);

	temp_engine->resources->returnTexture(whiteTexture);
	renderer->destroyTextureView(whiteTextureView);

	renderer->destroyTexture(fontAtlas);
	renderer->destroyTextureView(fontAtlasView);

	renderer->destroyCommandPool(testGUICommandPool);
	renderer->destroyDescriptorPool(guiTextureDescriptorPool);

	renderer->destroySampler(testSampler);

	renderer->destroyPipeline(guiGraphicsPipeline);
}

void GUIRenderer::createGraphicsPipeline()
{
	ShaderModule vertShader = renderer->createShaderModule("GameData/shaders/vulkan/nuklear-gui.glsl", SHADER_STAGE_VERTEX_BIT, SHADER_LANGUAGE_GLSL);
	ShaderModule fragShader = renderer->createShaderModule("GameData/shaders/vulkan/nuklear-gui.glsl", SHADER_STAGE_FRAGMENT_BIT, SHADER_LANGUAGE_GLSL);

	VertexInputBinding bindingDesc;
	bindingDesc.binding = 0;
	bindingDesc.stride = static_cast<uint32_t>(sizeof(nk_vertex));
	bindingDesc.inputRate = VERTEX_INPUT_RATE_VERTEX;

	PipelineShaderStage vertShaderStage = { };
	vertShaderStage.entry = "main";
	vertShaderStage.module = vertShader;

	PipelineShaderStage fragShaderStage = { };
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

	PipelineVertexInputInfo vertexInput = { };
	vertexInput.vertexInputAttribs = attribDesc;
	vertexInput.vertexInputBindings = {bindingDesc};

	PipelineInputAssemblyInfo inputAssembly = { };
	inputAssembly.primitiveRestart = false;
	inputAssembly.topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	PipelineViewportInfo viewportInfo = { };
	viewportInfo.scissors = {{0, 0, 1920, 1080}};
	viewportInfo.viewports = {{0, 0, 1920, 1080}};

	PipelineRasterizationInfo rastInfo = { };
	rastInfo.clockwiseFrontFace = true;
	rastInfo.cullMode = CULL_MODE_BACK_BIT;
	rastInfo.lineWidth = 1;
	rastInfo.polygonMode = POLYGON_MODE_FILL;
	rastInfo.rasterizerDiscardEnable = false;
	rastInfo.enableOutOfOrderRasterization = false;

	PipelineDepthStencilInfo depthInfo = { };
	depthInfo.enableDepthTest = true;
	depthInfo.enableDepthWrite = true;
	depthInfo.minDepthBounds = 0;
	depthInfo.maxDepthBounds = 1;
	depthInfo.depthCompareOp = COMPARE_OP_LESS_OR_EQUAL;

	PipelineColorBlendAttachment colorBlendAttachment = { };
	colorBlendAttachment.blendEnable = true;
	colorBlendAttachment.colorWriteMask = COLOR_COMPONENT_R_BIT | COLOR_COMPONENT_G_BIT | COLOR_COMPONENT_B_BIT | COLOR_COMPONENT_A_BIT;

	colorBlendAttachment.colorBlendOp = BLEND_OP_ADD;
	colorBlendAttachment.alphaBlendOp = BLEND_OP_ADD;

	colorBlendAttachment.srcAlphaBlendFactor = BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = BLEND_FACTOR_ZERO;

	colorBlendAttachment.srcColorBlendFactor = BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

	PipelineColorBlendInfo colorBlend = { };
	colorBlend.attachments =
	{colorBlendAttachment};
	colorBlend.logicOpEnable = false;
	colorBlend.logicOp = LOGIC_OP_COPY;
	colorBlend.blendConstants[0] = 1.0f;
	colorBlend.blendConstants[1] = 1.0f;
	colorBlend.blendConstants[2] = 1.0f;
	colorBlend.blendConstants[3] = 1.0f;

	PipelineDynamicStateInfo dynamicState = { };
	dynamicState.dynamicStates = {DYNAMIC_STATE_VIEWPORT, DYNAMIC_STATE_SCISSOR};

	GraphicsPipelineInfo info = { };
	info.stages =
	{vertShaderStage, fragShaderStage};
	info.vertexInputInfo = vertexInput;
	info.inputAssemblyInfo = inputAssembly;
	info.viewportInfo = viewportInfo;
	info.rasterizationInfo = rastInfo;
	info.depthStencilInfo = depthInfo;
	info.colorBlendInfo = colorBlend;
	info.dynamicStateInfo = dynamicState;

	info.inputPushConstantRanges = {{0, sizeof(glm::mat4) + sizeof(float), SHADER_STAGE_VERTEX_BIT}};
	info.inputSetLayouts = {{ {0, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT}}};

	guiGraphicsPipeline = renderer->createGraphicsPipeline(info, temp_engine->guiRenderPass, 0);

	renderer->destroyShaderModule(vertShader);
	renderer->destroyShaderModule(fragShader);
}
