#pragma once

#include <unordered_set>
#include <map>
#include <functional>
#include <vector>
#include <memory>
#include "Utils.hpp"

class RainbowTable
{
public:
    static const char mCharset[65];
	static const unsigned int mCharsetLength;

    using reductionFunc = std::function<void(int, int, ucharVectorPtr&, ucharVectorPtr&)>;
    using hashFunc = std::function<void(ucharVectorPtr, ucharVectorPtr)>;

    RainbowTable(double StartSize, int Password_Length, int Chain_steps);
    ~RainbowTable();

    void CreateTable();
    int GetSize() { return static_cast<int>(mDictionary.size()); }

    std::string FindPassword(const std::string& hashedPassword);
    std::string FindPasswordParallel(const std::string& hashedPassword);

    void Save(const std::string& filename);
    void Load(const std::string& filename);
    void LoadPasswords(const std::string& filename);
private:
    void CreateRows(unsigned int limit);
    void CreateRowsFromPass(unsigned int limit, unsigned int index);
    void RunChain(std::string password, int salt);

    std::string FindPasswordInChain(const std::string& startingHashedPassword, const std::string& hashedPassword);
    std::string FindPasswordInChainParallel(const std::string& startingHashedPassword, int startIndex);

    std::string GetRandomPassword(size_t length);

    static void ReductionFunction(const int salt, const int resultLength, const ucharVectorPtr& hashValue, ucharVectorPtr& plainValue);
    static void BlakeHash(ucharVectorPtr plainValue, ucharVectorPtr hashValue);

    reductionFunc mReductionFunc;
    hashFunc mHashFunc;
	const std::string mHashFunctionName;
    const int mHashLen;
    std::map<std::string, std::string> mDictionary;
    std::unordered_set<std::string> mOriginalPasswords;
    double mVerticalSize;
    int mChainSteps;
    int mPasswordLength;
};