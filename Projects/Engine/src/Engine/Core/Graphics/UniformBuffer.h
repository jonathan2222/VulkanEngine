#pragma once

#include "Shader.h"

namespace ym
{
	class UniformBuffer
	{
	public:
		UniformBuffer();
		virtual ~UniformBuffer();

		static UniformBuffer* create();

		virtual void setShader(Shader& shader, unsigned int index, unsigned int bindingPoint) = 0;
		virtual void setData(const void* data, unsigned int size) = 0;
		virtual bool updateData(const void* data, unsigned int size, unsigned int offset) = 0;
		virtual void bind(unsigned int typesMask) = 0;
	};
}
