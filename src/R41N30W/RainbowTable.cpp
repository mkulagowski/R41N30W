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

const std::string RAINBOW_MAGIC_TEXT_FILE = "RTXT"; // Rainbow TeXT
const std::string RAINBOW_MAGIC_BINARY_FILE = "RBIN"; // Rainbow BINary


RainbowTable::RainbowTable(size_t startSize, uint32_t passwordLength, int chainSteps, OSSLHasher::HashType hashType)
    : mChainSteps(chainSteps)
    , mThreadCount(1)
    , mVerticalSize(startSize)
    , mPasswordLength(passwordLength)
    , mHashFunc(OSSLHasher::GetHashFunc(hashType))
    , mHashType(hashType)
    , mHashLen(static_cast<uint32_t>(OSSLHasher::GetHashSize(hashType)))
{
    mReductionFunc = Reduction::Salted;
    mFreq = GetClockFreq();
}


RainbowTable::~RainbowTable()
{
}

void RainbowTable::SetThreadCount(uint32_t threadCount)
{
    if (threadCount > hardwareConcurrency())
    {
        threadCount = hardwareConcurrency();
        std::cout << "Requested more threads than system can handle - reducing to " << threadCount << std::endl;
    }

    mThreadCount = threadCount;
}

void RainbowTable::SetTextMode(bool textMode)
{
    mTextMode = textMode;
}

void RainbowTable::CreateTable()
{
    std::cout << "Threads used: " << mThreadCount << std::endl;
    std::cout << "Creating Rainbow Table with parameters:" << std::endl;
    LogTableInfo();

    const unsigned int limit = static_cast<unsigned int>(mVerticalSize / mThreadCount);
    std::vector<std::future<void>> createRowsResults;
    createRowsResults.reserve(mThreadCount);
    mDictionary.reserve(mVerticalSize);
    mOriginalPasswords.reserve(mVerticalSize);
    mStartTime = GetTime();

    if (mOriginalPasswords.empty())
    {
        for (unsigned int i = 0; i < mThreadCount; ++i)
            createRowsResults.push_back(std::async(std::launch::async, &RainbowTable::CreateRows, this, limit, i));
    }
    else
    {
        for (unsigned int i = 0; i < mThreadCount; ++i)
            createRowsResults.push_back(std::async(std::launch::async, &RainbowTable::CreateRowsFromPass, this, limit, i));
    }

    for (auto &i : createRowsResults)
        if (!i.valid())
            return;

    for (auto &i : createRowsResults)
        i.wait();

    uint64_t stop = GetTime();
    uint64_t diff = static_cast<uint64_t>(static_cast<double>(stop - mStartTime) / static_cast<double>(mFreq));
    std::cout << std::endl << "Table with " << mDictionary.size() << " entries built in ";
    PrettyLogTime(diff);
    std::cout << std::endl;
}

void RainbowTable::LogTableInfo()
{
    std::cout << "\tHash function:\t\t" << OSSLHasher::GetHashFuncName(mHashType) << std::endl;
    std::cout << "\tTable size:\t\t" << mVerticalSize << std::endl;
    std::cout << "\tChain steps:\t\t" << mChainSteps << std::endl;
    std::cout << "\tPassword length:\t" << mPasswordLength << std::endl;
}

void RainbowTable::LogProgress(unsigned int current, unsigned int step, unsigned int limit)
{
    if (current == 0)
    {
        // for first step, do not log anything
        return;
    }

    if ((current % step) == 0)
    {
        uint64_t stop = GetTime();
        double diff = static_cast<double>(stop - mStartTime) / static_cast<double>(mFreq);
        double progress = (static_cast<double>(current) / static_cast<double>(limit)) * 100.0;

        uint64_t diffSeconds = static_cast<uint64_t>(diff);
        uint64_t etaSeconds = (static_cast<uint64_t>(diff * limit) / current) - diffSeconds;

        std::cout << "Progress: " << current << "/" << limit << " ["
                  << std::setw(6) << std::setprecision(4) << std::fixed << progress << "% done] Elapsed ";
        PrettyLogTime(diffSeconds);
        std::cout << " Remaining ";
        PrettyLogTime(etaSeconds);
        std::cout << "        \r";
    }
}

void RainbowTable::CreateRows(unsigned int limit, unsigned int thread)
{
    std::string password;
    unsigned int lastIndex = thread * limit;
    for (unsigned int i = 0; i < limit; ++i)
    {
        if (thread == 0)
            LogProgress(i, 200, limit);
        int counter = 10;
        do {
            std::lock_guard<std::mutex> lock(mPasswordMutex);
            password = GetRandomPassword(mPasswordLength);
            while (!mOriginalPasswords.insert(password).second)
            {
                // generate passwords until we'll find a unique one
                password = GetRandomPassword(mPasswordLength);
            }
        } while (!RunChain(password, lastIndex - i) && --counter > 0);
    }
}

