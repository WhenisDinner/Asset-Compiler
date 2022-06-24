# **Paperback Engine Mesh Compiler**

## **To use**
- Launch the solution file **Compiler.sln**
- Build the solution in Release x64
- Retrieve the **Compiler.exe**, **assimp-vc142-mtd.dll** and **glew32.dll** files from the x64/Release folder
- Copy them over to the Paperback2.0/resources/compiler folder
- Place uncompiled files into Paperback2.0/resources/models/uncompiled, and run **Compiler.exe**
- The compiled files will appear in the Paperback2.0/resources/models/nui folder
- NUILoader.h and NUILoader.cpp shows how the .nui files are turned into data used in the graphics pipeline of Paperback 2.0 Engine.