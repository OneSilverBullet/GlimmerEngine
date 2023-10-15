
#include <chrono>
#include <ctime>
#include <ratio>
#include <chrono>


class EngineTimer
{
public:
	EngineTimer(){
		m_start = std::chrono::high_resolution_clock::now();
	}

	double TotalTime() {
		std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(currentTime - m_start);
		return time_span.count();
	}

private:
	std::chrono::high_resolution_clock::time_point m_start; // 
};



