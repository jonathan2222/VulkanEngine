#pragma once

#include "VertexArray.h"
#include "IndexBuffer.h"

namespace ym
{
	class Model
	{
	public:
		struct Info
		{
			Info(Topology topology = Topology::TRIANGLE_LIST, Usage usage = Usage::STATIC) : topology(topology), usage(usage){}
			Topology topology;
			Usage usage;
		};

		Model();
		virtual ~Model();

		// The vertices should be spcified by the indices in clock wise order!
		void setData(const void* vertices, unsigned int vSize, const void* indices, unsigned int iSize, const AttributeLayout& layout, Info info);
		void bind();

		unsigned int getVertexCount() const;
		unsigned int getIndexCount() const;

		VertexArray* getVertexArray();
		IndexBuffer* getIndexBuffer();

		Info getInfo() const;

	private:
		VertexArray* m_va;
		IndexBuffer* m_ib;
		Info m_info;
		unsigned int m_vertexCount;
	};
}