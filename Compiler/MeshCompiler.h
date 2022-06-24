#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "CompiledModel.h"
#include "MeshBuilder.h"
#include "glm/gtc/type_ptr.hpp"

class MeshCompiler
{
	MeshBuilder* m_pMeshBuilder;

public:

	MeshCompiler(MeshBuilder* mesh_builder) : m_pMeshBuilder{ mesh_builder }
	{

	}

	void CompileMeshes(std::string fbx_directory, std::string nui_directory)
	{
		for (const auto& entry : std::filesystem::directory_iterator(fbx_directory))
		{
			std::string mesh_file_name{ entry.path().filename().generic_string() };
			size_t mesh_file_found = mesh_file_name.find(".fbx");

			if (mesh_file_found == std::string::npos)
			{
				mesh_file_found = mesh_file_name.find(".obj");
			}

			std::string fbx_path{ fbx_directory + "/" + mesh_file_name };

			if (mesh_file_found != std::string::npos)
			{
				std::string name{ mesh_file_name.substr(0, mesh_file_name.find(".")) };
				std::string nui_path{ nui_directory + "/" + name + ".nui" };

				//.nui exists
				if (std::filesystem::is_regular_file(nui_path))
				{
					//compare date modified
					auto obj_time = std::filesystem::last_write_time(fbx_path);
					auto nui_time = std::filesystem::last_write_time(nui_path);

					//is not latest version
					if (nui_time < obj_time)
					{
						//update file
						std::cout << ".nui file for " << mesh_file_name << " is being updated." << std::endl;
						CompileMesh(fbx_path, nui_path);
						std::cout << ".nui file for " << mesh_file_name << " has been updated." << std::endl;
					}

					else
					{
						std::cout << ".nui file for " << mesh_file_name << " is up-to-date." << std::endl;
					}
				}

				//.nui does not exist
				else
				{
					std::cout << ".nui file for " << mesh_file_name << " is not found." << std::endl;

					if (!std::filesystem::is_directory(nui_directory))
					{
						std::filesystem::create_directory(nui_directory);
						std::cout << "Creating directory: " << nui_directory << "." << std::endl;
					}

					std::cout << ".nui file for " << mesh_file_name << " is being created." << std::endl;
					std::ofstream new_nui_file{ nui_path };
					CompileMesh(fbx_path, nui_path);
					std::cout << ".nui file for " << mesh_file_name << " has been created." << std::endl;
				}
			}

			else if (std::filesystem::is_directory(fbx_path))
			{
				std::cout << "Checking folder: " << mesh_file_name << std::endl;
				CompileMeshes(fbx_path, nui_directory + "/" + mesh_file_name);
			}
		}
	}

	template <typename T, typename Stream>
	std::uint16_t WriteInfoToStream(T& info, Stream& stream)
	{
		std::uint16_t size{ sizeof(T) };

		if (size)
		{
			std::vector<std::byte> info_formatted{ size };
			std::memcpy(&info_formatted[0], &info, size);
			stream.write(reinterpret_cast<char*>(&info_formatted[0]), size);
		}

		return size;
	}

	template <typename T, typename Stream>
	std::uint32_t WriteInfoToStream(std::vector<T>& info, Stream& stream)
	{
		std::uint32_t size{ static_cast<std::uint32_t>(sizeof(T) * info.size()) };
		std::uint16_t length{ WriteInfoToStream(size, stream) };

		if (size)
		{
			std::vector<std::byte> info_formatted{ size };
			std::memcpy(&info_formatted[0], info.data(), size);
			stream.write(reinterpret_cast<char*>(info_formatted.data()), size);
		}

		return length + size;
	}

	template <typename Stream>
	std::uint16_t WriteInfoToStream(std::string& info, Stream& stream)
	{
		std::uint16_t length{ static_cast<std::uint16_t>(info.length()) };
		WriteInfoToStream(length, stream);

		std::uint16_t size{ std::uint16_t(sizeof(char) * info.length()) };

		if (size)
		{
			std::vector<std::byte> info_formatted{ size };
			std::memcpy(&info_formatted[0], &info[0], size);
			stream.write(reinterpret_cast<char*>(&info_formatted[0]), size);
		}

		return sizeof(std::uint16_t) + size;
	}

