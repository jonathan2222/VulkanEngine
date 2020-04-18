#pragma once

#include "../Vertex.h"
#include <vector>

namespace ym
{
	class Terrain
	{
	public:
		struct Description
		{
			float minZ;
			float maxZ;
			float vertDist;
			glm::vec3 origin;
		};

		struct ProximityDescription
		{
			int32_t regionSize;
			int32_t proximityRadius;
			// Can be calculated from the two above.
			int32_t proximityWidth;
			int32_t indiciesPerRegion;
			// Need to know the dataWidth to calculate these.
			int32_t regionCount;
			int32_t regionWidthCount;
		};

	public:
		void init(uint32_t dataWidth, uint32_t dataHeight, uint8_t* data, Description description);
		void destroy();

		float getHeightAt(const glm::vec3& pos) const;
		glm::ivec2 getRegionFromPos(const glm::vec3& pos, int32_t regionSize) const;

		void fetchVertices(int32_t proximityWidth, const glm::vec3& pos, std::vector<Vertex>& verticesOut) const;

		uint64_t getUniqueID() const;
		VkDescriptorSet& getDescriptorSet();
		int32_t getRegionCount() const;
		glm::vec3 getOrigin() const;

		static Terrain::ProximityDescription createProximityDescription(int32_t dataWidth, int32_t regionSize, int32_t proximityRadius);

	private:
		void generateVertices(uint32_t dataWidth, uint32_t dataHeight, uint8_t* data);
		void generateNormals();

		static float barryCentricHeight(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec2 xz);

	private:
		uint64_t uniqueId;

		Description desc;
		std::vector<Vertex> vertices;
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
		VkDescriptorBufferInfo bufferInfo;
		ProximityDescription proximityDescription;

		int32_t width;
		int32_t height;
	};
}