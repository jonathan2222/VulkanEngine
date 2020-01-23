#pragma once

#include <vector>
#include "Types.h"

namespace ym
{
	class AttributeLayout
	{
	public:
		class Attribute
		{
		public:
			Attribute(Format format, const std::string& semanticName, unsigned int offset, unsigned int instanceDivisor);

			unsigned int getInstanceDivisor() const;
			unsigned int getCount() const;
			unsigned int getSize() const;
			unsigned int getOffset() const;
			Type getType() const;
			Format getFormat() const;
			const std::string& getSemanticName() const;

		private:
			unsigned int m_instanceDivisor = 0;
			unsigned int m_offset = 0;
			unsigned int m_count = 0;
			unsigned int m_size;
			Type m_type;
			Format m_format;
			std::string m_semanticName;
		};

		AttributeLayout();
		void push(Format format, const std::string& semanticName, unsigned int instanceDivisor);

		unsigned int getStride() const;
		const std::vector<Attribute>& getAttributes() const;

	private:
		unsigned int m_stride;
		std::vector<Attribute> m_attributes;
	};
}
