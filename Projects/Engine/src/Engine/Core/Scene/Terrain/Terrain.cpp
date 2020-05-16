#include "stdafx.h"
#include "Terrain.h"
#include "Engine/Core/Vulkan/Factory.h"
#include "Engine/Core/Input/Config.h"

void ym::Terrain::init(uint32_t dataWidth, uint32_t dataHeight, uint8_t* data, Description description)
{
	int32_t regionSize = Config::get()->fetch<int>("Terrain/regionSize");
	int32_t proximityRadius = Config::get()->fetch<int>("Terrain/proximityRadius");
	this->proximityDescription = Terrain::createProximityDescription(dataWidth, regionSize, proximityRadius);
	
	this->desc = description;
	
	int32_t padding = this->proximityDescription.regionWidthCount * (this->proximityDescription.regionSize - 1) + 1 - dataWidth;

	this->width = dataWidth + padding;
	this->height = dataHeight + padding;
	
	generateVertices(dataWidth, dataHeight, data);
	generateNormals();

	static uint64_t idGenerator = 0;
	this->uniqueId = idGenerator++;
}

void ym::Terrain::destroy()
{
}

float ym::Terrain::getHeightAt(const glm::vec3& pos) const
{
	auto toOffest = [&](glm::ivec2 v)->uint32_t {
		v = glm::clamp(v, 0, this->width - 1);
		return v.x + v.y * this->height;
	};

	float xDist = pos.x - this->desc.origin.x;
	float zDist = pos.z - this->desc.origin.z;

	// This is the bottom left corner in the quad x and z is in.
	glm::ivec2 blIdx;
	blIdx.x = static_cast<int>(xDist / this->desc.vertDist);
	blIdx.y = static_cast<int>(zDist / this->desc.vertDist);

	// Get the other corner indices.
	glm::ivec2 tlIdx(blIdx.x, blIdx.y - 1);
	glm::ivec2 trIdx(blIdx.x + 1, blIdx.y - 1);
	glm::ivec2 brIdx(blIdx.x + 1, blIdx.y);

	// Fetch each vertices height
	float bl = this->vertices[toOffest(blIdx)].pos.y;
	float tl = this->vertices[toOffest(tlIdx)].pos.y;
	float br = this->vertices[toOffest(brIdx)].pos.y;
	float tr = this->vertices[toOffest(trIdx)].pos.y;

	// Check which triangle it is in.
	glm::vec2 point;
	point.x = fmod(xDist, this->desc.vertDist) / this->desc.vertDist;
	point.y = fmod(zDist, this->desc.vertDist) / this->desc.vertDist;
	bool isLeft = point.x <= (1.0f - point.y);

	float height = 0.0f;
	if (isLeft)
	{
		// Top left triangle
		height = barryCentricHeight({ 0, tl, 0 }, { 1, tr, 0 }, { 0, bl, 1 }, point);
	}
	else
	{
		// Bottom right triangle
		height = barryCentricHeight({ 1, tr, 0 }, { 1, br, 1 }, { 0, bl, 1 }, point);
	}

	return height;
}

glm::ivec2 ym::Terrain::getRegionFromPos(const glm::vec3& pos, int32_t regionSize) const
{
	float regionWorldSize = (regionSize - 1) * this->desc.vertDist;

	float xDistance = pos.x - this->desc.origin.x;
	float zDistance = pos.z - this->desc.origin.z;

	// Calculate region index from position
	glm::vec2 index(0);
	index.x = (float)static_cast<int>(xDistance / regionWorldSize);
	index.y = (float)static_cast<int>(zDistance / regionWorldSize);

	return index;
}

void ym::Terrain::fetchVertices(int32_t proximityWidth, const glm::vec3& pos, std::vector<Vertex>& verticesOut) const
{
	float xDistance = pos.x - this->desc.origin.x;
	float zDistance = pos.z - this->desc.origin.z;

	// Calculate vertex index from position
	int xIndex = static_cast<int>(xDistance / this->desc.vertDist);
	int zIndex = static_cast<int>(zDistance / this->desc.vertDist);

	// Handle both odd and even vertex count on height map
	if (proximityWidth % 2 != 0) {
		// Odd
		xIndex -= proximityWidth / 2;
		zIndex -= proximityWidth / 2;
	}
	else {
		// Even
		xIndex -= (proximityWidth / 2) - 1;
		zIndex -= (proximityWidth / 2) - 1;
	}

	// Return vertices within proximity off position. {PADDED VERSION}
	for (int j = 0; j < proximityWidth; j++)
	{
		int zTemp = zIndex + j;
		for (int i = 0; i < proximityWidth; i++)
		{
			int xTemp = xIndex + i;
			if ((xTemp >= 0 && xTemp < this->width) && (zTemp >= 0 && zTemp < this->height)) {
				int offset = xTemp + zTemp * this->width;
				verticesOut[(size_t)i + (size_t)j * (size_t)proximityWidth] = this->vertices[offset];
			}
			else {
				Vertex padVertex;
				padVertex.pos = glm::vec3(xTemp, 0.f, zTemp) * this->desc.vertDist + this->desc.origin;
				padVertex.nor = glm::vec3(0.f, 1.f, 0.f);
				padVertex.uv0 = glm::vec2(xTemp, zTemp);
				verticesOut[(size_t)i + (size_t)j * (size_t)proximityWidth] = padVertex;
			}
		}
	}
}

