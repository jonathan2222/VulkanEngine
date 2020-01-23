#pragma once

#include "Types.h"

namespace ym
{
	class IndexBuffer
	{
	public:
		IndexBuffer();
		virtual ~IndexBuffer();

		static IndexBuffer* create();

		unsigned int getCount() const;

		virtual void setData(const void* data, unsigned int count, Usage usage) = 0;
		virtual void bind() = 0;

	protected:
		unsigned int m_count;
	};
}