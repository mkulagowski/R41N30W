#include <iostream>
#include <stdlib.h>
#include <string>
#include <random>
#include <algorithm>
#include <limits>
#include <cstring>
#include <cstdlib>
#include <iterator>
#include <fstream>
#include <future>
#include <iomanip>
#include "Common.hpp"
#include "RainbowTable.hpp"
#include "Reduction.hpp"


RainbowTable::RainbowTable(size_t startSize, size_t passwordLength, int chainSteps, OSSLHasher::HashType hashType)
    : mChainSteps(chainSteps)
    , mVerticalSize(startSize)
    , mPasswordLength(passwordLength)
    , mHashFunc(OSSLHasher::GetHashFunc(hashType))
    , mHashFunctionName(OSSLHasher::GetHashFuncName(hashType))
    , mHashLen(static_cast<int>(OSSLHasher::GetHashSize(hashType)))
{
    mReductionFunc = Reduction::Salted;
}

void RainbowTable::CreateTable()
{
    std::cout << "Creating Table for:\n\tVertical size = " << mVerticalSize << "\n\tHorizontal size = " << mChainSteps << "\n\tPassword length = " << mPasswordLength << std::endl;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    const unsigned int threadsNo = hardwareConcurrency();
    const unsigned int limit = static_cast<unsigned int>(mVerticalSize / threadsNo);
    std::vector<std::future<void>> createRowsResults;
    createRowsResults.reserve(threadsNo);

    if (mOriginalPasswords.empty())
    {
        for (unsigned int i = 0; i < threadsNo; ++i)
            createRowsResults.push_back(std::async(std::launch::async, &RainbowTable::CreateRows, this, limit, i));
    }
    else
    {
        for (unsigned int i = 0; i < threadsNo; ++i)
            createRowsResults.push_back(std::async(std::launch::async, &RainbowTable::CreateRowsFromPass, this, limit, i));
    }

    for (auto &i : createRowsResults)
        i.get();

    std::cout << "Table of size = " << GetSize() << " built in " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() << " [s]\n";
}

void RainbowTable::CreateRows(unsigned int limit, unsigned int thread)
{
    std::string password;
    for (unsigned int i = 0; i < limit; ++i)
    {
        if (thread == 0)
        {
            if ((i % 10) == 0)
            {
                unsigned int perThreadLimit = limit / hardwareConcurrency();
                double progress = static_cast<double>(i) / static_cast<double>(perThreadLimit) * 100.0;
                std::cout << "Progress: " << i << "/" << perThreadLimit << " ["
                     << std::setw(6) << std::setprecision(4) << std::fixed << progress << "% done]\r";
            }
        }

        {
            std::lock_guard<std::mutex> lock(mPasswordMutex);

            password = GetRandomPassword(mPasswordLength);
            while (!mOriginalPasswords.insert(password).second)
            {
                // generate passwords until we'll find a unique one
                password = GetRandomPassword(mPasswordLength);
            }
        }

        RunChain(password, i);
    }
}

void RainbowTable::CreateRowsFromPass(unsigned int limit, unsigned int index)
{
    auto begin = mOriginalPasswords.begin();
    std::advance(begin, index * limit);
    auto end = mOriginalPasswords.begin();
    std::advance(end, (index + 1) * limit);
    unsigned int counter = 0;

    for (auto i = begin; i != end; ++i)
        RunChain(*i, counter++);
}

void RainbowTable::RunChain(std::string password, int salt)
{
    ucharVectorPtr hashValue(new std::vector<unsigned char>());
    hashValue->resize(mHashLen);

    ucharVectorPtr plainValue(new std::vector<unsigned char>());
    plainValue->reserve(mPasswordLength);
    plainValue->assign(password.begin(), password.end());

    mHashFunc(plainValue, hashValue);
    for (int i = 0; i < mChainSteps; ++i)
    {
        mReductionFunc(i, mPasswordLength, hashValue, plainValue);
        mHashFunc(plainValue, hashValue);
    }

    std::string hash = HashToStr(hashValue);
    {
        std::lock_guard<std::mutex> lock(mDictionaryMutex);
        mDictionary.insert(std::make_pair(hash, password));
    }
}

void RainbowTable::LoadPasswords(const std::string& filename)
{
    std::cout << "Loading passwords from file \"" << filename << "\"\n";
    std::string line1;
    std::ifstream file(filename);

    if (file)
    {
        std::lock_guard<std::mutex> lock(mPasswordMutex);

        mOriginalPasswords.clear();
        while (getline(file, line1))
        {
            mOriginalPasswords.insert(line1);
        }
        mVerticalSize = mOriginalPasswords.size();
        if (mVerticalSize > 0)
            mPasswordLength = mOriginalPasswords.begin()->size();
        std::cout << "Loaded " << static_cast<unsigned int>(mVerticalSize) << " passwords of length = " << mPasswordLength << ".\n";
        file.close();
    }
    else
    {
        std::cout << "Unable to open file \"" << filename << "\"!\n";
    }
}

