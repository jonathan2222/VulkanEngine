#pragma once

#include "AttributeLayout.h"
#include "VertexBuffer.h"

namespace ym
{
	class VertexArray
	{
	public:
		VertexArray();
		virtual ~VertexArray();

		static VertexArray* create();

		virtual void addBuffer(VertexBuffer* vb, const AttributeLayout& layout) = 0;
		virtual void bind() = 0;
	};
}
