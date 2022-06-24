/**********************************************************************************
* \file MeshBuilder.cpp
* \brief This is the mesh builder class which provides helper function to generate a
* quad mesh used for 2D rendering.
*
* \author  Malcolm Lim 100% Code Contribution
*
*		copyright Copyright (c) 2021 DigiPen Institute of Technology.
	Reproduction or disclosure of this file or its contents without the prior
	written consent of DigiPen Institute of Technology is prohibited.
******************************************************************************/
#include "MeshBuilder.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include <vector>

CompiledModel MeshBuilder::Build3DMesh(const std::string& File)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(File, aiProcess_Triangulate |
												   aiProcess_LimitBoneWeights |
												   aiProcess_FindInstances |
												   aiProcess_GenSmoothNormals |
										 		   aiProcess_FlipUVs |
												   aiProcess_CalcTangentSpace |
												   aiProcess_JoinIdenticalVertices |
												   aiProcess_RemoveRedundantMaterials |
												   aiProcess_FindInvalidData);


	CompiledModel model;

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		return model;

	ProcessNode(scene->mRootNode, scene, model);

	model.SetPrimitive(GL_TRIANGLES);

	LoadAnimations(scene->mAnimations, scene->mNumAnimations, scene->mRootNode, &model);

	importer.FreeScene();

	return std::move(model);
}

void MeshBuilder::ProcessNode(aiNode* Node, const aiScene* Scene, CompiledModel& Mesh)
{
	for (size_t i = 0; i < Node->mNumMeshes; ++i)
	{
		aiMesh* mesh = Scene->mMeshes[Node->mMeshes[i]];
		Mesh.AddSubMesh(ProcessSubMesh(mesh, Scene, Mesh));
	}

	for (size_t i = 0; i < Node->mNumChildren; ++i)
	{
		ProcessNode(Node->mChildren[i], Scene, Mesh);
	}
}

CompiledModel::SubMesh MeshBuilder::ProcessSubMesh(aiMesh* SubMesh, const aiScene* Scene, CompiledModel& Mesh)
{
	std::vector<CompiledModel::Vertex> vertices;
	std::vector<GLushort> index;

	for (size_t i = 0; i < SubMesh->mNumVertices; ++i)
	{
		glm::vec3 position{ 0,0,0 }, normal{ 0,0,0 }, tangent{ 0,0,0 }, bitangent{ 0,0,0 };
		glm::vec2 uv{ 0,0 };

		position = glm::vec3{ SubMesh->mVertices[i].x, SubMesh->mVertices[i].y, SubMesh->mVertices[i].z };

		if (SubMesh->HasNormals())
			normal = glm::vec3{ SubMesh->mNormals[i].x, SubMesh->mNormals[i].y ,SubMesh->mNormals[i].z };

		if (SubMesh->mTextureCoords[0])
		{
			uv = glm::vec2{ SubMesh->mTextureCoords[0][i].x, SubMesh->mTextureCoords[0][i].y };
			tangent = glm::vec3{ SubMesh->mTangents[i].x, SubMesh->mTangents[i].y, SubMesh->mTangents[i].z };
			bitangent = glm::vec3{ SubMesh->mBitangents[i].x, SubMesh->mBitangents[i].y, SubMesh->mBitangents[i].z };
		}

		CompiledModel::Vertex vertex{ position, normal, uv, tangent, bitangent };

		//set bones to default
		for (int j = 0; j < 4; j++)
		{
			vertex.m_BoneIDs[j] = -1;
			vertex.m_Weights[j] = 0.0f;
		}

		vertices.push_back(vertex);
	}

	for (size_t i = 0; i < SubMesh->mNumFaces; ++i)
	{
		aiFace face = SubMesh->mFaces[i];

		for (size_t j = 0; j < face.mNumIndices; ++j)
			index.push_back(static_cast<GLushort>(face.mIndices[j]));
	}

	ExtractVertexBoneWeight(vertices, SubMesh, Mesh);

	aiString str;
	aiMaterial* material = Scene->mMaterials[SubMesh->mMaterialIndex];
	material->Get(AI_MATKEY_NAME, str);
	
	auto material_data = LoadMaterial(str.C_Str(), material);

	return std::move(CompiledModel::SubMesh{ vertices, index, material_data });
}

