#ifndef NEXUSPOOL_DATABASE_UTILS_HPP
#define NEXUSPOOL_DATABASE_UTILS_HPP

#include <memory>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace nexuspool {
namespace database {

inline const std::string time_to_string(time_t nTime)
{
	struct tm newtime;
	auto t = std::time(&nTime);
	localtime_s(&newtime, &t);

	std::ostringstream oss;
	oss << std::put_time(&newtime, "%Y-%m-%d %H-%M-%S");
	return oss.str();
}

// Get current date/time, format is YYYY-MM-DD HH:mm:ss
inline std::string const current_date_time() 
{
	time_to_string(time_t(0));
}

	
} // namespace database
} // namespace nexuspool

#endif
