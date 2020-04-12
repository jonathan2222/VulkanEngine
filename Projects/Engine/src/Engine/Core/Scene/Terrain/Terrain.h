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
			int32_t proximityRadius;
			glm::vec3 origin;
		};

	public:
		void init(uint32_t dataWidth, uint32_t dataHeight, uint8_t* data, Description description);
		void destroy();

		float getHeightAt(const glm::vec3& pos) const;
		glm::ivec2 getRegionFromPos(const glm::vec3& pos) const;
		void getProximityVertices(const glm::vec3& pos, std::vector<Vertex>& vertices) const;

		uint32_t getProximityIndiciesSize();
		std::vector<uint32_t> generateIndicies();

		uint64_t getUniqueID() const;

	private:
		void generateVertices(uint32_t dataWidth, uint32_t dataHeight, uint8_t* data);
		void generateNormals();

		static float barryCentricHeight(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec2 xz);

	private:
		uint64_t uniqueId;

		Description desc;
		std::vector<Vertex> vertices;

		int32_t proxVertDim;
		int32_t indiciesPerRegion;
		int32_t regionSize;
		int32_t regionCount;
		int32_t regionWidthCount;

		int32_t width;
		int32_t height;
	};
}