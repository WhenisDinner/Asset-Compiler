#include "CompiledModel.h"

CompiledModel::~CompiledModel()
{
}

void CompiledModel::AddSubMesh(const SubMesh& Mesh)
{
	m_SubMesh.push_back(Mesh);
}

void CompiledModel::RemoveAllSubMesh()
{
	m_SubMesh.clear();
}

void CompiledModel::AddAnimation(const Animation& animation, std::string animation_name)
{
	m_Animations[animation_name] = animation;
}

void CompiledModel::SetPrimitive(const int& Primitive)
{
	m_Type = Primitive;
}

int CompiledModel::GetPrimitive()
{
	return m_Type;
}

std::vector<CompiledModel::SubMesh>& CompiledModel::GetSubMeshes()
{
	return m_SubMesh;
}