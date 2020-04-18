#pragma once

#include <string>
#include <unordered_map>

namespace ym
{
	class NameBinder
	{
	public:
		static void bindObject(uint64_t handle, const std::string& name);

		static std::string getName(uint64_t handle);

		static void printContents();

	private:
		static std::unordered_map<uint64_t, std::string> s_objectMap;
	};
}