void MeshBuilder::ExtractVertexBoneWeight(std::vector<CompiledModel::Vertex>& Vertices, aiMesh* SubMesh, CompiledModel& Mesh)
{
	auto& bone_info_map{ Mesh.GetBoneInfoMap() };

	for (size_t bone_index = 0; bone_index < SubMesh->mNumBones; ++bone_index)
	{
		int bone_id{ -1 };
		std::string bone_name{ SubMesh->mBones[bone_index]->mName.C_Str() };

		if (bone_info_map.find(bone_name) == bone_info_map.end())
		{
			int new_id{ static_cast<int>(bone_info_map.size()) };

			auto mat{ SubMesh->mBones[bone_index]->mOffsetMatrix };
			BoneInfo bone_info{ new_id, {mat.a1, mat.b1, mat.c1, mat.d1,
										  mat.a2, mat.b2, mat.c2, mat.d2,
										  mat.a3, mat.b3, mat.c3, mat.d3,
										  mat.a4, mat.b4, mat.c4, mat.d4 } };

			bone_id = new_id;
			bone_info_map[bone_name] = bone_info;
		}

		else
		{
			bone_id = bone_info_map[bone_name].id;
		}

		auto weights{ SubMesh->mBones[bone_index]->mWeights };
		size_t num_weights{ SubMesh->mBones[bone_index]->mNumWeights };

		for (size_t weight_index = 0; weight_index < num_weights; ++weight_index)
		{
			size_t vertex_id{ weights[weight_index].mVertexId };
			float weight{ weights[weight_index].mWeight };

			for (int i = 0; i < 4; ++i)
			{
				//checks if there isn't data in the current slot
				if (Vertices[vertex_id].m_BoneIDs[i] == -1)
				{
					Vertices[vertex_id].m_Weights[i] = weight;
					Vertices[vertex_id].m_BoneIDs[i] = bone_id;
					break;
				}
			}
		}
	}
}

void MeshBuilder::LoadAnimations(aiAnimation** animation, int num_animations, aiNode* root_node, CompiledModel* model)
{
	if (animation)
	{
		for (int i = 0; i < num_animations; ++i)
		{
			model->AddAnimation({ animation[i], root_node, model->GetBoneInfoMap() }, { animation[i]->mName.C_Str() });
		}
	}
}

std::pair <std::string, CompiledModel::Material> MeshBuilder::LoadMaterial(const std::string& Material, aiMaterial* AiMat)
{
	std::pair <std::string, CompiledModel::Material> mat_data;
	aiString str;
	std::string filename;
	size_t start;

	mat_data.first = Material;

	AiMat->GetTexture(aiTextureType_DIFFUSE, 0, &str);

	filename = std::string{ str.C_Str() };
	start = filename.find_last_of('\\');
	if (start != std::string::npos) { filename = filename.substr(start + 1, filename.size() - 1); }
	mat_data.second.m_Diffuse = { filename, {"../../resources/textures/" + filename } };

	str.Clear();

	AiMat->GetTexture(aiTextureType_AMBIENT, 0, &str);

	filename = std::string{ str.C_Str() };
	start = filename.find_last_of("/\\");
	if (start != std::string::npos) { filename = filename.substr(start + 1, filename.size() - 1); }
	mat_data.second.m_Ambient = { filename, {"../../resources/textures/" + filename } };

	str.Clear();

	AiMat->GetTexture(aiTextureType_SPECULAR, 0, &str);

	filename = std::string{ str.C_Str() };
	start = filename.find_last_of("/\\");
	if (start != std::string::npos) { filename = filename.substr(start + 1, filename.size() - 1); }
	mat_data.second.m_Specular = { filename, {"../../resources/textures/" + filename } };

	str.Clear();

	AiMat->GetTexture(aiTextureType_NORMALS, 0, &str);

	filename = std::string{ str.C_Str() };
	start = filename.find_last_of("/\\");
	if (start != std::string::npos) { filename = filename.substr(start + 1, filename.size() - 1); }
	mat_data.second.m_Normal = { filename, {"../../resources/textures/" + filename } };
	
	return mat_data;
}