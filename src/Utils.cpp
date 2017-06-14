#include <thread>
#include <string>
#include <iterator>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "Utils.hpp"

unsigned int unixHardwareConcurrency()
{
    std::ifstream cpuinfo("/proc/cpuinfo");

    return static_cast<unsigned int>(std::count(std::istream_iterator<std::string>(cpuinfo),
                                                std::istream_iterator<std::string>(),
                                                std::string("processor")));
}

unsigned int myHardwareConcurrency()
{
    unsigned int cores = std::thread::hardware_concurrency();
    cores = cores ? cores : unixHardwareConcurrency();
    return cores ? cores : 1;
}

unsigned int hardwareConcurrency()
{
    static unsigned int cores = myHardwareConcurrency();
    return cores;
}

std::string HashToStr(ucharVectorPtr hashValue)
{
	std::stringstream hash("");
	hash << std::hex << std::setfill('0');

	for (const auto& i : *hashValue)
		hash << std::setw(2) << static_cast<int>(i);
	return hash.str();
}

void StrToHash(const std::string& hashString, ucharVectorPtr hashValue)
{
	for (size_t i = 0; i < hashString.size(); i += 2)
	{
		std::istringstream hexStream(hashString.substr(i, 2));
		int x;
		hexStream >> std::hex >> x;
		hashValue->push_back(static_cast<unsigned char>(x));
	}
}
