#pragma once

#include "Utils.hpp"


namespace Adrian {

void Reduction(const int salt, const size_t resultLength, const ucharVectorPtr& hashValue, ucharVectorPtr& plainValue);

} // namespace Adrian
