#include "NUILoader.h"
#include "../RenderResource/RenderResourceManager.h"
#include <queue>

Model NUILoader::LoadNui(std::string file_path)
{
	Model model;
	std::ifstream ifs{ file_path, std::ifstream::binary };

	if (ifs)
	{
		//Read SubMesh header
		std::uint16_t num_submeshes{};
		ifs.read(reinterpret_cast<char*>(&num_submeshes), sizeof(std::uint16_t));

		for (std::uint16_t i = 0; i < num_submeshes ; ++i)
		{
			model.AddSubMesh(LoadCompiledSubMesh(ifs));
		}

		//Read BoneInfo header
		std::uint16_t num_bone_info{};
		ifs.read(reinterpret_cast<char*>(&num_bone_info), sizeof(std::uint16_t));

		for (std::uint16_t i = 0; i < num_bone_info; ++i)
		{
			LoadCompiledBoneInfo(ifs, model.GetBoneInfoMap());
		}

		//Read Animation header
		std::uint16_t num_animations{};
		ifs.read(reinterpret_cast<char*>(&num_animations), sizeof(std::uint16_t));

		for (std::uint16_t i = 0; i < num_animations; ++i)
		{
			model.GetAnimations().insert(LoadCompiledAnimation(ifs));
		}
	}

	int type;
	ifs.read(reinterpret_cast<char*>(&type), sizeof(int));
	model.SetPrimitive(type);
	return model;
}

Model::SubMesh NUILoader::LoadCompiledSubMesh(std::ifstream& ifs)
{
	std::vector<Model::Vertex> vertices{};
	std::vector<GLushort> indices{};

	std::pair<std::string, TempMaterial> material;
	LoadVector(vertices, ifs);
	LoadVector(indices, ifs);
	LoadString(material.first, ifs);
	LoadString(material.second.m_Ambient.first, ifs);
	LoadString(material.second.m_Ambient.second, ifs);
	LoadString(material.second.m_Diffuse.first, ifs);
	LoadString(material.second.m_Diffuse.second, ifs);
	LoadString(material.second.m_Normal.first, ifs);
	LoadString(material.second.m_Normal.second, ifs);
	LoadString(material.second.m_Specular.first, ifs);
	LoadString(material.second.m_Specular.second, ifs);

	return CreateSubMesh(vertices, indices, material);
}

void NUILoader::LoadCompiledBoneInfo(std::ifstream& ifs, std::unordered_map<std::string, BoneInfo>& map)
{
	std::string bone_info_name;
	int id{};
	glm::mat4 offset{};

	LoadString(bone_info_name, ifs);
	ifs.read(reinterpret_cast<char*>(&id), sizeof(int));
	ifs.read(reinterpret_cast<char*>(&offset), sizeof(glm::mat4));

	map.insert({bone_info_name, { id, offset }});
}

std::pair<std::string, Animation> NUILoader::LoadCompiledAnimation(std::ifstream& ifs)
{
	std::string animation_name;

	LoadString(animation_name, ifs);

	float duration;
	ifs.read(reinterpret_cast<char*>(&duration), sizeof(float));
	float ticks;
	ifs.read(reinterpret_cast<char*>(&ticks), sizeof(float));

	//Bone header
	std::uint16_t num_bones;
	ifs.read(reinterpret_cast<char*>(&num_bones), sizeof(std::uint16_t));

	std::vector<Bone> bones;

	for (std::uint16_t i = 0; i < num_bones; ++i)
	{
		bones.push_back(LoadCompiledBone(ifs));
	}

	std::uint32_t node_data_offset;
	std::uint32_t bone_offset;

	ifs.read(reinterpret_cast<char*>(&node_data_offset), sizeof(std::uint32_t));
	ifs.read(reinterpret_cast<char*>(&bone_offset), sizeof(std::uint32_t));

	NodeData root_node;
	LoadCompiledNodeData(root_node, ifs, node_data_offset + sizeof(std::uint32_t) * 2);	//take into account size of bone_offset and node_data_offset
	ifs.seekg(bone_offset);

	std::uint16_t num_bone_info{};
	ifs.read(reinterpret_cast<char*>(&num_bone_info), sizeof(std::uint16_t));

	std::unordered_map<std::string, BoneInfo> bone_id_map;

	for (std::uint16_t i = 0; i < num_bone_info; ++i)
	{
		LoadCompiledBoneInfo(ifs, bone_id_map);
	}

	return { animation_name, {duration, ticks, bones, root_node, bone_id_map} };
}

