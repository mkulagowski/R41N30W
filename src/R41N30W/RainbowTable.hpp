#pragma once

#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include "Utils.hpp"
#include "OSSLHasher.hpp"


class RainbowTable
{
public:
    using ReductionFunc = std::function<void(int, size_t, ucharVectorPtr&, ucharVectorPtr&)>;

    RainbowTable(size_t startSize, size_t passwordLength, int chainSteps, OSSLHasher::HashType hashType);
    ~RainbowTable();

    void CreateTable();
    int GetSize() { return static_cast<int>(mDictionary.size()); }

    std::string FindPassword(const std::string& hashedPassword);
    std::string FindPasswordParallel(const std::string& hashedPassword);

    void Save(const std::string& filename);
    void Load(const std::string& filename);
    void LoadPasswords(const std::string& filename);

private:
    void CreateRows(unsigned int limit, unsigned int thread);
    void CreateRowsFromPass(unsigned int limit, unsigned int index);
    void RunChain(std::string password, int salt);

    std::string FindPasswordInChain(const std::string& startingHashedPassword, const std::string& hashedPassword);
    std::string FindPasswordInChainParallel(const std::string& startingHashedPassword, int startIndex);

    std::string GetRandomPassword(size_t length);

    ReductionFunc mReductionFunc;
    OSSLHasher::HashFunc mHashFunc;
    const std::string mHashFunctionName;
    const int mHashLen;

    std::unordered_map<std::string, std::string> mDictionary;
    std::unordered_set<std::string> mOriginalPasswords;
    size_t mVerticalSize;
    int mChainSteps;
    size_t mPasswordLength;

    std::mutex mDictionaryMutex;
    std::mutex mPasswordMutex;
};
