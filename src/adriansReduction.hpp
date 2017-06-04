#pragma once

#include "RainbowTable.hpp"

void AdrianReduction(const int salt, const int resultLength, const RainbowTable::ucharVectorPtr& hashValue, RainbowTable::ucharVectorPtr& plainValue);
