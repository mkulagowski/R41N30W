#pragma once

#include "Utils.hpp"


namespace Reduction {

void Adrian(const int salt, const size_t resultLength, const ucharVectorPtr& hashValue, ucharVectorPtr& plainValue);
void Salted(const int salt, const size_t resultLength, const ucharVectorPtr& hashValue, ucharVectorPtr& plainValue);

} // namespace Reduction
