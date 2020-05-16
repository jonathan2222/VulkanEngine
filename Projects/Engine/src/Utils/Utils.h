#pragma once

#include <string>

namespace ym
{
	class Utils
	{
	public:
		static std::string getNameFromPath(const std::string& path)
		{
			std::string name = path;
			const size_t last_slash_idx = name.find_last_of("\\/");
			if (std::string::npos != last_slash_idx)
				name.erase(0, last_slash_idx + 1);
			return name;
		}
	};
}