#pragma once

#pragma warning( push )
#pragma warning( disable : 26444 )
#pragma warning( disable : 26451 )
#pragma warning( disable : 28020 )
#include "../../../Utils/json.hpp"

using json = nlohmann::json;
#include "../../Defines.h"

namespace ym
{
	class Config
	{
	public:
		Config() = default;

		static Config* get();

		void init(const std::string& fileName);

		template<typename T>
		T fetch(const std::string& name);

		void print();

	private:
		enum ConfigType { STRING, INT, FLOAT, BOOL };
		struct ConfigData
		{
			ConfigType type;
			union
			{
				char str[256];
				int i;
				float f;
				bool b;
			};
		};

		void readFile(const std::string& fileName);
		void load(const std::string& str, json& j);

		std::unordered_map<std::string, ConfigData> m_map;

	};

	template<typename T>
	inline T Config::fetch(const std::string& name)
	{
		YM_ASSERT(false, "Cannot fetch \"{0}\" from config map, type \"{1}\" is not supported!", name.c_str(), typeid(T).name())
		return (T)0;
	}

	template<>
	inline int Config::fetch(const std::string& name)
	{
		std::unordered_map<std::string, ConfigData>::iterator& it = m_map.find(name);
		YM_ASSERT(it != m_map.end(), "Did not found \"{0}\" in config map!", name.c_str())
		YM_ASSERT(it->second.type == ConfigType::INT, "Config entry is not of type INT!")
		return it->second.i;
	}

	template<>
	inline float Config::fetch(const std::string& name)
	{
		std::unordered_map<std::string, ConfigData>::iterator& it = m_map.find(name);
		YM_ASSERT(it != m_map.end(), "Did not found \"{0}\" in config map!", name.c_str())
		YM_ASSERT(it->second.type == ConfigType::FLOAT, "Config entry is not of type INT!")
		return it->second.f;
	}

	template<>
	inline std::string Config::fetch(const std::string& name)
	{
		std::unordered_map<std::string, ConfigData>::iterator& it = m_map.find(name);
		YM_ASSERT(it != m_map.end(), "Did not found \"{0}\" in config map!", name.c_str())
		YM_ASSERT(it->second.type == ConfigType::STRING, "Config entry is not of type INT!")
		return std::string(it->second.str);
	}

	template<>
	inline bool Config::fetch(const std::string& name)
	{
		std::unordered_map<std::string, ConfigData>::iterator& it = m_map.find(name);
		YM_ASSERT(it != m_map.end(), "Did not found \"{0}\" in config map!", name.c_str())
		YM_ASSERT(it->second.type == ConfigType::BOOL, "Config entry is not of type INT!")
		return it->second.b;
	}
}

#pragma warning( pop )
