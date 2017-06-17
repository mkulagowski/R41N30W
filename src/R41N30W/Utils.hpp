#pragma once

#include <vector>
#include <memory>

using ucharVector = std::vector<unsigned char>;

unsigned int hardwareConcurrency();
void StrToHash(const std::string& hashString, ucharVector& hashValue);
std::string HashToStr(ucharVector hashValue);

uint64_t GetTime();
uint64_t GetClockFreq();
void PrettyLogTime(uint64_t timeSeconds);

unsigned int CantorPairing(const unsigned int x, const unsigned int y);