void RainbowTable::Load(const std::string& filename)
{
    std::cout << "Loading table from file \"" << filename << "\"\n";
    std::string line1, line2;
    std::ifstream file(filename);

    if (file)
    {
        std::lock_guard<std::mutex> lock(mDictionaryMutex);

        mDictionary.clear();
        std::getline(file, line1);
        std::cout << "\t>>Hash function:\t" << line1;

        std::getline(file, line1);
        std::cout << "\t>>Table size:\t" << line1;
        mVerticalSize = std::stoi(line1);

        std::getline(file, line1);
        std::cout << "\t>>Chain steps:\t" << line1;
        mChainSteps = std::stoi(line1);

        std::getline(file, line1);
        std::cout << "\t>>Password length:\t" << line1;
        mPasswordLength = std::stoi(line1);
        
        // 2 rows in file is 1 insertion into the dictionary
        while (getline(file, line1) && getline(file, line2))
        {
            mDictionary[line1] = line2;
        }
        mVerticalSize = mDictionary.size();
        if (mVerticalSize > 0)
            mPasswordLength = static_cast<int>(mDictionary.begin()->second.size());
        std::cout << "\t>>\n\t>>Table loaded.\n";
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
    std::cout << "Saving table to file \"" << filename << "\"\n";

    std::ofstream file;
    file.open(filename);
    if (file)
    {
        std::lock_guard<std::mutex> lock(mDictionaryMutex);

        file << mHashFunctionName << std::endl;
        file << mVerticalSize << std::endl;
        file << mChainSteps << std::endl;
        file << mPasswordLength << std::endl;
        for (const auto &row : mDictionary)
        {
            file << row.first << std::endl << row.second << std::endl;
        }
        std::cout << "Saved table of size = " << static_cast<unsigned int>(mVerticalSize) << ", chain length = " << mChainSteps << " & password length = " << mPasswordLength
            << ". Hash function " << mHashFunctionName << "used.\n";
        file.close();
    }
}

std::string RainbowTable::FindPassword(const std::string& hashedPassword)
{
    if (mDictionary.size() <= 0)
        return "";

    int hashLength = static_cast<int>(mDictionary.begin()->first.size());
    if (hashedPassword.size() != hashLength)
    {
        std::cout << "Hash length mismatch! Hashed passwords in the table are " << hashLength << " characters long.\n";
        return "";
    }

    ucharVectorPtr hashValue = std::make_shared<ucharVector>();
    hashValue->resize(mHashLen);

    ucharVectorPtr plainValue = std::make_shared<ucharVector>();
    plainValue->reserve(mPasswordLength);

    if (mDictionary.count(hashedPassword) > 0)
    {
        // then the right chain is found
        // the position of the pass word is in that chain, step i
        return FindPasswordInChain(hashedPassword, hashedPassword);
    }
    else
    {
        ucharVectorPtr hashedPasswordValue = std::make_shared<ucharVector>();
        hashedPasswordValue->reserve(mHashLen);
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

    ucharVectorPtr hashValue = std::make_shared<ucharVector>();
    hashValue->resize(mHashLen);

    ucharVectorPtr plainValue = std::make_shared<ucharVector>();
    plainValue->reserve(startPlain.length());
    plainValue->assign(startPlain.begin(), startPlain.end());

    for (int i = 0; i < mChainSteps; ++i)
    {
        mHashFunc(plainValue, hashValue);

        std::string currentHashedPassword = HashToStr(hashValue);

        if (currentHashedPassword == startingHashedPassword)
        {
            // found password = prehashvalue
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

    int hashLength = static_cast<int>(mDictionary.begin()->first.size());
    if (hashedPassword.size() != hashLength)
    {
        std::cout << "Hash length mismatch! Hashed passwords in the table are " << hashLength << " characters long.\n";
        return "";
    }

    ucharVectorPtr hashValue = std::make_shared<ucharVector>();
    hashValue->resize(mHashLen);

    ucharVectorPtr plainValue = std::make_shared<ucharVector>();
    plainValue->reserve(mPasswordLength);

    if (mDictionary.count(hashedPassword) > 0)
    {
        // then the right chain is found
        // the position of the pass word is in that chain, step i
        return FindPasswordInChain(hashedPassword, hashedPassword);
    }
    else
    {
        std::vector<std::future<std::string>> asyncFindPassResults;
        asyncFindPassResults.reserve(hardwareConcurrency());
        for (unsigned int i = 0; i < hardwareConcurrency(); ++i)
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
    ucharVectorPtr hashValue = std::make_shared<ucharVector>();
    hashValue->reserve(mHashLen);

    ucharVectorPtr plainValue = std::make_shared<ucharVector>();
    plainValue->reserve(mPasswordLength);

    ucharVectorPtr hashedPasswordValue = std::make_shared<ucharVector>();
    hashedPasswordValue->reserve(mHashLen);
    StrToHash(startingHashedPassword, hashedPasswordValue);

    for (int i = startIndex; i >= 0; i -= hardwareConcurrency())
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

auto Rand()
{
    static std::random_device rd;
    static std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
    static std::uniform_int_distribution<int> uni(0, Common::CharsetLength - 1);
    return uni(rng);
}

std::string RainbowTable::GetRandomPassword(size_t length)
{
    std::string result;
    std::generate_n(std::back_inserter(result), length, [&](){ return Common::Charset[Rand()]; });
    return result;
}
