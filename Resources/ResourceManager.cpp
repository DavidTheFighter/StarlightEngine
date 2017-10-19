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
	layoutBindings.push_back({2, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT});
	layoutBindings.push_back({3, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT});
	layoutBindings.push_back({4, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT});
	layoutBindings.push_back({5, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_FRAGMENT_BIT});

	mainThreadDescriptorPool = renderer->createDescriptorPool(layoutBindings, 16);
}

ResourceManager::~ResourceManager ()
{
	renderer->destroyCommandPool(mainThreadTransferCommandPool);
	renderer->destroyDescriptorPool(mainThreadDescriptorPool);
	// TODO Free all remaining resources when an instance of ResourceManager is deleted
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
		mat->sampler = renderer->createSampler(matDef->addressMode, matDef->linearFiltering ? SAMPLER_FILTER_LINEAR : SAMPLER_FILTER_NEAREST, matDef->linearFiltering ? SAMPLER_FILTER_LINEAR : SAMPLER_FILTER_NEAREST, 1, {0, 0, 0}, matDef->linearMipmapFiltering ? SAMPLER_MIPMAP_MODE_LINEAR : SAMPLER_MIPMAP_MODE_NEAREST);
		renderer->setObjectDebugName(mat->sampler, OBJECT_TYPE_SAMPLER, "Material: " + mat->defUniqueName + " sampler");

		std::vector<DescriptorWriteInfo> writes(1);
		writes[0].descriptorCount = 1;
		writes[0].descriptorType = DESCRIPTOR_TYPE_SAMPLER;
		writes[0].dstSet = mat->descriptorSet;
		writes[0].imageInfo = {{mat->sampler, nullptr, TEXTURE_LAYOUT_UNDEFINED}};
		writes[0].dstArrayElement = writes[0].dstBinding = 0;

		for (int i = 0; i < MATERIAL_DEF_MAX_TEXTURE_NUM; i ++)
		{
			mat->textures[i] = nullptr;

			if (std::string(matDef->textureFiles[i]).length() != 0)
			{
				mat->textures[i] = loadTextureImmediate(workingDir + std::string(matDef->textureFiles[i]));

				DescriptorWriteInfo writeInfo = {};
				writeInfo.descriptorCount = 1;
				writeInfo.descriptorType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				writeInfo.dstArrayElement = 0;
				writeInfo.dstBinding = uint32_t(i) + 1;
				writeInfo.dstSet = mat->descriptorSet;
				writeInfo.imageInfo = {{mat->sampler, mat->textures[i]->textureView, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};

				writes.push_back(writeInfo);
			}
		}

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

void ResourceManager::returnMaterial (ResourceMaterial mat)
{
	auto it = loadedMaterials.find(stringHash(mat->defUniqueName));

	if (it != loadedMaterials.end())
	{
		// Decrement the reference counter
		it->second.second --;

		// If the reference counter is now zero, then delete the mesh data entirely
		if (it->second.second == 0)
		{
			mainThreadDescriptorPool->freeDescriptorSet(mat->descriptorSet);
			renderer->destroySampler(mat->sampler);

			for (size_t i = 0; i < MATERIAL_DEF_MAX_TEXTURE_NUM; i ++)
			{
				if (mat->textures[i] != nullptr)
				{
					returnTexture(mat->textures[i]);
				}
			}

			delete mat;

			loadedMaterials.erase(it);
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

void ResourceManager::addMeshDef (const MeshDef &def)
{
	MeshDef *meshDef = new MeshDef();
	*meshDef = def;

	loadedMeshDefsMap[stringHash(std::string(def.uniqueName))] = meshDef;
}

LevelDef *ResourceManager::getLevelDef (const std::string &defUniqueName)
{
	return getLevelDef(stringHash(defUniqueName));
}

MaterialDef *ResourceManager::getMaterialDef (const std::string &defUniqueName)
{
	return getMaterialDef(stringHash(defUniqueName));
}

MeshDef *ResourceManager::getMeshDef (const std::string &defUniqueName)
{
	return getMeshDef(stringHash(defUniqueName));
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

MeshDef *ResourceManager::getMeshDef (size_t uniqueNameHash)
{
	auto it = loadedMeshDefsMap.find(uniqueNameHash);

	if (it != loadedMeshDefsMap.end())
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

		// Probably don't need this here, but I'm putting it here anyway
		COMPILER_BARRIER();

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
		texRes->file = file;
		texRes->dataLoaded = true;

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

		texRes->textureView = renderer->createTextureView(texRes->texture);

		// Probably don't need this here, but I'm putting here anyway. fight me
		COMPILER_BARRIER();
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

void ResourceManager::returnTexture (ResourceTexture tex)
{
	std::unique_lock<std::mutex> lock(loadedTextures_mutex);

	auto it = loadedTextures.find(tex->file);

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
	unsigned width, height;
	std::vector<uint8_t> textureData;

	lodepng::decode(textureData, width, height, tex->file, LCT_RGBA, 8);

	tex->texture = renderer->createTexture({(float) width, (float) height, 1.0f}, RESOURCE_FORMAT_R8G8B8A8_UNORM, TEXTURE_USAGE_TRANSFER_DST_BIT | TEXTURE_USAGE_SAMPLED_BIT, MEMORY_USAGE_GPU_ONLY, false);
	StagingBuffer stagingBuffer = renderer->createAndMapStagingBuffer(textureData.size(), textureData.data());

	CommandBuffer cmdBuffer = renderer->beginSingleTimeCommand(mainThreadTransferCommandPool);

	cmdBuffer->transitionTextureLayout(tex->texture, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL);
	cmdBuffer->stageBuffer(stagingBuffer, tex->texture);
	cmdBuffer->transitionTextureLayout(tex->texture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	renderer->endSingleTimeCommand(cmdBuffer, mainThreadTransferCommandPool, QUEUE_TYPE_GRAPHICS);

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