void RainbowTable::GeneratePasswords(unsigned int limit)
{
    std::string password;
    for (unsigned int i = 0; i < limit; ++i)
    {
        std::lock_guard<std::mutex> lock(mPasswordMutex);

        password = GetRandomPassword(mPasswordLength);
        while (!mOriginalPasswords.insert(password).second)
        {
            // generate passwords until we'll find a unique one
            password = GetRandomPassword(mPasswordLength);
        }
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
    {
        if (index == 0)
            LogProgress(counter, 200, limit);

        RunChain(*i, counter++);
    }
}

bool RainbowTable::RunChain(std::string password, unsigned int rowSalt)
{
    ucharVectorPtr hashValue = std::make_shared<ucharVector>();
    hashValue->resize(mHashLen);

    ucharVectorPtr plainValue = std::make_shared<ucharVector>();
    plainValue->reserve(mPasswordLength);
    plainValue->assign(password.begin(), password.end());

    mHashFunc(plainValue, hashValue);
    for (uint32_t i = 0; i < mChainSteps; ++i)
    {
        mReductionFunc(CantorPairing(rowSalt, i), mPasswordLength, hashValue, plainValue);
        mHashFunc(plainValue, hashValue);
    }

    std::string hash = HashToStr(hashValue);
    {
        std::lock_guard<std::mutex> lock(mDictionaryMutex);
        const auto res = mDictionary.insert(std::make_pair(hash, password));
        return res.second;
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
            mPasswordLength = static_cast<uint32_t>(mOriginalPasswords.begin()->size());
        std::cout << "Loaded " << static_cast<unsigned int>(mVerticalSize) << " passwords of length = " << mPasswordLength << ".\n";
        file.close();
    }
    else
    {
        std::cout << "Unable to open file \"" << filename << "\"!\n";
    }
}

void RainbowTable::SavePasswords(const std::string& filename)
{
    std::cout << "Saving passwords to file \"" << filename << "\"\n";
    std::string line1;
    std::ofstream file(filename);

    if (file)
    {
        std::lock_guard<std::mutex> lock(mPasswordMutex);

        for (const auto &pass : mOriginalPasswords)
        {
            file << pass << std::endl;
        }

        std::cout << "Saved " << static_cast<unsigned int>(mOriginalPasswords.size()) << " passwords of length = " << mPasswordLength << ".\n";
        file.close();
    }
    else
    {
        std::cout << "Unable to open file \"" << filename << "\"!\n";
    }
}

bool RainbowTable::LoadText(std::ifstream& file)
{
    std::string line1, line2;

    try
    {
        std::getline(file, line1); // flush the rest of the line with magic

        std::getline(file, line1);
        std::string hashFuncStr = line1;
        mHashType = OSSLHasher::GetHashTypeFromString(hashFuncStr);
        if (mHashType == OSSLHasher::HashType::UNKNOWN)
        {
            std::cout << "Unrecognized hash function type." << std::endl;
            return false;
        }

        mHashFunc = OSSLHasher::GetHashFunc(mHashType);

        std::getline(file, line1);
        mVerticalSize = std::stol(line1);

        std::getline(file, line1);
        mChainSteps = std::stoi(line1);

        std::getline(file, line1);
        mPasswordLength = std::stoi(line1);

        // 2 rows in file is 1 insertion into the dictionary
        uint32_t counter = 0;
        while (getline(file, line1) && getline(file, line2))
        {
            if (counter > 9999)
                LogProgress(counter, 10000, static_cast<unsigned int>(mVerticalSize));

            mDictionary[line1] = line2;
            counter++;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception caught while reading from file: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool RainbowTable::LoadBinary(std::ifstream& file)
{
    std::cout << "Loading binary files not yet implemented." << std::endl;
    return false;
}

bool RainbowTable::Load(const std::string& filename)
{
    std::cout << "Loading table from file \"" << filename << "\"\n";
    std::ifstream file(filename);
    mStartTime = GetTime();

    if (file)
    {
        std::lock_guard<std::mutex> lock(mDictionaryMutex);

        mDictionary.clear();

        // recognize file type and load appropriate
        char magic[5];
        file.read(magic, 4);
        magic[4] = 0;
        if (RAINBOW_MAGIC_TEXT_FILE.compare(0, 4, magic) == 0)
        {
            if (!LoadText(file))
                return false;
        }
        else if (RAINBOW_MAGIC_BINARY_FILE.compare(0, 4, magic) == 0)
        {
            if (!LoadBinary(file))
                return false;
        }
        else
        {
            std::cout << "Provided file is not a proper R41N30W table file." << std::endl;
            std::cout << "Generate one to be used with --generate option (see help for details)." << std::endl;
            return false;
        }

        if (mVerticalSize != mDictionary.size())
        {
            std::cout << "\nIncomplete table provided:" << std::endl;
            std::cout << "  Table has " << mDictionary.size() << " rows" << std::endl;
            std::cout << "  Should have " << mVerticalSize << " rows" << std::endl;
            return false;
        }

        if (mPasswordLength != mDictionary.begin()->second.size())
        {
            std::cout << "\nMalformed table provided - password lengths (declared vs actual) do not match." << std::endl;
            return false;
        }

        file.close();

        std::cout << "\nTable loaded:" << std::endl;
        LogTableInfo();
        return true;
    }
    else
    {
        std::cout << "Unable to open file \"" << filename << "\"!\n";
        return false;
    }
}

void RainbowTable::SaveText(const std::string& filename)
{
    std::ofstream file(filename);
    if (file)
    {
        std::lock_guard<std::mutex> lock(mDictionaryMutex);

        file.write(RAINBOW_MAGIC_TEXT_FILE.c_str(), 4); // to avoid writing the trailing zero from std string
        file << std::endl;
        file << OSSLHasher::GetHashFuncName(mHashType) << std::endl;
        file << mVerticalSize << std::endl;
        file << mChainSteps << std::endl;
        file << mPasswordLength << std::endl;
        for (const auto &row : mDictionary)
        {
            file << row.first << std::endl << row.second << std::endl;
        }

        file.close();
    }
}

void RainbowTable::SaveBinary(const std::string& filename)
{
    std::ofstream file(filename);
    if (file)
    {
        std::lock_guard<std::mutex> lock(mDictionaryMutex);

        /**
         * Binary file structure is similar to text structure:
         * Header:
         *   -> MAGIC (4 bytes)
         *   -> hash function ID (4 bytes)
         *   -> vertical size (8 bytes)
         *   -> horizontal size aka. chain steps (4 bytes)
         *   -> password length (4 bytes)
         * Data, for all vertical sizes:
         *   -> hash (size depends on hash function)
         *   -> password string (length depends on pwd length)
         */

        uint32_t hashID = static_cast<uint32_t>(mHashType);

        file.write(RAINBOW_MAGIC_BINARY_FILE.c_str(), RAINBOW_MAGIC_BINARY_FILE.length()); // magic
        file.write(reinterpret_cast<const char*>(&hashID), sizeof(hashID)); // hash
        file.write(reinterpret_cast<const char*>(&mVerticalSize), sizeof(mVerticalSize)); // vert size
        file.write(reinterpret_cast<const char*>(&mChainSteps), sizeof(mChainSteps)); // horizontal size
        file.write(reinterpret_cast<const char*>(&mPasswordLength), sizeof(mPasswordLength)); // horizontal size

        for (const auto& row: mDictionary)
        {
            // TODO
        }

        file.close();
    }
}

void RainbowTable::Save(const std::string& filename)
{
    if (GetSize() <= 0)
        return;
    std::cout << "Saving table to file \"" << filename << "\"\n";

    if (mTextMode)
        SaveText(filename);
    else
        SaveBinary(filename);

    std::cout << "Saved table:" << std::endl;
    LogTableInfo();
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
        std::vector<std::future<std::string>> asyncFindPassResults;
        asyncFindPassResults.reserve(mThreadCount);
        for (unsigned int i = 0; i < mThreadCount; ++i)
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

    for (uint32_t i = 0; i < mChainSteps; ++i)
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

std::string RainbowTable::FindPasswordInChainParallel(const std::string& startingHashedPassword, int startIndex)
{
    ucharVectorPtr hashValue = std::make_shared<ucharVector>();
    hashValue->reserve(mHashLen);

    ucharVectorPtr plainValue = std::make_shared<ucharVector>();
    plainValue->reserve(mPasswordLength);

    ucharVectorPtr hashedPasswordValue = std::make_shared<ucharVector>();
    hashedPasswordValue->reserve(mHashLen);
    StrToHash(startingHashedPassword, hashedPasswordValue);

    for (int i = startIndex; i >= 0; i -= mThreadCount)
    {
        hashValue->assign(hashedPasswordValue->begin(), hashedPasswordValue->end());

        for (uint32_t y = i; y < mChainSteps; y++)
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
