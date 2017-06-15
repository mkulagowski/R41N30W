#pragma once

#include "Utils.hpp"


namespace Salted {

void Reduction(const int salt, const size_t resultLength, const ucharVectorPtr& hashValue, ucharVectorPtr& plainValue);

} // namespace Salted
