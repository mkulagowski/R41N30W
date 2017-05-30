/*
*  Project: Rainbow Table Generator
*  File:   RainbowTable.cpp
*  Author: Jason Papapanagiotakis
*  Github: https://github.com/JasonPap/Rainbow-Table-Generator
*/
#include <iostream>
#include <stdlib.h>
#include <string>
#include <random>
#include <algorithm>
#include <limits>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <future>
#include "RainbowTable.hpp"
#include "Utils.hpp"
#include "blake_ref.hpp"

const char RainbowTable::mCharset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@";

RainbowTable::RainbowTable(double startSize, size_t passwordLength, int chainSteps)
    : mChainSteps(chainSteps)
    , mVerticalSize(startSize)
    , mPasswordLength(passwordLength)
{
    mHashFunc = &RainbowTable::BlakeHash;
    mReductionFunc = &RainbowTable::ReductionFunction;
}

void RainbowTable::CreateTable()
{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    const unsigned int threadsNo = 1;// Utils::hardwareConcurrency();
    const unsigned int limit = mVerticalSize / threadsNo;
    std::vector<std::future<void>> createRowsResults;
    createRowsResults.reserve(threadsNo);

    for (int i = 0; i < threadsNo; i++)
        createRowsResults.push_back(std::async(std::launch::async, &RainbowTable::CreateRows, this, limit));

    for (auto &i : createRowsResults)
        i.wait();

    std::cout << "Dictionary size = " << GetSize() << std::endl;
    std::cout << "Building time = " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() << " [s]\n";
}

void RainbowTable::CreateRows(unsigned int limit)
{
    int counter;

    std::string password;
    for (unsigned int i = 0; i < limit; i++)
    {
        password = GetRandomPassword(mPasswordLength);
        {
            std::lock_guard<std::mutex> lock(mPasswordMutex);
            while (!mOriginalPasswords.insert(password).second)
            {
                // generuj hasło póki nie będzie oryginalne
                password = GetRandomPassword(mPasswordLength);
            }
        }
        RunChain(password, i);
    }
}

void RainbowTable::RunChain(std::string password, int salt)
{
    ucharVectorPtr hashValue(new std::vector<unsigned char>());
    hashValue->resize(64);

    ucharVectorPtr plainValue(new std::vector<unsigned char>());
    plainValue->reserve(mPasswordLength);
    plainValue->assign(password.begin(), password.end());

    mHashFunc(plainValue, hashValue);
    std::cout << password << " ==== " << HashToStr(hashValue) << std::endl;
    for (int i = 0; i < mChainSteps; i++)
    {
        mReductionFunc(i, mPasswordLength, hashValue, plainValue);
        mHashFunc(plainValue, hashValue);
    }

    std::string finalHash = HashToStr(hashValue);

    {
        std::lock_guard<std::mutex> lock(mDictionaryMutex);
        mDictionary.insert(std::make_pair(finalHash, password));
    }
}

void RainbowTable::BlakeHash(ucharVectorPtr plainValue, ucharVectorPtr hashValue)
{
    Hash(hashValue->size() * 8, plainValue->data(), plainValue->size() * 8, hashValue->data());
}

void RainbowTable::ReductionFunction(const int salt, const int resultLength, const ucharVectorPtr& hashValue, ucharVectorPtr& plainValue)
{
    // clear() leaves capacity unchanged - no need to reserve
    plainValue->clear();

    for (int i = 0; i < resultLength; i++)
    {
        unsigned int index = (*hashValue)[i] + (*hashValue)[i + resultLength]
            + (*hashValue)[i + (2 * resultLength)]
            + (*hashValue)[i + (3 * resultLength)]
            + (*hashValue)[i + (4 * resultLength)] + salt;
        plainValue->push_back(mCharset[index % mCharsetLength]);
    }
}

std::string RainbowTable::HashToStr(ucharVectorPtr hashValue)
{
    std::stringstream hash("");
    hash << std::hex << std::setfill('0');

    for (const auto& i : *hashValue)
        hash << std::setw(2) << static_cast<int>(i);
    return hash.str();
}

void RainbowTable::StrToHash(const std::string& hashString, ucharVectorPtr hashValue)
{
    for (size_t i = 0; i < hashString.size(); i += 2)
    {
        std::istringstream hexStream(hashString.substr(i, 2));
        unsigned char x;
        hexStream >> std::hex >> x;
        hashValue->push_back(x);
    }
}

void RainbowTable::Load(const std::string& filename)
{
    std::string line1, line2;
    std::ifstream file(filename);

    if (file)
    {
        std::lock_guard<std::mutex> lock(mDictionaryMutex);
        mDictionary.clear();

        // 2 rows in file is 1 insertion into the dictionary
        while (getline(file, line1) && getline(file, line2))
        {
            mDictionary[line1] = line2;
        }
        file.close();
    }
    else
    {
        std::cout << "Unable to open file \"" << filename << "\"!\n";
    }
}

void RainbowTable::Save(const std::string& filename)
{
    if (GetSize() <= 0)
        return;

    std::ofstream file;
    file.open(filename);
    if (file)
    {
        std::lock_guard<std::mutex> lock(mDictionaryMutex);
        for (const auto &row : mDictionary)
        {
            file << row.first << std::endl << row.second << std::endl;
        }
        file.close();
    }
}

