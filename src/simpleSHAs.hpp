#pragma once

#include "RainbowTable.hpp" 

extern const int SHA256_OUT_LEN;
bool simpleSHA256(RainbowTable::ucharVectorPtr plainValue, RainbowTable::ucharVectorPtr hashValue);

extern const int SHA1_OUT_LEN;
bool simpleSHA1(RainbowTable::ucharVectorPtr plainValue, RainbowTable::ucharVectorPtr hashValue);