uint64_t ym::Terrain::getUniqueID() const
{
	return this->uniqueId;
}

VkDescriptorSet& ym::Terrain::getDescriptorSet()
{
	return this->descriptorSet;
}

int32_t ym::Terrain::getRegionCount() const
{
	return this->proximityDescription.regionCount;
}

glm::vec3 ym::Terrain::getOrigin() const
{
	return this->desc.origin;
}

ym::Terrain::ProximityDescription ym::Terrain::createProximityDescription(int32_t dataWidth, int32_t regionSize, int32_t proximityRadius)
{
	Terrain::ProximityDescription desc;
	desc.regionSize = regionSize;
	desc.proximityRadius = proximityRadius;

	const int quads = desc.regionSize - 1;
	desc.indiciesPerRegion = (quads * quads) * 6;

	int regionCountWidth = 1 + desc.proximityRadius * 2;
	desc.proximityWidth = desc.regionSize * regionCountWidth - regionCountWidth + 1;

	desc.regionWidthCount = static_cast<int32_t>(ceilf((dataWidth - 1) / (float)(desc.regionSize - 1)));
	desc.regionCount = desc.regionWidthCount * desc.regionWidthCount;
	return desc;
}

void ym::Terrain::generateVertices(uint32_t dataWidth, uint32_t dataHeight, uint8_t* data)
{
	const unsigned maxValue = 0xFF;
	float zDist = abs(this->desc.maxZ - this->desc.minZ);
	this->vertices.resize((size_t)this->width * (size_t)this->height);

	for (int32_t z = 0; z < this->height; z++)
	{
		for (int32_t x = 0; x < this->width; x++)
		{
			float height = 0;
			if (z < (int32_t)dataHeight && x < (int32_t)dataWidth)
				height = this->desc.minZ + ((float)data[x + z * (int32_t)dataWidth] / maxValue) * zDist;

			float xPos = this->desc.origin.x + x * this->desc.vertDist;
			float zPos = this->desc.origin.z + z * this->desc.vertDist;

			Vertex& vertex = this->vertices[(size_t)x + (size_t)z * (size_t)this->height];
			vertex.pos = glm::vec4(xPos, height, zPos, 1.0);
			vertex.uv0 = glm::vec2(x, z);
		}
	}
}

void ym::Terrain::generateNormals()
{
	auto getIndex = [&](int32_t x, int32_t z)->size_t {
		return (size_t)x + (size_t)z * (size_t)this->height;
	};

	for (int32_t z = 0; z < this->height; z++)
	{
		for (int32_t x = 0; x < this->width; x++)
		{
			float west, east, north, south;

			//Get adjacent vertex height from heightmap image then calculate normals with the height
			int32_t offset = std::max((z - 1), 0);
			west = this->vertices[getIndex(x, offset)].pos.y;

			offset = glm::min((z + 1), this->width - 1);
			east = this->vertices[getIndex(x, offset)].pos.y;

			offset = glm::max((x - 1), 0);
			north = this->vertices[getIndex(offset, z)].pos.y;

			offset = glm::min((x + 1), this->height - 1);
			south = this->vertices[getIndex(offset, z)].pos.y;

			this->vertices[getIndex(x, z)].nor = glm::normalize(glm::vec3(west - east, 2.f, south - north));
		}
	}
}

float ym::Terrain::barryCentricHeight(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec2 xz)
{
	float det = (v2.z - v3.z) * (v1.x - v3.x) - (v2.x - v3.x) * (v1.z - v3.z);
	float l1 = abs(((v2.z - v3.z) * (xz.x - v3.x) - (v2.x - v3.x) * (xz.y - v3.z)) / det);
	float l2 = abs(((v1.z - v3.z) * (xz.x - v3.x) - (v1.x - v3.x) * (xz.y - v3.z)) / det);
	float l3 = 1.0f - l1 - l2;
	return l1 * v1.y + l2 * v2.y + l3 * v3.y;
}
