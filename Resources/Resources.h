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
 * Resources.h
 * 
 * Created on: Sep 27, 2017
 *     Author: david
 */

#ifndef RESOURCES_RESOURCES_H_
#define RESOURCES_RESOURCES_H_

// Instead of including a file in another header file & having massive compile-time issues, I'll just prototype here
struct RendererBuffer;
struct RendererTexture;
struct RendererTextureView;
class  RendererDescriptorSet;
struct RendererSampler;

#include <Rendering/Renderer/RendererEnums.h>

/*
 * Describes what components the data of a mesh must contain. The syntax of the enum describes what
 * components are present, and in what order. For example, using the keys below, MESH_DATA_FORMAT_IVNT
 * would contain, in order: indices, vertices, normals, and tangents.
 *
 * Note that if the data is built as interlaced, the format is a bit different. Indices exist in their own
 * block at the start of the buffer, and then the rest of the components are laid out interlaced in order after that.
 *
 * For example: (w/ Format IVUN)
 * 		Un-interlaced: 	IIIIIVVVVVUUUUUNNNNN
 *		Interlaced:		IIIIIVUNVUNVUNVUNVUN
 *
 * Table of keys:
 *
 * I - Indices
 * V - Vertices (which are guaranteed to ALWAYS be present in a mesh data format)
 * U - UVs / Texcoords
 * N - Normals
 * T - Tangents
 * B - Bitangents
 * D - Bone IDs     (for skinning, represented as an ivec4)
 * W - Bone Weights (for skinning, represented as a vec4)
 *
 */
typedef enum MeshDataFormat
{
	MESH_DATA_FORMAT_IVUNT = 0,
	MESH_DATA_FORMAT_IV,
	MESH_DATA_FORMAT_V,
	MESH_DATA_FORMAT_MAX_ENUM
} MeshDataFormat;

/*
 * A list of (currently or eventually) supported file formats to load textures from. Usually
 * the format is inferred from the file extension, which can be inferred by passing the value
 * TEXTURE_FILE_FORMAT_MAX_ENUM to functions. For example when loading a texture using
 * ResourceManager, if you don't know the file format of the texture you're loading, pass
 * TEXTURE_FILE_FORMAT_MAX_ENUM to let the function infer what format it is.
 *
 */
typedef enum TextureFileFormat
{
	TEXTURE_FILE_FORMAT_PNG = 0,
	TEXTURE_FILE_FORMAT_DDS,
	TEXTURE_FILE_FORMAT_BMP,
	TEXTURE_FILE_FORMAT_JPG,
	TEXTURE_FILE_FORMAT_TGA,
	TEXTURE_FILE_FORMAT_MAX_ENUM
} TextureFileFormat;

/*
 * The raw data for a mesh. If the size() of a vector is != 0, then that component is
 * considered to hold valid data for the entire mesh. If size() == 0, then that component
 * is omitted. For example, bitangents are usually calculated on the fly in the vertex shader,
 * and so are usually missing from raw mesh data. Therefore, the valid way to signify this
 * is to have bitangents.size() == 0.
 */
typedef struct ResourceMeshData
{
		uint32_t faceCount;
		bool uses32BitIndices; // Note if there are no indices, then this value is simply ignored

		std::vector<uint16_t> indices_16bit;
		std::vector<uint32_t> indices_32bit;

		std::vector<svec3> vertices;
		std::vector<svec2> uvs;
		std::vector<svec3> normals;
		std::vector<svec3> tangents;
		std::vector<svec3> bitangents;
		std::vector<sivec4> boneIDs;
		std::vector<svec4> boneWeights;
} ResourceMeshData;

/*
 * A mesh resource data struct. Contains all the data needed to render the mesh,
 * and compare w/ other meshes. Do not create this yourself, use the provided
 * functions in ResourceManager.h to load and unload meshes, both for safety and
 * to make use of it's internal cache.
 *
 * Note that most mesh file types can contain multiple meshes, and so meshes are
 * named both by their file, and the mesh that was loaded from it. In the case of
 * custom binary formats that only contain one mesh, 'file' is the only valid name,
 * and 'mesh' is an empty string.
 */
typedef struct ResourceMeshObject
{
		std::atomic<bool> dataLoaded; // Used for multi-threaded resource loading
		size_t indexChunkSize; // The size in bytes of the index value chunk at the start of the mesh buffer
		size_t vertexStride;   // The stride in bytes of each vertex
		uint32_t faceCount;
		std::string file;
		std::string mesh;

		MeshDataFormat meshFormat;
		bool interlaced;

		RendererBuffer *meshBuffer; // Contains ALL the data for the mesh
} *ResourceMesh;

/*
 * A texture resource data struct. Contains all the data needed to use the texture
 * in rendering & materials. Do not create this yourself, use the provided
 * functions in ResourceManager.h to load and unload textures.
 */
