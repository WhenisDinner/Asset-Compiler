#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <glew/glew.h>
#include "glm/glm.hpp"
#include "Animation.h"

class CompiledModel
{
public:
	struct Vertex
	{
		glm::vec3 m_Position;
		glm::vec3 m_Normal;
		glm::vec2 m_UV;
		glm::vec3 m_Tangent;
		glm::vec3 m_BiTangent;

		int m_BoneIDs[4];
		float m_Weights[4];
	};

	struct Material
	{
		std::pair<std::string, std::string> m_Ambient; //texture name, path
		std::pair<std::string, std::string> m_Diffuse;
		std::pair<std::string, std::string> m_Specular;
		std::pair<std::string, std::string> m_Normal;
	};

	struct SubMesh
	{
		std::vector<Vertex> m_Vertices;
		std::vector<GLushort> m_Indices;

		// Submesh material
		std::pair <std::string, Material> m_Material;
	};

	CompiledModel() = default;
	~CompiledModel();

	void AddSubMesh(const SubMesh& Mesh);
	void RemoveAllSubMesh();
	void AddAnimation(const Animation& animation, std::string animation_name);
	std::unordered_map<std::string, Animation>& GetAnimations() { return m_Animations; }
	void SetPrimitive(const int& Primitive);
	int GetPrimitive();
	std::vector<SubMesh>& GetSubMeshes();
	auto& GetBoneInfoMap() { return m_BoneInfoMap; }

private:
	std::vector<SubMesh> m_SubMesh;
	int m_Type;
	std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
	std::unordered_map<std::string, Animation> m_Animations;
};