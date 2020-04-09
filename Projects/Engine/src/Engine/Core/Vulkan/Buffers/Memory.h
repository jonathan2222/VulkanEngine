#pragma once

#include "stdafx.h"

typedef uint64_t Offset;

namespace ym
{
	class Buffer;
	struct Texture;

	class Memory
	{
	public:
		Memory();
		~Memory();

		void init(VkMemoryPropertyFlags memProp);
		void destroy();

		void bindBuffer(Buffer* buffer);
		void bindTexture(Texture* texture);
		void directTransfer(Buffer* buffer, const void* data, uint64_t size, Offset bufferOffset);

		VkDeviceMemory getMemory();

	private:
		VkDeviceMemory memory;
		std::unordered_map<Buffer*, Offset> bufferOffsets;
		std::unordered_map<Texture*, Offset> textureOffsets;
		Offset currentOffset;
	};
}