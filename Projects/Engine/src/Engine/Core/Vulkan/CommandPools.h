#pragma once

#include "CommandPool.h"

namespace ym
{
	struct CommandPools
	{
		CommandPool graphicsPool;
		CommandPool transferPool;
		CommandPool computePool;

		void init()
		{
			this->graphicsPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
			this->transferPool.init(CommandPool::Queue::TRANSFER, 0);
			this->computePool.init(CommandPool::Queue::COMPUTE, 0);
		}

		void destroy()
		{
			this->graphicsPool.destroy();
			this->transferPool.destroy();
			this->computePool.destroy();
		}
	};

}
