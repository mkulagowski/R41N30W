#pragma once

#include <vector>
#include <memory>

using ucharVector = std::vector<unsigned char>;
using ucharVectorPtr = std::shared_ptr<ucharVector>;

unsigned int hardwareConcurrency();
void StrToHash(const std::string& hashString, ucharVectorPtr hashValue);
std::string HashToStr(ucharVectorPtr hashValue);

uint64_t GetTime();
uint64_t GetClockFreq();
void PrettyLogTime(uint64_t timeSeconds);

unsigned int CantorPairing(const unsigned int x, const unsigned int y);