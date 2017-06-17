#pragma once

#include "Utils.hpp"
#include <functional>


namespace Reduction {

using ReductionFunc = std::function<void(const unsigned int, const size_t, const ucharVector&, ucharVector&)>;

void Adrian(const unsigned int salt, const size_t resultLength, const ucharVector& hashValue, ucharVector& plainValue);
void Salted(const unsigned int salt, const size_t resultLength, const ucharVector& hashValue, ucharVector& plainValue);

} // namespace Reduction
