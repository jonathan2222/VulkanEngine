#include "stdafx.h"
#include "Instrumentation.h"

// Code from: https://github.com/TheCherno/Hazel

bool ym::Instrumentation::g_runRenderingProfiling = false;

ym::Instrumentation::Instrumentation() : m_counter(0)
{
}

ym::Instrumentation::~Instrumentation()
{
}

ym::Instrumentation& ym::Instrumentation::get()
{
	static Instrumentation instrumentation;
	return instrumentation;
}

void ym::Instrumentation::beginSession(const std::string& name, const std::string& filePath)
{
	m_counter = 0;
	m_file.open(filePath);
	m_file << "{\"otherData\": {}, \"displayTimeUnit\": \"ms\", \"traceEvents\": [";
	m_file.flush();
}

void ym::Instrumentation::write(ProfileData data)
{
	if (m_counter++ > 0) m_file << ",";

	std::string name = data.name;
	std::replace(name.begin(), name.end(), '"', '\'');

	m_file << "{";
	m_file << "\"name\": \"" << name << "\",";
	m_file << "\"cat\": \"function\",";
	m_file << "\"ph\": \"X\",";
	m_file << "\"pid\": 0,";
	m_file << "\"tid\": 0,";
	m_file << "\"ts\": " << data.start << ",";
	m_file << "\"dur\": " << (data.end - data.start);
	m_file << "}";

	m_file.flush();
}

void ym::Instrumentation::endSession()
{
	m_file << "]" << std::endl << "}";
	m_file.close();
}
