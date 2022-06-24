#pragma once
#ifndef _ANIMATION_H_
#define _ANIMATION_H_

#include <string>
#include <vector>
#include <map>
#include "Bone.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct NodeData
{
    glm::mat4 transformation;
    std::string name;
    std::vector<NodeData> children;
};

struct BoneInfo
{
    int id;				//index in finalBoneMatrices
    glm::mat4 offset;	//model space to bone space
};

class Animation
{
    float m_Duration;
    float m_TicksPerSecond;
    std::vector<Bone> m_Bones;
    NodeData m_RootNode;
    std::unordered_map < std::string, BoneInfo > m_BoneInfoMap;

    void ReadHeirarchyData(NodeData& dest, const aiNode* src)
    {
        dest.name = src->mName.data;
        aiMatrix4x4 mat{ src->mTransformation };
        dest.transformation = { mat.a1, mat.b1, mat.c1, mat.d1,
                                mat.a2, mat.b2, mat.c2, mat.d2,
                                mat.a3, mat.b3, mat.c3, mat.d3,
                                mat.a4, mat.b4, mat.c4, mat.d4 };

        for (size_t i = 0 ; i < src->mNumChildren; ++i)
        {
            NodeData new_data;
            ReadHeirarchyData(new_data, src->mChildren[i]);
            dest.children.push_back(new_data);
        }
    }

    void ReadMissingBones(const aiAnimation* animation, std::unordered_map<std::string, BoneInfo>& bone_info_map)
    {

        for (size_t i = 0; i < animation->mNumChannels; ++i)
        {
            auto channel {animation->mChannels[i]};
            std::string bone_name{ channel->mNodeName.data };

            if (bone_info_map.find(bone_name) == bone_info_map.end())
            {
                bone_info_map[bone_name].id = static_cast<int>(bone_info_map.size());
            }

            m_Bones.push_back({CreateBone(bone_name,
                                          bone_info_map[bone_name].id,
                                          channel)});
        }

        m_BoneInfoMap = bone_info_map;
    }

    Bone CreateBone(std::string name, int ID, const aiNodeAnim* channel)
    {
        std::vector<KeyPosition> positions;
        std::vector<KeyRotation> rotations;
        std::vector<KeyScale> scales;

        for (size_t pos_index = 0; pos_index < channel->mNumPositionKeys; ++pos_index)
        {
            aiVector3D aiPosition {channel->mPositionKeys[pos_index].mValue};

            positions.push_back({ {aiPosition.x, aiPosition.y, aiPosition.z},
                                    static_cast<float>(channel->mPositionKeys[pos_index].mTime) });
        }

        for (size_t rot_index = 0; rot_index < channel->mNumRotationKeys; ++rot_index)
        {
            aiQuaternion aiOrientation{ channel->mRotationKeys[rot_index].mValue };
            
            rotations.push_back({ { aiOrientation.w, aiOrientation.x, aiOrientation.y, aiOrientation.z},
                                    static_cast<float>(channel->mRotationKeys[rot_index].mTime) });
        }

        for (size_t scale_index = 0; scale_index < channel->mNumScalingKeys; ++scale_index)
        {
            aiVector3D aiScale{ channel->mScalingKeys[scale_index].mValue };

            scales.push_back({ {aiScale.x, aiScale.y, aiScale.z},
                                static_cast<float>(channel->mScalingKeys[scale_index].mTime) });
        }

        return { positions, rotations, scales, name, ID };
    }

public:
    Animation() = default;

    Animation(float duration, float ticks_per_second,
              std::vector<Bone> bones, NodeData root_node,
              std::unordered_map <std::string, BoneInfo> bone_info_map)
        : m_Duration{ duration },
          m_TicksPerSecond{ ticks_per_second },
          m_Bones{ bones },
          m_RootNode{ root_node },
          m_BoneInfoMap{ bone_info_map }
    {

    }

    Animation(aiAnimation* animation, aiNode* root_node, std::unordered_map<std::string, BoneInfo>& bone_info_map)
    {
        m_Duration = static_cast<float>(animation->mDuration);
        m_TicksPerSecond = static_cast<float>(animation->mTicksPerSecond);
        ReadHeirarchyData(m_RootNode, root_node);
        ReadMissingBones(animation, bone_info_map);
    }

    Bone* FindBone(const std::string& name)
    {
        auto it { std::find_if(m_Bones.begin(), m_Bones.end(),
                  [&](const Bone& Bone)
                  {
                      return Bone.GetBoneName() == name;
                  } ) };
        
        if (it == m_Bones.end())
        {
            return nullptr;
        }

        return &(*it);
    }

    float& GetTicksPerSecond() { return m_TicksPerSecond; }

    float& GetDuration() { return m_Duration; }

    std::vector<Bone> GetBones() { return m_Bones; }

    NodeData& GetRootNode() { return m_RootNode; }

    const std::unordered_map<std::string, BoneInfo>& GetBoneIDMap() { return m_BoneInfoMap; }



};

#endif