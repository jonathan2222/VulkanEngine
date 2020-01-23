#include "stdafx.h"
#include "Types.h"

unsigned int ym::sizeOfType(Type type)
{
	switch (type)
	{
	case Type::SINT: return 4; break;
	case Type::SBYTE: return 1; break;
	case Type::UINT: return 4; break;
	case Type::UBYTE: return 1; break;
	case Type::FLOAT:
	default: return 4;
	}
}

unsigned int ym::countOfFormat(Format format)
{
	switch (format)
	{
	case Format::SINT_8_R:
	case Format::UINT_8_R:
	case Format::SINT_32_R:
	case Format::UINT_32_R:
	case Format::FLOAT_32_R:
		return 1; break;
	case Format::SINT_8_RG:
	case Format::UINT_8_RG:
	case Format::SINT_32_RG:
	case Format::UINT_32_RG:
	case Format::FLOAT_32_RG:
		return 2; break;
	case Format::SINT_8_RGB:
	case Format::UINT_8_RGB:
	case Format::FLOAT_32_RGB:
	case Format::SINT_32_RGB:
	case Format::UINT_32_RGB:
		return 3; break;
	case Format::SINT_8_RGBA:
	case Format::UINT_8_RGBA:
	case Format::SINT_32_RGBA:
	case Format::UINT_32_RGBA:
	case Format::FLOAT_32_RGBA:
	default:
		return 4; break;
	}
}

ym::Type ym::typeOfFormat(Format format)
{
	switch (format)
	{
	case Format::SINT_8_R:
	case Format::SINT_8_RG:
	case Format::SINT_8_RGB:
	case Format::SINT_8_RGBA:
		return Type::SBYTE; break;
	case Format::UINT_8_R:
	case Format::UINT_8_RG:
	case Format::UINT_8_RGB:
	case Format::UINT_8_RGBA:
		return Type::UBYTE; break;
	case Format::SINT_32_R:
	case Format::SINT_32_RG:
	case Format::SINT_32_RGB:
	case Format::SINT_32_RGBA:
		return Type::SINT; break;
	case Format::UINT_32_R:
	case Format::UINT_32_RG:
	case Format::UINT_32_RGB:
	case Format::UINT_32_RGBA:
		return Type::UINT; break;
	case Format::FLOAT_32_R:
	case Format::FLOAT_32_RG:
	case Format::FLOAT_32_RGB:
	case Format::FLOAT_32_RGBA:
	default:
		return Type::FLOAT; break;
	}
}