Bone NUILoader::LoadCompiledBone(std::ifstream& ifs)
{
	std::vector<KeyPosition> positions {};
	std::vector<KeyRotation> rotations {};
	std::vector<KeyScale> scales {};
	glm::mat4 local_transform;
	std::string name;
	int id;

	LoadVector(positions, ifs);
	LoadVector(rotations, ifs);
	LoadVector(scales, ifs);
	ifs.read(reinterpret_cast<char*>(&local_transform), sizeof(glm::mat4));
	LoadString(name, ifs);
	ifs.read(reinterpret_cast<char*>(&id), sizeof(int));

	return { positions, rotations, scales, local_transform, name, id };
}

void NUILoader::LoadCompiledNodeData(NodeData& node_data, std::ifstream& ifs, std::uint32_t offset)
{
	ifs.read(reinterpret_cast<char*>(&node_data.transformation), sizeof(glm::mat4));
	offset += sizeof(glm::mat4);
	offset += LoadString(node_data.name, ifs);

	std::uint16_t num_children{};
	ifs.read(reinterpret_cast<char*>(&num_children), sizeof(std::uint16_t));
	offset += sizeof(std::uint16_t);

	node_data.children.resize(num_children);

	for (std::uint16_t i = 0; i < num_children; ++i)
	{
		//set back to the current node's next offset data
		ifs.seekg(offset);

		std::uint32_t child_offset;
		ifs.read(reinterpret_cast<char*>(&child_offset), sizeof(std::uint32_t));
		offset += sizeof(std::uint32_t);
		
		ifs.seekg(child_offset);
		LoadCompiledNodeData(node_data.children[i], ifs, child_offset);
	}
}

template <typename T>
void NUILoader::LoadVector(std::vector<T>& vec, std::ifstream& ifs)
{
	std::uint32_t vec_size;
	ifs.read(reinterpret_cast<char*>(&vec_size), sizeof(std::uint32_t));

	vec.resize(vec_size / sizeof(T));

	if (vec_size) { ifs.read(reinterpret_cast<char*>(&vec[0]), vec_size); }
}

std::uint16_t NUILoader::LoadString(std::string& str, std::ifstream& ifs)
{
	std::uint16_t string_length{};
	ifs.read(reinterpret_cast<char*>(&string_length), sizeof(std::uint16_t));
	
	if (string_length)
	{
		str.resize(string_length);
		ifs.read(reinterpret_cast<char*>(&str[0]), string_length);
	}

	return sizeof(std::uint16_t) + sizeof(char) * string_length;
}

Model::SubMesh NUILoader::CreateSubMesh(std::vector<Model::Vertex>& vertices, std::vector<GLushort>& indices, std::pair<std::string, TempMaterial>& material)
{
	GLuint vbo;
	glCreateBuffers(1, &vbo);
	glNamedBufferStorage(vbo, sizeof(Model::Vertex) * vertices.size(), vertices.data(), GL_DYNAMIC_STORAGE_BIT);

	GLuint ebo;
	glCreateBuffers(1, &ebo);
	glNamedBufferStorage(ebo, sizeof(GLushort) * indices.size(), reinterpret_cast<GLvoid*>(indices.data()), GL_DYNAMIC_STORAGE_BIT);

	RenderResourceManager::GetInstanced().LoadMaterialNUI(material.first, material.second);

	return std::move(Model::SubMesh{ vertices, indices, vbo, ebo, static_cast<GLuint>(indices.size()), material.first });
}