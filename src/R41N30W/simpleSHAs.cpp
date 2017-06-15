#include "simpleSHAs.hpp"
#include <openssl/sha.h>

const int SHA256_OUT_LEN = SHA256_DIGEST_LENGTH;
bool simpleSHA256(ucharVectorPtr plainValue, ucharVectorPtr hashValue)
{
    SHA256_CTX ctx;
    if (!SHA256_Init(&ctx))
        return false;

    if (!SHA256_Update(&ctx, plainValue->data(), plainValue->size()))
        return false;

    if (!SHA256_Final(hashValue->data(), &ctx))
        return false;

    return true;
}

const int SHA1_OUT_LEN = SHA_DIGEST_LENGTH;
bool simpleSHA1(ucharVectorPtr plainValue, ucharVectorPtr hashValue)
{
    SHA_CTX ctx;
    if (!SHA1_Init(&ctx))
        return false;

    if (!SHA1_Update(&ctx, plainValue->data(), plainValue->size()))
        return false;

    if (!SHA1_Final(hashValue->data(), &ctx))
        return false;

    return true;
}