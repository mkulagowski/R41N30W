#pragma once

#include "Utils.hpp"
#include <functional>

namespace OSSLHasher
{

using HashFunc = std::function<void(ucharVectorPtr, ucharVectorPtr)>;

enum class HashType: unsigned char
{
    SHA1 = 0,
    SHA256,
    BLAKE512,
};

void Hash(HashType type, ucharVectorPtr plain, ucharVectorPtr hash);

size_t GetHashSize(HashType type);
HashFunc GetHashFunc(HashType type);
std::string GetHashFuncName(HashType type);

void SHA1(ucharVectorPtr plain, ucharVectorPtr hash);
void SHA256(ucharVectorPtr plain, ucharVectorPtr hash);
void BLAKE512(ucharVectorPtr plain, ucharVectorPtr hash);

} // namespace Blake
