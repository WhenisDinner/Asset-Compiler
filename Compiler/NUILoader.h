#pragma once
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include "../Mesh/Model.h"

class NUILoader
{
public:

	struct TempNodeData
	{
		glm::mat4 transformation;
		std::string name;
		std::string parent_name;
		std::vector<TempNodeData> children;
	};

	struct TempMaterial
	{
		std::pair<std::string, std::string> m_Ambient; //texture name, path
		std::pair<std::string, std::string> m_Diffuse;
		std::pair<std::string, std::string> m_Specular;
		std::pair<std::string, std::string> m_Normal;
	};

	Model LoadNui(std::string file_path);
	Model::SubMesh LoadCompiledSubMesh(std::ifstream& ifs);
	void  LoadCompiledBoneInfo(std::ifstream& ifs, std::unordered_map<std::string, BoneInfo>& map);
	std::pair<std::string, Animation> LoadCompiledAnimation(std::ifstream& ifs);
	Bone LoadCompiledBone(std::ifstream& ifs);
	void LoadCompiledNodeData(NodeData& node_data, std::ifstream& ifs, std::uint32_t offset);

	template <typename T>
	void LoadVector(std::vector<T>& vec, std::ifstream& ifs);
	std::uint16_t LoadString(std::string& str, std::ifstream& ifs);
	Model::SubMesh CreateSubMesh(std::vector<Model::Vertex>& vertices, std::vector<GLushort>& indices, std::pair<std::string, TempMaterial>& material);
};