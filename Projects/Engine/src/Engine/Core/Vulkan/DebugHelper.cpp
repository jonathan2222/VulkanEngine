#include "stdafx.h"
#include "DebugHelper.h"

std::unordered_map<uint64_t, std::string> ym::NameBinder::s_objectMap;

void ym::NameBinder::bindObject(uint64_t handle, const std::string& name)
{
	s_objectMap[handle] = name;
}

std::string ym::NameBinder::getName(uint64_t handle)
{
	std::string name("UNKNOWN_OBJECT_HANDLE");
	auto it = s_objectMap.find(handle);
	if (it != s_objectMap.end()) name = it->second;
	return name;
}

void ym::NameBinder::printContents()
{
	for (auto elem : s_objectMap)
	{
		YM_LOG_CRITICAL("{0:x} => {1}", elem.first, elem.second.c_str());
	}
}
