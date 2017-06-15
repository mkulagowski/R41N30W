#pragma once

#include <vector>
#include <memory>

using ucharVector = std::vector<unsigned char>;
using ucharVectorPtr = std::shared_ptr<ucharVector>;

unsigned int hardwareConcurrency();
void StrToHash(const std::string& hashString, ucharVectorPtr hashValue);
std::string HashToStr(ucharVectorPtr hashValue);
