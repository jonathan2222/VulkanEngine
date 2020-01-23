#pragma once

#include <string>
#include "AttributeLayout.h"
#include "Texture.h"

#define YM_SHADER_PATH "./Resources/Shaders/"

#define YM_SHADER_TYPE_VERTEX 1
#define YM_SHADER_TYPE_PIXEL 2

namespace ym
{
	class Shader
	{
	public:
		Shader();
		virtual ~Shader();

		static Shader* create();

		/*
			Will load the vertex shader and pixel shader.
			Arguments: 
				fileName: Should be the path to the file, should not include the file extension!
		*/
		virtual void load(const std::string& fileName, AttributeLayout& layout) = 0;
		virtual void bind() = 0;
		virtual void* getId() = 0;

		virtual void setTexture(const std::string& name, Texture* texture, Sampler sampler, unsigned int unit) = 0;
	};
}