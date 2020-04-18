C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex pbrTestVert.glsl -o pbrTestVert.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag pbrTestFrag.glsl -o pbrTestFrag.spv

C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex terrainVert.glsl -o terrainVert.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag terrainFragTest.glsl -o terrainFragTest.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=compute terrainComp.glsl -o terrainComp.spv
pause