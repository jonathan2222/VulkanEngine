#pragma once

// Code from: https://www.youtube.com/watch?v=xlAH4dbMVnU

#include <string>
#include <fstream>

namespace ym
{
	class Instrumentation
	{
	public:
		struct ProfileData
		{
			std::string name;
			long long start, end;
		};
		Instrumentation();
		~Instrumentation();

		static Instrumentation& get();

		void beginSession(const std::string& name, const std::string& filePath = "result.json");

		void write(ProfileData data);

		void endSession();


		static bool g_runRenderingProfiling;

	private:
		std::ofstream m_file;
		unsigned long long m_counter;
	};
}