typedef struct ResourceTextureObject
{
		std::atomic<bool> dataLoaded; // Used for multi-threaded texture loading
		std::string file;
		ResourceFormat textureFormat;
		uint32_t mipmapLevels;

		RendererTexture *texture;
		RendererTextureView *textureView; // A view for the whole texture, aka a default view
} *ResourceTexture;

#define RESOURCE_DEF_MAX_NAME_LENGTH 64
#define RESOURCE_DEF_MAX_FILE_LENGTH 128
#define MATERIAL_DEF_MAX_TEXTURE_NUM 5

typedef struct ResourceMaterialObject
{
		std::string defUniqueName;

		ResourceTexture textures[MATERIAL_DEF_MAX_TEXTURE_NUM];

		RendererDescriptorSet *descriptorSet;
		RendererSampler *sampler;

} *ResourceMaterial;

typedef struct ResourceStaticMeshObject
{
		std::string defUniqueName;

		ResourceMesh mesh;

} *ResourceStaticMesh;

/*
 * The definition for a material. It includes it's unique name, all of the
 * constituent textures, it's material properties, etc.
 */
typedef struct ResourceMaterialDefinition
{
		// The unique name/identifier of material, MUST be the only loaded material definition with this uniqueName
		char uniqueName[RESOURCE_DEF_MAX_NAME_LENGTH];

		/*
		 * The list of textures used in the material, specifying the file
		 * that the texture is from. Files should be specified relative to
		 * the root game dir, so a texture in "${GAME_DIR}/GameData/textures/blah.png"
		 * would be specified as "GameData/textures/blah.png". There should be NO slash
		 * as the first character.
		 *
		 * The textures can be used for anything, with a max of 8 textures, as long as
		 * the pipeline specified with this material is compatible with it.
		 */
		char textureFiles[MATERIAL_DEF_MAX_TEXTURE_NUM][RESOURCE_DEF_MAX_FILE_LENGTH];

		/*
		 * The graphics pipeline definition that this material will use to render.
		 * I've decided to enforce using separate definitions to enforce that materials
		 * should try and use as few pipelines as possible, instead of allowing
		 * individual settings for each material and selecting a pipeline from a cache.
		 */
		char pipelineUniqueName[RESOURCE_DEF_MAX_NAME_LENGTH];

		/*
		 * The list of sampler definitions, as each material will get it's own
		 * descriptor set & immutable sampler (from a cache).
		 */

		bool enableAnisotropy;
		SamplerAddressMode addressMode; // Note only repeat, mirrored repeat, and clamp to edge are valid, clamp to border is disabled for now
		bool linearFiltering;
		bool linearMipmapFiltering;

} MaterialDef;

typedef struct ResourcePipelineDefinition
{
		// The unique name/identifier of pipeline, MUST be the only loaded pipeline definition with this uniqueName
		char uniqueName[RESOURCE_DEF_MAX_NAME_LENGTH];

		/*
		 * The list of shaders used in this pipeline. For optional stages,
		 * they are considered omited if the string's length() == 0. The file
		 * should be specified relative to the root game dir, so
		 * "${GAME_DIR}/GameData/shaders/blah.glsl" would be specifed as
		 * "GameData/shaders/blah.glsl"
		 */

		char vertexShaderFile[RESOURCE_DEF_MAX_FILE_LENGTH];
		char tessControlShaderFile[RESOURCE_DEF_MAX_FILE_LENGTH];
		char tessEvalShaderFile[RESOURCE_DEF_MAX_FILE_LENGTH];
		char geometryShaderFile[RESOURCE_DEF_MAX_FILE_LENGTH];
		char fragmentShaderFile[RESOURCE_DEF_MAX_FILE_LENGTH];

		/*
		 * The list of pipeline options, such as face culling.
		 */

		bool clockwiseFrontFace;
		bool backfaceCulling;
		bool frontfaceCullilng;
		bool polygonLineMode;

} PipelineDef;

typedef struct ResourceStaticMeshDefinition
{
		// The unique name/identifier of mesh, MUST be the only loaded mesh definition with this uniqueName
		char uniqueName[RESOURCE_DEF_MAX_NAME_LENGTH];

		// The file the mesh is to be loaded file
		char meshFile[RESOURCE_DEF_MAX_FILE_LENGTH];

		// The name of the mesh within the file to load, is ignored for custom binary formats w/ only one mesh per file
		char meshName[RESOURCE_DEF_MAX_NAME_LENGTH];

} StaticMeshDef;

typedef struct ResourceLevelDefinition
{
		char uniqueName[RESOURCE_DEF_MAX_NAME_LENGTH];
} LevelDef;

#endif /* RESOURCES_RESOURCES_H_ */

