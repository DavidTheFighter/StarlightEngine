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
 * ResourceManager.h
 * 
 * Created on: Sep 27, 2017
 *     Author: david
 */

#ifndef RESOURCES_RESOURCEMANAGER_H_
#define RESOURCES_RESOURCEMANAGER_H_

#include <common.h>
#include <Resources/Resources.h>

#include <assimp/Importer.hpp>

class  Renderer;
struct RendererCommandPool;
class  RendererDescriptorPool;

/*
 * Manages resources such as meshes, textures, scripts, etc for the game. It
 * makes use of a reference-counter based cache to only load & keep unique
 * copies of resources.
 */
class ResourceManager
{
	public:
		ResourceManager (Renderer *rendererInstance, const std::string &gameWorkingDirectory);
		virtual ~ResourceManager ();

		ResourceMesh loadMeshImmediate (const std::string &file, const std::string &mesh);
		void returnMesh (ResourceMesh mesh);

		ResourceTexture loadTextureImmediate (const std::string &file, TextureFileFormat format = TEXTURE_FILE_FORMAT_MAX_ENUM);
		void returnTexture (ResourceTexture tex);

		ResourceMaterial loadMaterialImmediate (const std::string &defUniqueName);
		ResourceMaterial findMaterial (const std::string &defUniqueName);
		ResourceMaterial findMaterial (size_t defUniqueNameHash);
		void returnMaterial (ResourceMaterial mat);

		void loadGameDefsFile (const std::string &file);

		void addLevelDef (const LevelDef &def);
		void addMaterialDef (const MaterialDef &def);
		void addMeshDef (const MeshDef &def);

		LevelDef *getLevelDef (const std::string &defUniqueName);
		MaterialDef *getMaterialDef (const std::string &defUniqueName);
		MeshDef *getMeshDef (const std::string &defUniqueName);

		LevelDef *getLevelDef (size_t uniqueNameHash);
		MaterialDef *getMaterialDef (size_t uniqueNameHash);
		MeshDef *getMeshDef (size_t uniqueNameHash);

		static std::vector<char> getFormattedMeshData (const ResourceMeshData &data, MeshDataFormat format, size_t &indexChunkSize, size_t &vertexStride, bool interlaceData = true);

	private:

		std::string workingDir;

		Renderer *renderer;
		RendererCommandPool *mainThreadTransferCommandPool;
		RendererDescriptorPool *mainThreadDescriptorPool;

		std::map<size_t, MaterialDef*> loadedMaterialDefsMap;
		//std::vector<MaterialDef*> loadedMaterialDefs;

		std::map<size_t, MeshDef*> loadedMeshDefsMap;
		//std::vector<MeshDef*> loadedMeshDefs;

		std::map<size_t, LevelDef*> loadedLevelDefsMap;
		//std::vector<LevelDef*> loadedLevelDefs;

		std::map<size_t, std::pair<ResourceMaterial, uint32_t> > loadedMaterials;

		// Because I don't know the thread safety of assimp importers, the access is controlled by a mutex for now
		// TODO Figure out assimp importer thread safety
		std::mutex assimpImporter_mutex;
		Assimp::Importer assimpImporter;

		/*
		 * Note that at least for now, I'm using maps to do the proper cache lookups. Right now it's
		 * pretty inefficient because it has to compare a lot of variables. For meshes, it's 2 strings,
		 * an enum, and a boolean. Definitely will make this lookup faster later on.
		 *
		 * TODO Make resource manager cache lookups faster
		 */

		std::mutex loadedMeshes_mutex; // Controls access of member "loadedMeshes"

		/*
		 * The cache for mesh resources. The key consists of the mesh file, mesh name, data format, and a bool
		 * of whether it's interlaced or not. The mapped value consists of a pair, the first value being a ptr
		 * to the resource, the second being a reference counter. The counter works by incrementing each time
		 * loadMesh*() is called, and decremented when returnMesh() is called. When the reference counter is
		 * 0, the meshes is totally unloaded until the next call to loadMesh*(), where it wil have to load it
		 * all over again. At some point I will add a decay timer so this doesn't happen immediately.
		 *
		 * Note that the access to this member is controlled by "loadedMeshes_mutex"
		 *
		 * TODO Implement a decay timer for 0 reference counter cache objects in ResourceManager
		 */
		std::map<std::tuple<std::string, std::string, MeshDataFormat, bool>, std::pair<ResourceMesh, uint32_t> > loadedMeshes;

		std::mutex loadedTextures_mutex; // Controls access of member "loadedTextures"

		/*
		 * The cache for texture resources. The key is just the file name, and the mapped value consists of a
		 * pair. The first value is the actual texture resource, and the second value is a reference counter.
		 */
		std::map<std::string, std::pair<ResourceTexture, uint32_t> > loadedTextures;

		ResourceMeshData loadRawMeshData (const std::string &file, const std::string &mesh);

		void loadPNGTextureData (ResourceTexture tex);
};

#endif /* RESOURCES_RESOURCEMANAGER_H_ */
