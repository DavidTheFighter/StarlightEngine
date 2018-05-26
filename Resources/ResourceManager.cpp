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
 * ResourceHandler.cpp
 * 
 * Created on: Sep 27, 2017
 *     Author: david
 */

#include "Resources/ResourceManager.h"

#include <Rendering/Renderer/Renderer.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>

ResourceManager::ResourceManager (Renderer *rendererInstance, const std::string &gameWorkingDirectory)
{
	renderer = rendererInstance;
	workingDir = gameWorkingDirectory;
	mainThreadTransferCommandPool = renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, COMMAND_POOL_TRANSIENT_BIT);

	std::vector<DescriptorSetLayoutBinding> layoutBindings;
	layoutBindings.push_back({0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT});
	layoutBindings.push_back({1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT});

	mainThreadDescriptorPool = renderer->createDescriptorPool(layoutBindings, 16);

	pipelineRenderPass = nullptr;
	pipelineShadowRenderPass = nullptr;
}

ResourceManager::~ResourceManager ()
{
	renderer->destroyCommandPool(mainThreadTransferCommandPool);
	renderer->destroyDescriptorPool(mainThreadDescriptorPool);
	// TODO Free all remaining resources when an instance of ResourceManager is deleted
	
	printf("Remaining resources - %u, %u, %u\n", loadedMaterials.size(), loadedTextures.size(), loadedStaticMeshes.size());
}

void ResourceManager::loadGameDefsFile (const std::string &file)
{

}