std::string RainbowTable::FindPassword(const std::string& hashedPassword)
{
    if (mDictionary.size() <= 0)
        return "";

    int hashLength = mDictionary.begin()->first.size();
    if (hashedPassword.size() != hashLength)
    {
        std::cout << "Hash length mismatch! Hashed passwords in the table are " << hashLength << " characters long.\n";
        return "";
    }

    ucharVectorPtr hashValue(new std::vector<unsigned char>());
    hashValue->resize(64);

    ucharVectorPtr plainValue(new std::vector<unsigned char>());
    plainValue->reserve(mPasswordLength);

    if (mDictionary.count(hashedPassword) > 0)
    {
        //then the right chain is found
        //the position of the pass word is in that chain, step i
        return FindPasswordInChain(hashedPassword, hashedPassword);
    }
    else
    {
        ucharVectorPtr hashedPasswordValue(new std::vector<unsigned char>());
        hashedPasswordValue->reserve(64);
        StrToHash(hashedPassword, hashedPasswordValue);

        for (int i = mChainSteps - 1; i >= 0; i--)
        {
            hashValue->assign(hashedPasswordValue->begin(), hashedPasswordValue->end());
            for (int y = i; y < mChainSteps; y++)
            {
                mReductionFunc(y, mPasswordLength, hashValue, plainValue);
                mHashFunc(plainValue, hashValue);
            }

            std::string finalHash = HashToStr(hashValue);

            if (mDictionary.count(finalHash) > 0)
            {
                return FindPasswordInChain(hashedPassword, finalHash);
            }
        }
    }
    return "";
}


std::string RainbowTable::FindPasswordInChain(const std::string& startingHashedPassword, const std::string& hashedPassword)
{
    if (mDictionary.count(hashedPassword) <= 0)
        return "";

    std::string startPlain = mDictionary[hashedPassword];

    ucharVectorPtr hashValue(new std::vector<unsigned char>());
    hashValue->resize(64);

    ucharVectorPtr plainValue(new std::vector<unsigned char>());
    plainValue->reserve(startPlain.length());
    plainValue->assign(startPlain.begin(), startPlain.end());

    for (int i = 0; i <= mChainSteps; i++)
    {
        mHashFunc(plainValue, hashValue);

        std::string currentHashedPassword = HashToStr(hashValue);

        if (currentHashedPassword == startingHashedPassword)
        {//found password = prehashvalue
            std::string password;
            password.reserve(plainValue->size());
            for (const auto& i : *plainValue)
            {
                password += i;
            }
            return password;
        }
        else
        {
            mReductionFunc(i, mPasswordLength, hashValue, plainValue);
        }
    }
    return "";
}



RainbowTable::~RainbowTable()
{
}

std::string RainbowTable::FindPasswordParallel(const std::string& hashedPassword)
{
    if (mDictionary.size() <= 0)
        return "";

    int hashLength = mDictionary.begin()->first.size();
    if (hashedPassword.size() != hashLength)
    {
        std::cout << "Hash length mismatch! Hashed passwords in the table are " << hashLength << " characters long.\n";
        return "";
    }

    ucharVectorPtr hashValue(new std::vector<unsigned char>());
    hashValue->resize(64);

    ucharVectorPtr plainValue(new std::vector<unsigned char>());
    plainValue->reserve(mPasswordLength);

    if (mDictionary.count(hashedPassword) > 0)
    {
        //then the right chain is found
        //the position of the pass word is in that chain, step i
        return FindPasswordInChain(hashedPassword, hashedPassword);
    }
    else
    {
        std::vector<std::future<std::string>> asyncFindPassResults;
        asyncFindPassResults.reserve(Utils::hardwareConcurrency());
        for (int i = 0; i < Utils::hardwareConcurrency(); i++)
        {
            asyncFindPassResults.push_back(std::async(std::launch::async, &RainbowTable::FindPasswordInChainParallel, this, hashedPassword, mChainSteps - 1 - i));
        }

        std::string foundPassword;
        for (auto& i : asyncFindPassResults)
        {
            std::string result = i.get();
            if (!result.empty())
                foundPassword = result;
        }
        return foundPassword;
    }
}


std::string RainbowTable::FindPasswordInChainParallel(const std::string& startingHashedPassword, int startIndex)
{
    ucharVectorPtr hashValue(new std::vector<unsigned char>());
    hashValue->reserve(64);

    ucharVectorPtr plainValue(new std::vector<unsigned char>());
    plainValue->reserve(mPasswordLength);

    ucharVectorPtr hashedPasswordValue(new std::vector<unsigned char>());
    hashedPasswordValue->reserve(64);
    StrToHash(startingHashedPassword, hashedPasswordValue);

    for (int i = startIndex; i >= 0; i -= 8)
    {
        hashValue->assign(hashedPasswordValue->begin(), hashedPasswordValue->end());

        for (int y = i; y < mChainSteps; y++)
        {
            mReductionFunc(y, mPasswordLength, hashValue, plainValue);
            mHashFunc(plainValue, hashValue);
        }

        std::string finalHashedPassword = HashToStr(hashValue);

        if (mDictionary.count(finalHashedPassword) > 0)
        {
            return FindPasswordInChain(startingHashedPassword, finalHashedPassword);
        }
    }

    return "";
}


void RainbowTable::Print()
{
    for (const auto& row : mDictionary)
    {
        std::cout << "\n==" << row.second << "  ->  " << row.first << std::endl;
    }
}

auto Rand()
{
    static std::random_device rd;
    static std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
    static std::uniform_int_distribution<int> uni(0, RainbowTable::mCharsetLength);
    return uni(rng);
}

std::string RainbowTable::GetRandomPassword(size_t length)
{
    std::string result(length, 0);
    std::generate_n(result.begin(), length, [&](){ return mCharset[Rand()]; });
    return result;
}