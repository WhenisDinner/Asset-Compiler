#include "MeshCompiler.h"

int main()
{
	MeshBuilder mesh_builder;
	MeshCompiler mesh_compiler{&mesh_builder};

	std::cout << "Compiling Meshes..." << std::endl;
	mesh_compiler.CompileMeshes("../models/uncompiled", "../models/nui");
	system("pause");
}