#include <thread>
#include <string>
#include <iterator>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "Utils.hpp"
#include <Windows.h>

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

std::string HashToStr(ucharVector hashValue)
{
    std::stringstream hash("");
    hash << std::hex << std::setfill('0');

    for (const auto& i : hashValue)
        hash << std::setw(2) << static_cast<int>(i);
    return hash.str();
}

void StrToHash(const std::string& hashString, ucharVector& hashValue)
{
    for (size_t i = 0; i < hashString.size(); i += 2)
    {
        std::istringstream hexStream(hashString.substr(i, 2));
        int x;
        hexStream >> std::hex >> x;
        hashValue.push_back(static_cast<unsigned char>(x));
    }
}

uint64_t GetTime()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return time.QuadPart;
}

uint64_t GetClockFreq()
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return freq.QuadPart;
}

void PrettyLogTime(uint64_t timeSeconds)
{
    uint64_t timeMinutes = timeSeconds / 60;
    timeSeconds %= 60;
    uint64_t timeHours = timeMinutes / 60;
    timeMinutes %= 60;

    std::cout << std::setw(2) << std::setfill('0') << timeHours << ":"
              << std::setw(2) << std::setfill('0') << timeMinutes << ":"
              << std::setw(2) << std::setfill('0') << timeSeconds;
}

unsigned int CantorPairing(const unsigned int x, const unsigned int y)
{
    unsigned int result = ((x + y) * (x + y + 1)) >> 1;
    return result + y;
}