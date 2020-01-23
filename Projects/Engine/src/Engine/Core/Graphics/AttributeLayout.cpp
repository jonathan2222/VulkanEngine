#include "stdafx.h"
#include "AttributeLayout.h"

ym::AttributeLayout::AttributeLayout() : m_stride(0)
{
}

void ym::AttributeLayout::push(Format format, const std::string& semanticName, unsigned int instanceDivisor)
{
	Attribute attribute(format, semanticName, m_stride, instanceDivisor);
	m_stride += attribute.getSize();
	m_attributes.push_back(attribute);
}

unsigned int ym::AttributeLayout::getStride() const
{
	return m_stride;
}

const std::vector<ym::AttributeLayout::Attribute>& ym::AttributeLayout::getAttributes() const
{
	return m_attributes;
}

ym::AttributeLayout::Attribute::Attribute(Format format, const std::string& semanticName, unsigned int offset, unsigned int instanceDivisor) :
	m_offset(offset), m_semanticName(semanticName), m_size(0), m_type(Type::FLOAT), m_format(format), m_instanceDivisor(instanceDivisor)
{
	m_count = countOfFormat(format);
	m_type = typeOfFormat(format);
	m_size = sizeOfType(m_type) * m_count;
}

unsigned int ym::AttributeLayout::Attribute::getInstanceDivisor() const
{
	return m_instanceDivisor;
}

unsigned int ym::AttributeLayout::Attribute::getCount() const
{
	return m_count;
}

unsigned int ym::AttributeLayout::Attribute::getSize() const
{
	return m_size;
}

unsigned int ym::AttributeLayout::Attribute::getOffset() const
{
	return m_offset;
}

ym::Type ym::AttributeLayout::Attribute::getType() const
{
	return m_type;
}

ym::Format ym::AttributeLayout::Attribute::getFormat() const
{
	return m_format;
}

const std::string& ym::AttributeLayout::Attribute::getSemanticName() const
{
	return m_semanticName;
}
