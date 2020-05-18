C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex pbrTestVert.glsl -o pbrTestVert.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag pbrTestFrag.glsl -o pbrTestFrag.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag pbrFrag.glsl -o pbrFrag.spv

C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex terrainVert.glsl -o terrainVert.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag terrainFragTest.glsl -o terrainFragTest.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=compute terrainComp.glsl -o terrainComp.spv

C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex skyboxVert.glsl -o skyboxVert.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag skyboxFrag.glsl -o skyboxFrag.spv

C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex cubeVert.glsl -o cubeVert.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag cubeConvertFrag.glsl -o cubeConvertFrag.spv

C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag irradianceCubeFrag.glsl -o irradianceCubeFrag.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag prefilteredEnvMapFrag.glsl -o prefilteredEnvMapFrag.spv

C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex genbrdflutVert.glsl -o genbrdflutVert.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag genbrdflutFrag.glsl -o genbrdflutFrag.spv
pause