#pragma once

#include "Utils.hpp"
#include <functional>

namespace OSSLHasher
{

using HashFunc = std::function<void(ucharVectorPtr, ucharVectorPtr)>;

enum class HashType: unsigned char
{
    UNKNOWN = 0,
    SHA1,
    SHA256,
    BLAKE512,
};

void Hash(HashType type, ucharVectorPtr plain, ucharVectorPtr hash);

size_t GetHashSize(HashType type);
HashFunc GetHashFunc(HashType type);
std::string GetHashFuncName(HashType type);
HashType GetHashTypeFromString(const std::string& type);

void SHA1(ucharVectorPtr plain, ucharVectorPtr hash);
void SHA256(ucharVectorPtr plain, ucharVectorPtr hash);
void BLAKE512(ucharVectorPtr plain, ucharVectorPtr hash);

} // namespace Blake
