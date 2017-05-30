#include <thread>
#include <string>
#include <iterator>
#include <fstream>
#include "Utils.hpp"

namespace Utils
{

auto unixHardwareConcurrency()
{
    std::ifstream cpuinfo("/proc/cpuinfo");

    return std::count(std::istream_iterator<std::string>(cpuinfo),
                      std::istream_iterator<std::string>(),
                      std::string("processor"));
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
}