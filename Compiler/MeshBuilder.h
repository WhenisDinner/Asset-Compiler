/**********************************************************************************
* \file MeshBuilder.h
* \brief This is the mesh builder class which provides helper function to generate a
* quad mesh used for 2D rendering.
*
* \author  Malcolm Lim 100% Code Contribution
*
*		copyright Copyright (c) 2021 DigiPen Institute of Technology.
	Reproduction or disclosure of this file or its contents without the prior
	written consent of DigiPen Institute of Technology is prohibited.
******************************************************************************/
#ifndef MESHBUILDER_H
#define MESHBUILDER_H

#include <string>
#include "assimp/scene.h"

#include "CompiledModel.h"

class MeshBuilder
{
public:
	MeshBuilder() = default;
	MeshBuilder(const MeshBuilder&) = delete;
	~MeshBuilder() = default;

	MeshBuilder& operator=(const MeshBuilder&) = delete;

	static CompiledModel Build3DMesh(const std::string& File);

private:
	static CompiledModel::SubMesh ProcessSubMesh(aiMesh* SubMesh, const aiScene* Scene, CompiledModel& Mesh);
	static void ProcessNode(aiNode* Node, const aiScene* Scene, CompiledModel& Mesh);
	static void ExtractVertexBoneWeight(std::vector<CompiledModel::Vertex>& Vertices, aiMesh* SubMesh, CompiledModel& Mesh);
	static void LoadAnimations(aiAnimation** animation, int num_animations, aiNode* root_node, CompiledModel* model);
	static std::pair <std::string, CompiledModel::Material> LoadMaterial(const std::string& Material, aiMaterial* AiMat);
};

#endif