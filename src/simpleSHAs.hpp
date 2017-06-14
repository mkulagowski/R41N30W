#pragma once

#include "Utils.hpp" 

extern const int SHA256_OUT_LEN;
bool simpleSHA256(ucharVectorPtr plainValue, ucharVectorPtr hashValue);

extern const int SHA1_OUT_LEN;
bool simpleSHA1(ucharVectorPtr plainValue, ucharVectorPtr hashValue);