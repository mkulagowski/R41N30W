#include "OSSLHasher.hpp"

#include <openssl/ssl.h>
#include <iostream>


namespace OSSLHasher {

namespace {

void destroyCtx(EVP_MD_CTX* ctx)
{
    if (ctx != nullptr)
        EVP_MD_CTX_free(ctx);
}

using MDCtxPtr = std::unique_ptr<EVP_MD_CTX, std::function<void(EVP_MD_CTX*)>>;
using MDFunc = std::function<const EVP_MD*()>;

MDFunc SelectMD(HashType type)
{
    switch (type)
    {
    case HashType::SHA1: return EVP_sha1;
    case HashType::SHA256: return EVP_sha256;
    case HashType::BLAKE512: return EVP_blake2b512;
    default:
        std::cout << "Unsupported hash type" << std::endl;
        return MDFunc();
    }
}

} // anonymous namespace


void Hash(HashType type, ucharVectorPtr plain, ucharVectorPtr hash)
{
    MDFunc mdFunc = SelectMD(type);
    if (hash->size() < GetHashSize(type))
    {
        std::cout << "Not enough space to input " << GetHashFuncName(type).c_str() << " hash - needed " << GetHashSize(type) << std::endl;
        return;
    }

    MDCtxPtr ctx(EVP_MD_CTX_new(), destroyCtx);
    if (!ctx)
    {
        std::cout << "Failed to create MD context" << std::endl;
        return;
    }

    if (!EVP_DigestInit(ctx.get(), mdFunc()))
    {
        std::cout << "Failed to initialize MD context to BLAKE 512 digest" << std::endl;
        return;
    }

    if (!EVP_DigestUpdate(ctx.get(), plain->data(), plain->size()))
    {
        std::cout << "Failed to update MD digest from data" << std::endl;
        return;
    }

    if (!EVP_DigestFinal(ctx.get(), hash->data(), nullptr))
    {
        std::cout << "Failed to finalize MD digest" << std::endl;
        return;
    }
}

size_t GetHashSize(HashType type)
{
    return static_cast<size_t>(EVP_MD_size(SelectMD(type)()));
}

HashFunc GetHashFunc(HashType type)
{
    switch (type)
    {
    case HashType::SHA1: return SHA1;
    case HashType::SHA256: return SHA256;
    case HashType::BLAKE512: return BLAKE512;
    default:
        std::cout << "Unsupported hash function" << std::endl;
        return HashFunc();
    }
}

std::string GetHashFuncName(HashType type)
{
    switch (type)
    {
    case HashType::SHA1: return "SHA1";
    case HashType::SHA256: return "SHA256";
    case HashType::BLAKE512: return "BLAKE512";
    default:
        std::cout << "Unsupported hash function" << std::endl;
        return "";
    }
}

HashType GetHashTypeFromString(const std::string& type)
{
    if (type == "SHA1") return HashType::SHA1;
    if (type == "SHA256") return HashType::SHA256;
    if (type == "BLAKE512") return HashType::BLAKE512;
    return HashType::BLAKE512; // defaults to BLAKE512
}

void SHA1(ucharVectorPtr plain, ucharVectorPtr hash)
{
    Hash(HashType::SHA1, plain, hash);
}

void SHA256(ucharVectorPtr plain, ucharVectorPtr hash)
{
    Hash(HashType::SHA256, plain, hash);
}

void BLAKE512(ucharVectorPtr plain, ucharVectorPtr hash)
{
    Hash(HashType::BLAKE512, plain, hash);
}

} // namespace OSSLHasher
