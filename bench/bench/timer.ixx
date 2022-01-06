
export module timer;

#pragma warning(disable:5050)
import std.core;

using namespace std::chrono;

export class timer
{
public:

	timer(std::string name)
		: m_name(name)
		, m_start(steady_clock::now())
	{
	}

	~timer()
	{
		auto duration = duration_cast<milliseconds>(steady_clock::now() - m_start);
		std::cout << std::format("{}: {}\n", m_name, duration);
	}

private:
	std::string m_name;
	steady_clock::time_point m_start;
};