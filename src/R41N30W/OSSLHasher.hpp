#pragma once

#include "Utils.hpp"
#include <functional>

namespace OSSLHasher
{

using HashFunc = std::function<void(const ucharVector&, ucharVector&)>;

enum class HashType: unsigned char
{
    UNKNOWN = 0,
    SHA1,
    SHA256,
    BLAKE512,
};

void Hash(HashType type, const ucharVector& plain, ucharVector& hash);

size_t GetHashSize(HashType type);
HashFunc GetHashFunc(HashType type);
std::string GetHashFuncName(HashType type);
HashType GetHashTypeFromString(const std::string& type);

void SHA1(const ucharVector& plain, ucharVector& hash);
void SHA256(const ucharVector& plain, ucharVector& hash);
void BLAKE512(const ucharVector& plain, ucharVector& hash);

} // namespace Blake
