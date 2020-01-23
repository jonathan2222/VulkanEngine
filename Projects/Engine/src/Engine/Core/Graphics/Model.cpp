#include "stdafx.h"
#include "Model.h"

ym::Model::Model() : m_info(Info()), m_vertexCount(0)
{
	m_va = VertexArray::create();
	m_ib = IndexBuffer::create();
}

ym::Model::~Model()
{
	delete m_va;
	delete m_ib;
}

void ym::Model::setData(const void* vertices, unsigned int vSize, 
	const void* indices, unsigned int iSize, const AttributeLayout& layout, Info info)
{
	m_info = info;

	// Set the vertex buffer.
	VertexBuffer* vb = VertexBuffer::create();
	vb->setData(vertices, vSize, info.usage);
	m_va->addBuffer(vb, layout);
	m_vertexCount = vSize / layout.getStride();

	// Set the index buffer.
	m_ib->setData(indices, iSize, info.usage);
}

void ym::Model::bind()
{
	m_va->bind();
	m_ib->bind();
}

unsigned int ym::Model::getVertexCount() const
{
	return m_vertexCount;
}

unsigned int ym::Model::getIndexCount() const
{
	return m_ib->getCount();
}

ym::VertexArray* ym::Model::getVertexArray()
{
	return m_va;
}

ym::IndexBuffer* ym::Model::getIndexBuffer()
{
	return m_ib;
}

ym::Model::Info ym::Model::getInfo() const
{
	return m_info;
}