ResourceMaterial ResourceManager::loadMaterialImmediate (const std::string &defUniqueName)
{
	auto it = loadedMaterials.find(stringHash(defUniqueName));

	if (it == loadedMaterials.end())
	{
		MaterialDef *matDef = getMaterialDef(defUniqueName);

		ResourceMaterialObject *mat = new ResourceMaterialObject();
		mat->defUniqueName = defUniqueName;
		mat->descriptorSet = mainThreadDescriptorPool->allocateDescriptorSet();
		mat->sampler = renderer->createSampler(matDef->addressMode, matDef->linearFiltering ? SAMPLER_FILTER_LINEAR : SAMPLER_FILTER_NEAREST, matDef->linearFiltering ? SAMPLER_FILTER_LINEAR : SAMPLER_FILTER_NEAREST, 4, {0, 14, 0},
				matDef->linearMipmapFiltering ? SAMPLER_MIPMAP_MODE_LINEAR : SAMPLER_MIPMAP_MODE_NEAREST);
		mat->pipelineHash = stringHash(matDef->pipelineUniqueName);
		renderer->setObjectDebugName(mat->sampler, OBJECT_TYPE_SAMPLER, "Material: " + mat->defUniqueName + " sampler");

		std::vector<std::string> texFiles;

		for (int i = 0; i < MATERIAL_DEF_MAX_TEXTURE_NUM; i ++)
			if (std::string(matDef->textureFiles[i]).length() != 0)
				texFiles.push_back(workingDir + std::string(matDef->textureFiles[i]));

		mat->textures = loadTextureArrayImmediate(texFiles);

		std::vector<DescriptorWriteInfo> writes(2);
		writes[0].descriptorCount = 1;
		writes[0].descriptorType = DESCRIPTOR_TYPE_SAMPLER;
		writes[0].dstSet = mat->descriptorSet;
		writes[0].imageInfo =
		{
			{	mat->sampler, nullptr, TEXTURE_LAYOUT_UNDEFINED}};
		writes[0].dstBinding = 0;
		writes[0].dstArrayElement = 0;

		writes[1].descriptorCount = 1;
		writes[1].descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[1].dstSet = mat->descriptorSet;
		writes[1].imageInfo = {{mat->sampler, mat->textures->textureView, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
		writes[1].dstBinding = 1;
		writes[1].dstArrayElement = 0;

		renderer->writeDescriptorSets(writes);

		loadedMaterials[stringHash(defUniqueName)] = std::make_pair(mat, 1);

		return mat;
	}
	else
	{
		it->second.second ++;

		return it->second.first;
	}
}

ResourceMaterial ResourceManager::findMaterial (const std::string &defUniqueName)
{
	return findMaterial(stringHash(defUniqueName));
}

ResourceMaterial ResourceManager::findMaterial (size_t defUniqueNameHash)
{
	auto it = loadedMaterials.find(defUniqueNameHash);

	return it != loadedMaterials.end() ? it->second.first : nullptr;
}

void ResourceManager::returnMaterial (const std::string &defUniqueName)
{
	return returnMaterial(stringHash(defUniqueName));
}

void ResourceManager::returnMaterial (size_t defUniqueNameHash)
{
	auto it = loadedMaterials.find(defUniqueNameHash);

	if (it != loadedMaterials.end())
	{
		// Decrement the reference counter
		it->second.second --;

		// If the reference counter is now zero, then delete the mesh data entirely
		if (it->second.second == 0)
		{
			ResourceMaterial mat = it->second.first;

			mainThreadDescriptorPool->freeDescriptorSet(mat->descriptorSet);
			renderer->destroySampler(mat->sampler);

			returnTexture(mat->textures);

			delete mat;

			loadedMaterials.erase(it);
		}
	}
}

bool ResourceMananger_meshLodDistComp (const std::pair<float, ResourceMesh> &arg0, const std::pair<float, ResourceMesh> &arg1)
{
	return arg0.first < arg1.first;
}

ResourceStaticMesh ResourceManager::loadStaticMeshImmediate (const std::string &defUniqueName)
{
	auto it = loadedStaticMeshes.find(stringHash(defUniqueName));

	if (it == loadedStaticMeshes.end())
	{
		StaticMeshDef *matDef = getMeshDef(defUniqueName);

		ResourceStaticMeshObject *mesh = new ResourceStaticMeshObject();
		mesh->defUniqueName = matDef->uniqueName;

		DEBUG_ASSERT(matDef->meshLODFiles.size() == matDef->meshLODNames.size() && matDef->meshLODNames.size() == matDef->meshLODMaxDists.size());

		for (size_t i = 0; i < matDef->meshLODFiles.size(); i ++)
		{
			std::string lodMeshFile = std::string(matDef->meshLODFiles[i]);
			std::string lodMeshName = std::string(matDef->meshLODNames[i]);
			float lodMaxDist = matDef->meshLODMaxDists[i];

			mesh->meshLODs.push_back(std::make_pair(lodMaxDist, loadMeshImmediate(workingDir + lodMeshFile, lodMeshName)));
		}

		// The "meshLODs" member is required to be sorted by lod distance
		std::sort(mesh->meshLODs.begin(), mesh->meshLODs.end(), ResourceMananger_meshLodDistComp);

		//mesh->mesh = loadMeshImmediate(workingDir + std::string(matDef->meshFile), std::string(matDef->meshName));

		loadedStaticMeshes[stringHash(defUniqueName)] = std::make_pair(mesh, 1);

		return mesh;
	}
	else
	{
		it->second.second ++;

		return it->second.first;
	}
}

ResourceStaticMesh ResourceManager::findStaticMesh (const std::string &defUniqueName)
{
	return findStaticMesh(stringHash(defUniqueName));
}

ResourceStaticMesh ResourceManager::findStaticMesh (size_t defUniqueNameHash)
{
	auto it = loadedStaticMeshes.find(defUniqueNameHash);

	return it != loadedStaticMeshes.end() ? it->second.first : nullptr;
}

void ResourceManager::returnStaticMesh (const std::string &defUniqueName)
{
	returnStaticMesh(stringHash(defUniqueName));
}

void ResourceManager::returnStaticMesh (size_t defUniqueNameHash)
{
	auto it = loadedStaticMeshes.find(defUniqueNameHash);

	if (it != loadedStaticMeshes.end())
	{
		// Decrement the reference counter
		it->second.second --;

		// If the reference counter is now zero, then delete the mesh data entirely
		if (it->second.second == 0)
		{
			ResourceStaticMesh mesh = it->second.first;

			for (uint32_t lod = 0; lod < mesh->meshLODs.size(); lod ++)
			{
				returnMesh(mesh->meshLODs[lod].second);
			}

			delete mesh;

			loadedStaticMeshes.erase(it);
		}
	}
}

ResourcePipeline ResourceManager::loadPipelineImmediate (const std::string &defUniqueName)
{
	auto it = loadedPipelines.find(stringHash(defUniqueName));

	if (it == loadedPipelines.end())
	{
		PipelineDef *pipeDef = getPipelineDef(defUniqueName);

		ResourcePipelineObject *pipe = new ResourcePipelineObject();
		pipe->dataLoaded = true;

		ShaderModule vertShader = renderer->createShaderModule(workingDir + std::string(pipeDef->vertexShaderFile), SHADER_STAGE_VERTEX_BIT);
		ShaderModule fragShader = renderer->createShaderModule(workingDir + std::string(pipeDef->fragmentShaderFile), SHADER_STAGE_FRAGMENT_BIT);

		ShaderModule tessCtrlShader = nullptr, tessEvalShader = nullptr, geomShader = nullptr;

		if (strlen(pipeDef->tessControlShaderFile) != 0)
			tessCtrlShader = renderer->createShaderModule(workingDir + std::string(pipeDef->tessControlShaderFile), SHADER_STAGE_TESSELLATION_CONTROL_BIT);

		if (strlen(pipeDef->tessEvalShaderFile) != 0)
			tessEvalShader = renderer->createShaderModule(workingDir + std::string(pipeDef->tessEvalShaderFile), SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

		if (strlen(pipeDef->geometryShaderFile) != 0)
			geomShader = renderer->createShaderModule(workingDir + std::string(pipeDef->geometryShaderFile), SHADER_STAGE_GEOMETRY_BIT);

		// If only one of the tessellation stages is present
		if ((tessCtrlShader != nullptr && tessEvalShader == nullptr) || (tessCtrlShader == nullptr && tessEvalShader != nullptr))
		{
			printf("%s Failed to load pipeline: %s, must have both tessellation control and evaluation shaders, not just one\n", INFO_PREFIX, pipeDef->uniqueName);
			// TODO Handle failed pipeline case more gracefully

			return nullptr;
		}

		const uint32_t ivunt_vertexFormatSize = 44;

		VertexInputBinding meshVertexBindingDesc = {}, instanceVertexBindingDesc = {};
		meshVertexBindingDesc.binding = 0;
		meshVertexBindingDesc.stride = ivunt_vertexFormatSize;
		meshVertexBindingDesc.inputRate = VERTEX_INPUT_RATE_VERTEX;

		instanceVertexBindingDesc.binding = 1;
		instanceVertexBindingDesc.stride = sizeof(svec4) * 2;
		instanceVertexBindingDesc.inputRate = VERTEX_INPUT_RATE_INSTANCE;

		std::vector<VertexInputAttrib> attribDesc = std::vector<VertexInputAttrib>(6);
		attribDesc[0].binding = 0;
		attribDesc[0].location = 0;
		attribDesc[0].format = RESOURCE_FORMAT_R32G32B32_SFLOAT;
		attribDesc[0].offset = 0;

		attribDesc[1].binding = 0;
		attribDesc[1].location = 1;
		attribDesc[1].format = RESOURCE_FORMAT_R32G32_SFLOAT;
		attribDesc[1].offset = sizeof(glm::vec3);

		attribDesc[2].binding = 0;
		attribDesc[2].location = 2;
		attribDesc[2].format = RESOURCE_FORMAT_R32G32B32_SFLOAT;
		attribDesc[2].offset = sizeof(glm::vec3) + sizeof(glm::vec2);

		attribDesc[3].binding = 0;
		attribDesc[3].location = 3;
		attribDesc[3].format = RESOURCE_FORMAT_R32G32B32_SFLOAT;
		attribDesc[3].offset = sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3);

		attribDesc[4].binding = 1;
		attribDesc[4].location = 4;
		attribDesc[4].format = RESOURCE_FORMAT_R32G32B32A32_SFLOAT;
		attribDesc[4].offset = 0;

		attribDesc[5].binding = 1;
		attribDesc[5].location = 5;
		attribDesc[5].format = RESOURCE_FORMAT_R32G32B32A32_SFLOAT;
		attribDesc[5].offset = sizeof(svec4);

		PipelineVertexInputInfo vertexInput = {};
		vertexInput.vertexInputAttribs = attribDesc;
		vertexInput.vertexInputBindings =
		{	meshVertexBindingDesc, instanceVertexBindingDesc};

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
		rastInfo.clockwiseFrontFace = pipeDef->clockwiseFrontFace;
		rastInfo.cullMode = (pipeDef->backfaceCulling ? CULL_MODE_BACK_BIT : 0) | (pipeDef->frontfaceCullilng ? CULL_MODE_FRONT_BIT : 0);
		rastInfo.lineWidth = 1;
		rastInfo.polygonMode = POLYGON_MODE_FILL;
		rastInfo.rasterizerDiscardEnable = false;

		PipelineDepthStencilInfo depthInfo = {};
		depthInfo.enableDepthTest = true;
		depthInfo.enableDepthWrite = true;
		depthInfo.minDepthBounds = 0;
		depthInfo.maxDepthBounds = 1;
		depthInfo.depthCompareOp = COMPARE_OP_GREATER;

		PipelineColorBlendAttachment colorBlendAttachment = {};
		colorBlendAttachment.blendEnable = false;
		colorBlendAttachment.colorWriteMask = COLOR_COMPONENT_R_BIT | COLOR_COMPONENT_G_BIT | COLOR_COMPONENT_B_BIT | COLOR_COMPONENT_A_BIT;

		PipelineColorBlendInfo colorBlend = {};
		colorBlend.attachments =
		{	colorBlendAttachment, colorBlendAttachment};
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

		PipelineShaderStage vertShaderStage = {};
		vertShaderStage.entry = "main";
		vertShaderStage.module = vertShader;

		PipelineShaderStage fragShaderStage = {};
		fragShaderStage.entry = "main";
		fragShaderStage.module = fragShader;

		// Note if you change the index of the frag shader, replace the index in if (pipeDef->canRenderDepth) {..}
		info.stages =
		{	vertShaderStage, fragShaderStage};

		if (tessCtrlShader != nullptr)
		{
			PipelineShaderStage tessCtrlShaderStage = {};
			tessCtrlShaderStage.entry = "main";
			tessCtrlShaderStage.module = tessCtrlShader;

			info.stages.push_back(tessCtrlShaderStage);
		}

		if (tessEvalShader != nullptr)
		{
			PipelineShaderStage tessEvalShaderStage = {};
			tessEvalShaderStage.entry = "main";
			tessEvalShaderStage.module = tessEvalShader;

			info.stages.push_back(tessEvalShaderStage);

			info.tessellationInfo.patchControlPoints = 3;
		}

		if (geomShader != nullptr)
		{
			PipelineShaderStage geomShaderStage = {};
			geomShaderStage.entry = "main";
			geomShaderStage.module = geomShader;

			info.stages.push_back(geomShaderStage);
		}

		info.vertexInputInfo = vertexInput;
		info.inputAssemblyInfo = inputAssembly;
		info.viewportInfo = viewportInfo;
		info.rasterizationInfo = rastInfo;
		info.depthStencilInfo = depthInfo;
		info.colorBlendInfo = colorBlend;
		info.dynamicStateInfo = dynamicState;

		std::vector<DescriptorSetLayoutBinding> layoutBindings;
		layoutBindings.push_back({0, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_FRAGMENT_BIT});
		layoutBindings.push_back({1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT});

		info.inputPushConstantRanges = {{0, sizeof(glm::mat4) + sizeof(glm::vec4) + sizeof(glm::vec4), SHADER_STAGE_VERTEX_BIT}};
		info.inputSetLayouts = {layoutBindings};

		pipe->pipeline = renderer->createGraphicsPipeline(info, pipelineRenderPass, 0);
		pipe->depthPipeline = nullptr;

		if (pipeDef->canRenderDepth)
		{
			ShaderModule shadowFragShader = renderer->createShaderModule(workingDir + std::string(pipeDef->shadows_fragmentShaderFile), SHADER_STAGE_FRAGMENT_BIT);
			fragShaderStage.module = shadowFragShader;

			// Should probably do this better
			info.rasterizationInfo.clockwiseFrontFace = !info.rasterizationInfo.clockwiseFrontFace;
			info.stages[1].module = shadowFragShader;
			info.depthStencilInfo.depthCompareOp = COMPARE_OP_LESS;

			pipe->depthPipeline = renderer->createGraphicsPipeline(info, pipelineShadowRenderPass, 0);

			renderer->destroyShaderModule(shadowFragShader);
		}

		renderer->destroyShaderModule(vertShader);
		renderer->destroyShaderModule(fragShader);

		if (tessCtrlShader != nullptr)
			renderer->destroyShaderModule(tessCtrlShader);

		if (tessEvalShader != nullptr)
			renderer->destroyShaderModule(tessEvalShader);

		if (geomShader != nullptr)
			renderer->destroyShaderModule(geomShader);

		loadedPipelines[stringHash(defUniqueName)] = std::make_pair(pipe, 1);

		return pipe;
	}
	else
	{
		it->second.second ++;

		return it->second.first;
	}
}

ResourcePipeline ResourceManager::findPipeline (const std::string &defUniqueName)
{
	return findPipeline(stringHash(defUniqueName));
}

ResourcePipeline ResourceManager::findPipeline (size_t defUniqueNameHash)
{
	auto it = loadedPipelines.find(defUniqueNameHash);

	return it != loadedPipelines.end() ? it->second.first : nullptr;
}

void ResourceManager::returnPipeline (const std::string &defUniqueName)
{
	returnPipeline(stringHash(defUniqueName));
}

void ResourceManager::returnPipeline (size_t defUniqueNameHash)
{
	auto it = loadedPipelines.find(defUniqueNameHash);

	if (it != loadedPipelines.end())
	{
		// Decrement the reference counter
		it->second.second --;

		// If the reference counter is now zero, then delete the pipeline entirely
		if (it->second.second == 0)
		{
			ResourcePipeline pipeline = it->second.first;

			renderer->destroyPipeline(pipeline->pipeline);
			renderer->destroyPipeline(pipeline->depthPipeline);

			delete pipeline;

			loadedPipelines.erase(it);
		}
	}
}

void ResourceManager::addLevelDef (const LevelDef &def)
{
	LevelDef *levelDef = new LevelDef();
	*levelDef = def;

	loadedLevelDefsMap[stringHash(std::string(def.uniqueName))] = levelDef;
}

void ResourceManager::addMaterialDef (const MaterialDef &def)
{
	MaterialDef *matDef = new MaterialDef();
	*matDef = def;

	loadedMaterialDefsMap[stringHash(std::string(def.uniqueName))] = matDef;
}

void ResourceManager::addMeshDef (const StaticMeshDef &def)
{
	StaticMeshDef *meshDef = new StaticMeshDef();
	*meshDef = def;

	loadedMeshDefsMap[stringHash(std::string(def.uniqueName))] = meshDef;
}

void ResourceManager::addPipelineDef (const PipelineDef &def)
{
	PipelineDef *pipeDef = new PipelineDef();
	*pipeDef = def;

	loadedPipelineDefsMap[stringHash(std::string(def.uniqueName))] = pipeDef;
}

LevelDef *ResourceManager::getLevelDef (const std::string &defUniqueName)
{
	return getLevelDef(stringHash(defUniqueName));
}

MaterialDef *ResourceManager::getMaterialDef (const std::string &defUniqueName)
{
	return getMaterialDef(stringHash(defUniqueName));
}

StaticMeshDef *ResourceManager::getMeshDef (const std::string &defUniqueName)
{
	return getMeshDef(stringHash(defUniqueName));
}

PipelineDef *ResourceManager::getPipelineDef (const std::string &defUniqueName)
{
	return getPipelineDef(stringHash(defUniqueName));
}

LevelDef *ResourceManager::getLevelDef (size_t uniqueNameHash)
{
	auto it = loadedLevelDefsMap.find(uniqueNameHash);

	if (it != loadedLevelDefsMap.end())
		return it->second;

	return nullptr;
}

MaterialDef *ResourceManager::getMaterialDef (size_t uniqueNameHash)
{
	auto it = loadedMaterialDefsMap.find(uniqueNameHash);

	if (it != loadedMaterialDefsMap.end())
		return it->second;

	return nullptr;
}

StaticMeshDef *ResourceManager::getMeshDef (size_t uniqueNameHash)
{
	auto it = loadedMeshDefsMap.find(uniqueNameHash);

	if (it != loadedMeshDefsMap.end())
		return it->second;

	return nullptr;
}

PipelineDef *ResourceManager::getPipelineDef (size_t uniqueNameHash)
{
	auto it = loadedPipelineDefsMap.find(uniqueNameHash);

	if (it != loadedPipelineDefsMap.end())
		return it->second;

	return nullptr;
}

/*
 * Loads a mesh resource, complete with rendering data. This function is thread safe if called ONLY on the main thread,
 * and note that it might busy wait if the resource has already been requested to be loaded, but the
 * actual file loading/formatting has been pushed to a worker thread. Also note that this function
 * WILL return a valid and immediately usuable mesh resource, and will load from a cache if possible.
 * Preferably, and for explicit multithreading, or to load a mesh from another thread other than the main thread, use
 * the other in-built function(s). This function is NOT the most optimized out of all the other built-ins.
 *
 * This function loads mesh data in a format optimized for rendering, as specified in the function.
 */
ResourceMesh ResourceManager::loadMeshImmediate (const std::string &file, const std::string &mesh)
{
	std::unique_lock<std::mutex> lock(loadedMeshes_mutex);

	const MeshDataFormat rendererOptimizedMeshFormat = MESH_DATA_FORMAT_IVUNT;

	auto it = loadedMeshes.find(std::make_tuple(file, mesh, rendererOptimizedMeshFormat, true));

	if (it == loadedMeshes.end())
	{
		ResourceMeshObject *meshRes = new ResourceMeshObject();
		meshRes->file = file;
		meshRes->mesh = mesh;
		meshRes->meshFormat = rendererOptimizedMeshFormat;
		meshRes->interlaced = true;
		meshRes->dataLoaded = true;

		ResourceMeshData rawMeshData = loadRawMeshData(file, mesh);
		std::vector<char> formattedData = getFormattedMeshData(rawMeshData, rendererOptimizedMeshFormat, meshRes->indexChunkSize, meshRes->vertexStride, true);

		meshRes->faceCount = rawMeshData.faceCount;

		StagingBuffer stagingBuffer = renderer->createAndMapStagingBuffer(formattedData.size(), formattedData.data());
		meshRes->meshBuffer = renderer->createBuffer(formattedData.size(), BUFFER_USAGE_INDEX_BUFFER_BIT | BUFFER_USAGE_VERTEX_BUFFER_BIT | BUFFER_USAGE_TRANSFER_DST_BIT, MEMORY_USAGE_GPU_ONLY, false);

		CommandBuffer cmdBuffer = renderer->beginSingleTimeCommand(mainThreadTransferCommandPool);
		cmdBuffer->stageBuffer(stagingBuffer, meshRes->meshBuffer);
		renderer->endSingleTimeCommand(cmdBuffer, mainThreadTransferCommandPool, QUEUE_TYPE_GRAPHICS);

		renderer->destroyStagingBuffer(stagingBuffer);

		/*
		 * I'm formatting the debug name to trim it to the "GameData/meshes/" directory. For example:
		 * "GameData/meshes/blah.fbx" would turn to ".../blah.fbx""
		 */

		size_t i = file.find("/meshes/");

		std::string debugMarkerName = ".../";

		if (i != file.npos)
			debugMarkerName += file.substr(i + 8);
		else
			debugMarkerName = file;

		renderer->setObjectDebugName(meshRes->meshBuffer, OBJECT_TYPE_BUFFER, debugMarkerName + "[" + mesh + "]");

		loadedMeshes[std::make_tuple(file, mesh, rendererOptimizedMeshFormat, true)] = std::make_pair(meshRes, 1);

		return meshRes;
	}
	else
	{
		// Increment the reference counter
		it->second.second ++;

		// If the mesh object is loaded, but it's data isn't, then wait until it is
		// This usually happens when one thread pushes loading to another thread, and so
		// the object is created, but the loading hasn't finished yet
		while (it->second.first->dataLoaded == false)
		{
			std::this_thread::yield();
		}

		return it->second.first;
	}
}

/*
 * Returns an owner's possession of a mesh. Essentially just decrements the
 * reference counter for the resource, and deletes it if there's none left.
 */
void ResourceManager::returnMesh (ResourceMesh mesh)
{
	std::unique_lock<std::mutex> lock(loadedMeshes_mutex);

	auto it = loadedMeshes.find(std::make_tuple(mesh->file, mesh->mesh, mesh->meshFormat, mesh->interlaced));

	if (it != loadedMeshes.end())
	{
		// Decrement the reference counter
		it->second.second --;

		// If the reference counter is now zero, then delete the mesh data entirely
		if (it->second.second == 0)
		{
			loadedMeshes.erase(it);

			renderer->destroyBuffer(mesh->meshBuffer);

			delete mesh;
		}
	}
}

TextureFileFormat inferFileFormat (const std::string &file);

ResourceTexture ResourceManager::loadTextureImmediate (const std::string &file, TextureFileFormat format)
{
	std::unique_lock<std::mutex> lock(loadedTextures_mutex);

	auto it = loadedTextures.find(file);

	if (it == loadedTextures.end())
	{
		ResourceTextureObject *texRes = new ResourceTextureObject();
		texRes->files = {file};
		texRes->dataLoaded = true;
		texRes->arrayLayers = 1;

		if (format == TEXTURE_FILE_FORMAT_MAX_ENUM)
			format = inferFileFormat(file);

		if (format == TEXTURE_FILE_FORMAT_MAX_ENUM)
		{
			// If it's still max enum, then we failed to specify/find the format we're supposed to load it in
			// Therefore, we'll cancel and return a nullptr
			printf("%s Failed to load texture: %s, couldn't find or isn't a supported file format", ERR_PREFIX, file.c_str());

			return nullptr;
		}

		switch (format)
		{
			case TEXTURE_FILE_FORMAT_PNG:
				loadPNGTextureData(texRes);
				break;
			default:
				break;
		}

		/*
		 * I'm formatting the debug name to trim it to the "GameData/textures/" directory. For example:
		 * "GameData/textures/blah.png" would turn to ".../blah.png""
		 */

		size_t i = file.find("/textures/");

		std::string debugMarkerName = ".../";

		if (i != file.npos)
			debugMarkerName += file.substr(i + 10);
		else
			debugMarkerName = file;

		renderer->setObjectDebugName(texRes->texture, OBJECT_TYPE_TEXTURE, debugMarkerName);

		texRes->textureView = renderer->createTextureView(texRes->texture, TEXTURE_VIEW_TYPE_2D, {0, texRes->mipmapLevels, 0, 1});

		loadedTextures[file] = std::make_pair(texRes, 1);

		return texRes;
	}
	else
	{
		// Increment the reference counter
		it->second.second ++;

		// If the texture object is loaded, but it's data isn't, then wait until it is
		// This usually happens when one thread pushes loading to another thread, and so
		// the object is created, but the loading hasn't finished yet
		while (it->second.first->dataLoaded == false)
		{
			std::this_thread::yield();
		}

		return it->second.first;
	}
}

ResourceTexture ResourceManager::loadTextureArrayImmediate (const std::vector<std::string> &files, TextureFileFormat format)
{
	std::unique_lock<std::mutex> lock(loadedTextures_mutex);

	auto it = loadedTextures.find(files[0]);

	if (it == loadedTextures.end())
	{
		ResourceTextureObject *texRes = new ResourceTextureObject();
		texRes->files = files;
		texRes->dataLoaded = true;
		texRes->arrayLayers = 1;

		if (format == TEXTURE_FILE_FORMAT_MAX_ENUM)
			format = inferFileFormat(files[0]);

		if (format == TEXTURE_FILE_FORMAT_MAX_ENUM)
		{
			// If it's still max enum, then we failed to specify/find the format we're supposed to load it in
			// Therefore, we'll cancel and return a nullptr
			printf("%s Failed to load texture: %s, couldn't find or isn't a supported file format", ERR_PREFIX, files[0].c_str());

			return nullptr;
		}

		switch (format)
		{
			case TEXTURE_FILE_FORMAT_PNG:
				loadPNGTextureData(texRes);
				break;
			default:
				break;
		}

		/*
		 * I'm formatting the debug name to trim it to the "GameData/textures/" directory. For example:
		 * "GameData/textures/blah.png" would turn to ".../blah.png""
		 */

		size_t i = files[0].find("/textures/");

		std::string debugMarkerName = ".../";

		if (i != files[0].npos)
			debugMarkerName += files[0].substr(i + 10);
		else
			debugMarkerName = files[0];

		renderer->setObjectDebugName(texRes->texture, OBJECT_TYPE_TEXTURE, debugMarkerName);

		texRes->textureView = renderer->createTextureView(texRes->texture, TEXTURE_VIEW_TYPE_2D_ARRAY, {0, texRes->mipmapLevels, 0, (uint32_t) files.size()});

		loadedTextures[files[0]] = std::make_pair(texRes, 1);

		return texRes;
	}
	else
	{
		// Increment the reference counter
		it->second.second ++;

		// If the texture object is loaded, but it's data isn't, then wait until it is
		// This usually happens when one thread pushes loading to another thread, and so
		// the object is created, but the loading hasn't finished yet
		while (it->second.first->dataLoaded == false)
		{
			std::this_thread::yield();
		}

		return it->second.first;
	}
}

void ResourceManager::returnTexture (ResourceTexture tex)
{
	std::unique_lock<std::mutex> lock(loadedTextures_mutex);

	auto it = loadedTextures.find(tex->files[0]);

	if (it != loadedTextures.end())
	{
		// Decrement the reference counter
		it->second.second --;

		// If the reference counter is now zero, then delete the mesh data entirely
		if (it->second.second == 0)
		{
			loadedTextures.erase(it);
			renderer->destroyTexture(tex->texture);
			renderer->destroyTextureView(tex->textureView);

			delete tex;
		}
	}
}

void ResourceManager::loadPNGTextureData (ResourceTexture tex)
{
	uint32_t width = 0, height = 0;
	std::vector<std::vector<uint8_t>> textureData;

	for (uint32_t f = 0; f < tex->files.size(); f ++)
	{
		if (tex->files[f].length() == 0)
			continue;

		textureData.push_back(std::vector<uint8_t> ());

		uint32_t fwidth, fheight;

		unsigned err = lodepng::decode(textureData[f], fwidth, fheight, tex->files[f], LCT_RGBA, 8);

		if (err)
		{
			printf("%s Encountered an error while loading PNG file: %s, lodepng returned: %u\n", ERR_PREFIX, tex->files[f].c_str(), err);
					continue;
		}

		// Make sure each texture has the same size
		if (f > 0 && (fwidth != width || fheight != height))
		{
			printf("%s Couldn't load a texture array, one or more don't have consistent dimensions. Files: ", ERR_PREFIX);

			for (uint32_t i = 0; i < tex->files.size(); i ++)
			{
				printf("%s%s", tex->files[i].c_str(), (i == tex->files.size() - 1 ? "" : ", "));
			}
			printf("\n");

			return;
		}

		width = fwidth;
		height = fheight;
	}

	StagingBuffer stagingBuffer = renderer->createStagingBuffer(textureData[0].size());

	tex->mipmapLevels = (uint32_t) glm::floor(glm::log2(glm::max<float>(width, height))) + 1;
	tex->texture = renderer->createTexture({(float) width, (float) height, 1.0f}, RESOURCE_FORMAT_R8G8B8A8_UNORM, TEXTURE_USAGE_TRANSFER_SRC_BIT | TEXTURE_USAGE_TRANSFER_DST_BIT | TEXTURE_USAGE_SAMPLED_BIT, MEMORY_USAGE_GPU_ONLY, false, tex->mipmapLevels, textureData.size());

	for (uint32_t f = 0; f < tex->files.size(); f ++)
	{
		renderer->mapStagingBuffer(stagingBuffer, textureData[f].size(), textureData[f].data());

		CommandBuffer cmdBuffer = renderer->beginSingleTimeCommand(mainThreadTransferCommandPool);

		TextureSubresourceRange subresourceRange = {};
		subresourceRange.baseMipLevel = 0;
		subresourceRange.baseArrayLayer = f;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;

		//cmdBuffer->transitionTextureLayout(tex->texture, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
		cmdBuffer->setTextureLayout(tex->texture, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
		cmdBuffer->stageBuffer(stagingBuffer, tex->texture, {0, f, 1});

		if (tex->mipmapLevels > 1)
		{
			//cmdBuffer->transitionTextureLayout(tex->texture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			cmdBuffer->setTextureLayout(tex->texture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);

			for (uint32_t i = 1; i < tex->mipmapLevels; i ++)
			{
				TextureSubresourceRange mipRange = {};
				mipRange.baseMipLevel = i;
				mipRange.baseArrayLayer = f;
				mipRange.levelCount = 1;
				mipRange.layerCount = 1;

				TextureBlitInfo blitInfo = {};
				blitInfo.srcSubresource =
				{	i - 1, f, 1};
				blitInfo.dstSubresource =
				{	i, f, 1};
				blitInfo.srcOffsets[0] =
				{	0, 0, 0};
				blitInfo.dstOffsets[0] =
				{	0, 0, 0};
				blitInfo.srcOffsets[1] =
				{	int32_t(width >> (i - 1)), int32_t(height >> (i - 1)), 1};
				blitInfo.dstOffsets[1] =
				{	int32_t(width >> i), int32_t(height >> i), 1};

				//cmdBuffer->transitionTextureLayout(tex->texture, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, mipRange);
				cmdBuffer->setTextureLayout(tex->texture, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, mipRange, PIPELINE_STAGE_TRANSFER_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);
				cmdBuffer->blitTexture(tex->texture, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, tex->texture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, {blitInfo}, SAMPLER_FILTER_LINEAR);
				cmdBuffer->setTextureLayout(tex->texture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, mipRange, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_TRANSFER_BIT);
			}

			subresourceRange.levelCount = tex->mipmapLevels;

			cmdBuffer->setTextureLayout(tex->texture, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT);

			//cmdBuffer->transitionTextureLayout(tex->texture, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
		}
		else
		{
			cmdBuffer->transitionTextureLayout(tex->texture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		renderer->endSingleTimeCommand(cmdBuffer, mainThreadTransferCommandPool, QUEUE_TYPE_GRAPHICS);
	}

	renderer->destroyStagingBuffer(stagingBuffer);
}

ResourceMeshData ResourceManager::loadRawMeshData (const std::string &file, const std::string &mesh)
{
	ResourceMeshData meshData = {};

	std::unique_lock<std::mutex> lock(assimpImporter_mutex);

	const aiScene* scene = assimpImporter.ReadFile(file, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_ImproveCacheLocality);

	if (!scene)
	{
		printf("%s Failed to load file: %s, mesh: %s, with assimp. Returned: %s\n", ERR_PREFIX, file.c_str(), mesh.c_str(), assimpImporter.GetErrorString());
		assimpImporter_mutex.unlock();

		return meshData;
	}

	for (uint32_t i = 0; i < scene->mNumMeshes; i ++)
	{
		aiMesh* aiSceneMesh = scene->mMeshes[i];

		if (strcmp(aiSceneMesh->mName.C_Str(), mesh.c_str()) == 0)
		{
			meshData.vertices.resize(aiSceneMesh->mNumVertices);
			memcpy(meshData.vertices.data(), &aiSceneMesh->mVertices[0].x, aiSceneMesh->mNumVertices * sizeof(glm::vec3));

			if (aiSceneMesh->HasTextureCoords(0))
			{
				meshData.uvs.reserve(aiSceneMesh->mNumVertices);

				for (uint32_t uv = 0; uv < aiSceneMesh->mNumVertices; uv ++)
				{
					meshData.uvs.push_back({aiSceneMesh->mTextureCoords[0][uv].x, aiSceneMesh->mTextureCoords[0][uv].y});
				}
			}

			if (aiSceneMesh->HasNormals())
			{
				meshData.normals.resize(aiSceneMesh->mNumVertices);
				memcpy(meshData.normals.data(), &aiSceneMesh->mNormals[0].x, aiSceneMesh->mNumVertices * sizeof(glm::vec3));
			}

			if (aiSceneMesh->HasTangentsAndBitangents())
			{
				meshData.tangents.resize(aiSceneMesh->mNumVertices);
				memcpy(meshData.tangents.data(), &aiSceneMesh->mTangents[0].x, aiSceneMesh->mNumVertices * sizeof(glm::vec3));
			}

			if (aiSceneMesh->HasFaces())
			{
				meshData.faceCount = aiSceneMesh->mNumFaces;

				// If there are more than (2^16)-1 vertices, then we'll have to use 32-bit indices
				if (aiSceneMesh->mNumVertices > 35535)
				{
					meshData.uses32BitIndices = true;
					meshData.indices_32bit.reserve(aiSceneMesh->mNumFaces * 3);

					for (uint32_t f = 0; f < aiSceneMesh->mNumFaces; f ++)
					{
						meshData.indices_32bit.push_back((uint32_t) aiSceneMesh->mFaces[f].mIndices[0]);
						meshData.indices_32bit.push_back((uint32_t) aiSceneMesh->mFaces[f].mIndices[1]);
						meshData.indices_32bit.push_back((uint32_t) aiSceneMesh->mFaces[f].mIndices[2]);
					}
				}
				else
				{
					meshData.uses32BitIndices = false;
					meshData.indices_16bit.reserve(aiSceneMesh->mNumFaces * 3);

					for (uint32_t f = 0; f < aiSceneMesh->mNumFaces; f ++)
					{
						meshData.indices_16bit.push_back((uint16_t) aiSceneMesh->mFaces[f].mIndices[0]);
						meshData.indices_16bit.push_back((uint16_t) aiSceneMesh->mFaces[f].mIndices[1]);
						meshData.indices_16bit.push_back((uint16_t) aiSceneMesh->mFaces[f].mIndices[2]);
					}
				}
			}
		}
	}

	assimpImporter.FreeScene();

	return meshData;
}

void ResourceManager::setPipelineRenderPass (RendererRenderPass *renderPass, RendererRenderPass *shadowRenderPass)
{
	pipelineRenderPass = renderPass;
	pipelineShadowRenderPass = shadowRenderPass;
}

std::vector<char> ResourceManager::getFormattedMeshData (const ResourceMeshData &data, MeshDataFormat format, size_t &indexChunkSize, size_t &vertexStride, bool interlaceData)
{
	std::vector<char> meshDataArray;

	// Note that vertices are missing because meshes are always guaranteed to have vertices (nothing else, just vertices)
	bool requiresIndices = false, requiresUVs = false, requiresNormals = false, requiresTangents = false, requiresBitangents = false;

	switch (format)
	{
		case MESH_DATA_FORMAT_IVUNT:
		{
			requiresIndices = true;
			requiresUVs = true;
			requiresNormals = true;
			requiresTangents = true;

			break;
		}
		case MESH_DATA_FORMAT_IV:
		{
			requiresIndices = true;

			break;
		}
		case MESH_DATA_FORMAT_V:
			break;
		default:

			break;
	}

	if (interlaceData)
	{
		// Because indices don't necessarily correlate to the vertex data, all of them
		// go sequentially first in the buffer, which is the followed by the interlaced
		// vertex data
		if ((data.uses32BitIndices ? data.indices_32bit.size() > 0 : data.indices_16bit.size() > 0) > 0 && requiresIndices)
		{
			if (data.uses32BitIndices)
			{
				meshDataArray.resize(data.indices_32bit.size() * sizeof(data.indices_32bit[0]));
				memcpy(meshDataArray.data(), data.indices_32bit.data(), data.indices_32bit.size() * sizeof(data.indices_32bit[0]));
			}
			else
			{
				meshDataArray.resize(data.indices_16bit.size() * sizeof(data.indices_16bit[0]));
				memcpy(meshDataArray.data(), data.indices_16bit.data(), data.indices_16bit.size() * sizeof(data.indices_16bit[0]));
			}
		}

		size_t vertexSize = sizeof(data.vertices[0]);
		size_t interlacedDataStart = meshDataArray.size() * (requiresIndices ? 1 : 0); // I don't trust boolean math :D
		indexChunkSize = interlacedDataStart;

		if (requiresUVs)
			vertexSize += sizeof(svec2);

		if (requiresNormals)
			vertexSize += sizeof(svec3);

		if (requiresTangents)
			vertexSize += sizeof(svec3);

		if (requiresBitangents)
			vertexSize += sizeof(svec3);

		meshDataArray.resize(interlacedDataStart + vertexSize * data.vertices.size());

		vertexStride = vertexSize;

		for (size_t i = 0; i < data.vertices.size(); i ++)
		{
			memcpy(meshDataArray.data() + interlacedDataStart + i * vertexSize, &data.vertices[i].x, sizeof(data.vertices[0]));
			size_t offset = sizeof(data.vertices[0]);

			if (data.uvs.size() > 0 && requiresUVs)
			{
				memcpy(meshDataArray.data() + interlacedDataStart + i * vertexSize + offset, &data.uvs[i].x, sizeof(data.uvs[0]));
				offset += sizeof(data.uvs[0]);
			}
			else if (requiresUVs)
			{
				glm::vec2 zeroVec = glm::vec2(0, 0);
				memcpy(meshDataArray.data() + interlacedDataStart + i * vertexSize + offset, &zeroVec.x, sizeof(zeroVec));
				offset += sizeof(zeroVec);
			}

			if (data.normals.size() > 0 && requiresNormals)
			{
				memcpy(meshDataArray.data() + interlacedDataStart + i * vertexSize + offset, &data.normals[i].x, sizeof(data.normals[0]));
				offset += sizeof(data.normals[0]);
			}
			else if (requiresNormals)
			{
				glm::vec3 zeroVec = glm::vec3(0, 0, 0);
				memcpy(meshDataArray.data() + interlacedDataStart + i * vertexSize + offset, &zeroVec.x, sizeof(zeroVec));
				offset += sizeof(zeroVec);
			}

			if (data.tangents.size() > 0 && requiresTangents)
			{
				memcpy(meshDataArray.data() + interlacedDataStart + i * vertexSize + offset, &data.tangents[i].x, sizeof(data.tangents[0]));
				offset += sizeof(data.tangents[0]);
			}
			else if (requiresTangents)
			{
				glm::vec3 zeroVec = glm::vec3(0, 0, 0);
				memcpy(meshDataArray.data() + interlacedDataStart + i * vertexSize + offset, &zeroVec.x, sizeof(zeroVec));
				offset += sizeof(zeroVec);
			}
		}
	}
	else
	{

	}

	return meshDataArray;
}

TextureFileFormat inferFileFormat (const std::string &file)
{
	size_t lastPeriodInName = file.find_last_of('.');

	// If it has no file extension, at least for now we're not even gonna try
	if (lastPeriodInName == std::string::npos)
	{
		return TEXTURE_FILE_FORMAT_MAX_ENUM;
	}

	std::string fileExtension = file.substr(lastPeriodInName + 1, file.length());

	if (fileExtension == "png")
		return TEXTURE_FILE_FORMAT_PNG;
	else if (fileExtension == "dds")
		return TEXTURE_FILE_FORMAT_DDS;
	else if (fileExtension == "bmp")
		return TEXTURE_FILE_FORMAT_BMP;
	else if (fileExtension == "jpg")
		return TEXTURE_FILE_FORMAT_JPG;
	else if (fileExtension == "tga")
		return TEXTURE_FILE_FORMAT_TGA;
	else
		return TEXTURE_FILE_FORMAT_MAX_ENUM;
}

