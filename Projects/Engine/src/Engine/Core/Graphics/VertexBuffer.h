#pragma once

#include "Types.h"

namespace ym
{
	class VertexBuffer
	{
	public:
		VertexBuffer();
		virtual ~VertexBuffer();

		static VertexBuffer* create();

		virtual void setData(const void* data, unsigned int size, Usage usage) = 0;

		virtual void bind() = 0;

		virtual void* getBuffer() = 0;
		virtual unsigned int getSize() = 0;
	};
}