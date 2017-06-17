#pragma once

#include <unordered_set>
#include <map>
#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include "Utils.hpp"
#include "OSSLHasher.hpp"
#include "Reduction.hpp"


class RainbowTable
{
public:
    RainbowTable(size_t startSize, uint32_t passwordLength, int chainSteps, OSSLHasher::HashType hashType);
    ~RainbowTable();

    void SetThreadCount(uint32_t threadCount);
    void SetTextMode(bool textMode);

    bool CreateTable();
    void GeneratePasswords(unsigned int limit);
    int GetSize() { return static_cast<int>(mDictionary.size()); }

    std::string FindPassword(const std::string& hashedPassword);

    void Save(const std::string& filename);
    bool Load(const std::string& filename);
    void SavePasswords(const std::string& filename);
    void LoadPasswords(const std::string& filename);

private:
    void CreateRows(unsigned int limit, unsigned int thread);
    void CreateRowsFromPass(unsigned int limit, unsigned int index);
    bool RunChain(std::string password, unsigned int salt);

    void LogTableInfo();
    void LogProgress(unsigned int current, unsigned int step, unsigned int limit);

    std::string FindPasswordInChain(const ucharVector& startingHashedPassword, const ucharVector& hashedPassword);
    std::string FindPasswordInChainParallel(const ucharVector& startingHashedPassword, int startIndex);

    std::string GetRandomPassword(size_t length);

    bool LoadText(std::ifstream& file);
    bool LoadBinary(std::ifstream& file);
    void SaveText(const std::string& filename);
    void SaveBinary(const std::string& filename);

    Reduction::ReductionFunc mReductionFunc;
    OSSLHasher::HashFunc mHashFunc;
    OSSLHasher::HashType mHashType;
    uint32_t mHashLen;

    std::map<ucharVector, std::string> mDictionary;
    std::unordered_set<std::string> mOriginalPasswords;
    uint32_t mThreadCount;
    bool mTextMode; // whether to save table to text
    uint64_t mVerticalSize;
    uint32_t mChainSteps;
    uint32_t mPasswordLength;

    std::mutex mDictionaryMutex;
    std::mutex mPasswordMutex;

    uint64_t mStartTime;
    uint64_t mFreq;
};
