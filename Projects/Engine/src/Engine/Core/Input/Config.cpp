#include "stdafx.h"
#include "Config.h"

#include <fstream>

#include <iostream>

ym::Config* ym::Config::get()
{
	static Config config;
	return &config;
}

void ym::Config::init(const std::string& fileName)
{
	YM_PROFILER_FUNCTION();
	readFile(fileName);
}

void ym::Config::print()
{
	for (auto e : m_map)
	{
		std::string type;
		switch (e.second.type)
		{
		case STRING: type = std::string(e.second.str); break;
		case INT: type = std::to_string(e.second.i); break;
		case FLOAT: type = std::to_string(e.second.f); break;
		case BOOL:
		default: type = e.second.b ? "True" : "False"; break;
		}
		YM_LOG_INFO("[{0}] -> [{1}]", e.first.c_str(), type);
	}
}

void ym::Config::readFile(const std::string& fileName)
{
	YM_PROFILER_FUNCTION();
	std::ifstream file(fileName);

	json j;

	if (file.is_open())
	{
		file >> j;
		file.close();
		YM_LOG_SUCCESS("Loaded engine config!");

		load("", j);
	}
	else
	{
		YM_LOG_CRITICAL("Failed to load engine config!");
	}
}

#pragma warning( push )
#pragma warning( disable : 26444 )
void ym::Config::load(const std::string& str, json& j)
{
	YM_PROFILER_FUNCTION();
	for (auto it = j.begin(); it != j.end(); it++) {
		std::string newKey = str + (str.empty() ? "" : "/") + it.key();
		if (it->is_object())
			load(newKey, *it);
		else
		{
			ConfigData data = {};
			if (it->is_boolean())
			{
				data.b = (bool)it.value();
				data.type = ConfigType::BOOL;
			}
			if (it->is_number_integer())
			{
				data.i = (int)it.value();
				data.type = ConfigType::INT;
			}
			if (it->is_number_float())
			{
				data.f = (float)it.value();
				data.type = ConfigType::FLOAT;
			}
			if (it->is_string())
			{
				std::string str = it.value();
				memcpy(data.str, str.c_str(), 256);
				data.type = ConfigType::STRING;
			}
			m_map[newKey] = data;
		}
	}
}
#pragma warning( pop )