	template <typename Stream>
	std::uint16_t WriteInfoToStream(const std::string& info, Stream& stream)
	{
		std::uint16_t length{ static_cast<std::uint16_t>(info.length()) };
		WriteInfoToStream(length, stream);

		std::uint16_t size{ std::uint16_t(sizeof(char) * info.length()) };

		if (size)
		{
			std::vector<std::byte> info_formatted{ size };
			std::memcpy(&info_formatted[0], &info[0], size);
			stream.write(reinterpret_cast<char*>(&info_formatted[0]), size);
		}

		return sizeof(std::uint16_t) + size;
	}

	bool CheckIfAffectedByBone(std::vector<CompiledModel::SubMesh>& submeshes)
	{
		for (auto& sub_mesh : submeshes)
		{
			for (auto& vert : sub_mesh.m_Vertices)
			{
				for (int i = 0; i < 4; ++i)
				{
					if (vert.m_BoneIDs[i] != -1)
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	void CompileMesh(std::string fbx_name, std::string nui_name)
	{
		std::ofstream ofs;
		ofs.open(nui_name, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);

		CompiledModel model{ m_pMeshBuilder->Build3DMesh(fbx_name) };

		//ensure that animation moves for certain animations
		if (model.GetAnimations().size())
		{
			if (!CheckIfAffectedByBone(model.GetSubMeshes()))
			{
				for (auto& sub_mesh : model.GetSubMeshes())
				{
					for (auto& vert : sub_mesh.m_Vertices)
					{
						vert.m_BoneIDs[0] = 0;
						vert.m_Weights[0] = 1.0f;
					}
				}
			}
		}

		std::uint16_t num_submeshes { static_cast<std::uint16_t>(model.GetSubMeshes().size()) };
		std::uint16_t num_bone_info { static_cast<std::uint16_t>(model.GetBoneInfoMap().size()) };
		std::uint16_t num_animations { static_cast<std::uint16_t>(model.GetAnimations().size()) };
		std::uint32_t total_offset{};

		//Submesh header
		total_offset += WriteInfoToStream(num_submeshes, ofs);

		for (auto& sub_mesh : model.GetSubMeshes())
		{
			total_offset += CompileSubmesh(sub_mesh, ofs);
		}

		//Bone info header
		total_offset += WriteInfoToStream(num_bone_info, ofs);

		for (auto bone_info : model.GetBoneInfoMap())
		{
			total_offset += CompileBoneInfo(bone_info, ofs);
		}

		//Animation header
		total_offset += WriteInfoToStream(num_animations, ofs);

		for (auto animation : model.GetAnimations())
		{
			total_offset += CompileAnimation(animation, ofs, total_offset);
		}

		int type{ model.GetPrimitive() };
		WriteInfoToStream(type, ofs);
	}

	std::uint32_t CompileSubmesh(CompiledModel::SubMesh& sub_mesh, std::ofstream& ofs)
	{
		std::uint32_t size{};
		
		size += WriteInfoToStream(sub_mesh.m_Vertices, ofs);
		size += WriteInfoToStream(sub_mesh.m_Indices, ofs);

		//Has material
		if (sub_mesh.m_Material.first != "")
		{
			size += WriteInfoToStream(sub_mesh.m_Material.first, ofs);

			CompiledModel::Material& material{ sub_mesh.m_Material.second };
			size += WriteInfoToStream(material.m_Ambient.first, ofs);
			size += WriteInfoToStream(material.m_Ambient.second, ofs);
			size += WriteInfoToStream(material.m_Diffuse.first, ofs);
			size += WriteInfoToStream(material.m_Diffuse.second, ofs);
			size += WriteInfoToStream(material.m_Normal.first, ofs);
			size += WriteInfoToStream(material.m_Normal.second, ofs);
			size += WriteInfoToStream(material.m_Specular.first, ofs);
			size += WriteInfoToStream(material.m_Specular.second, ofs);
		}

		return size;
	}

	std::uint16_t CompileBoneInfo(const std::pair<const std::string, BoneInfo>& bone, std::ofstream& ofs)
	{
		std::uint16_t size{};

		size += WriteInfoToStream(bone.first, ofs);
		size += WriteInfoToStream(bone.second.id, ofs);
		size += WriteInfoToStream(bone.second.offset, ofs);

		return size;
	}

	std::uint32_t CompileAnimation(std::pair<const std::string, Animation>& animation, std::ofstream& ofs, const std::uint32_t& total_offset)
	{
		std::uint32_t size{};

		size += WriteInfoToStream(animation.first, ofs);
		
		size += WriteInfoToStream(animation.second.GetDuration(), ofs);
		size += WriteInfoToStream(animation.second.GetTicksPerSecond(), ofs);

		//Bone header
		std::uint16_t num_bones{ static_cast<std::uint16_t>(animation.second.GetBones().size()) };
		size += WriteInfoToStream(num_bones, ofs);

		for (auto& bone : animation.second.GetBones())
		{
			size += CompileBone(bone, ofs);
		}

		std::stringstream compiled_node_datas{};
		std::uint32_t node_data_offset{ total_offset + size };
		std::uint32_t node_data_size { CompileNodeData(animation.second.GetRootNode(), compiled_node_datas, node_data_offset + sizeof(std::uint32_t) * 2) };	//take into account size of bone_offset and node_data_offset
		size += node_data_size;

		size += WriteInfoToStream(node_data_offset, ofs);
		//Offset to BoneInfoMap
		std::uint32_t bone_offset{ node_data_offset + 2 * sizeof(std::uint32_t) + node_data_size };
		size +=  WriteInfoToStream(bone_offset, ofs);
		ofs << compiled_node_datas.rdbuf();

		//Bone info header
		std::uint16_t num_bone_info{ static_cast<std::uint16_t>(animation.second.GetBoneIDMap().size()) };
		size += WriteInfoToStream(num_bone_info, ofs);

		for (auto& bone_info : animation.second.GetBoneIDMap())
		{
			size += CompileBoneInfo(bone_info, ofs);
		}

		return size;
	}

	std::uint32_t CompileBone(Bone& bone, std::ofstream& ofs)
	{
		std::uint32_t size{};

		size += WriteInfoToStream(bone.m_Positions, ofs);
		size += WriteInfoToStream(bone.m_Rotations, ofs);
		size += WriteInfoToStream(bone.m_Scales, ofs);
		size += WriteInfoToStream(bone.m_LocalTransform, ofs);
		size += WriteInfoToStream(bone.m_Name, ofs);
		size += WriteInfoToStream(bone.m_ID, ofs);

		return size;
	}

	std::uint32_t CompileNodeData(NodeData& node_data, std::stringstream& compiled_node_datas, const std::uint32_t& offset)
	{
		std::uint32_t size{};

		size += WriteInfoToStream(node_data.transformation, compiled_node_datas);
		size += WriteInfoToStream(node_data.name, compiled_node_datas);

		if (node_data.children.empty())
		{
			std::uint16_t empty {0};
			size += WriteInfoToStream(empty, compiled_node_datas);
		}

		else
		{
			std::uint16_t num_children{ static_cast<std::uint16_t>(node_data.children.size()) };
			size += WriteInfoToStream(num_children, compiled_node_datas);

			//Accounts for storing of all child offsets
			size += sizeof(std::uint32_t) * num_children;

			std::stringstream compiled_children{};

			for (auto& child : node_data.children)
			{
				std::uint32_t child_offset = offset + size;
				WriteInfoToStream(child_offset, compiled_node_datas);								//offset where the child is located at
				size += CompileNodeData(child, compiled_children, child_offset);
			}

			compiled_node_datas << compiled_children.rdbuf();
		}

		return size;
	}
};