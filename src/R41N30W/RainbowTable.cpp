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

bool RainbowTable::CreateTable()
{
    std::cout << "Threads used: " << mThreadCount << std::endl;

    uint32_t sizeMod = mVerticalSize % mThreadCount;
    if (sizeMod != 0)
    {
        mVerticalSize -= sizeMod;
        std::cout << "Table size is not a multiple of thread count." << std::endl;
        std::cout << "To make our life easier, table size is reduced to " << mVerticalSize << std::endl;
    }

    std::cout << "Creating Rainbow Table with parameters:" << std::endl;
    LogTableInfo();

    if (mVerticalSize > std::numeric_limits<uint32_t>::max())
    {
        std::cout << "Cannot create " << mVerticalSize << " Rainbow Table on 32-bit compilation." << std::endl;
        std::cout << "Please use 64-bit build for big Rainbow Tables." << std::endl;
        return false;
    }

    const unsigned int limit = static_cast<unsigned int>(mVerticalSize / mThreadCount);
    std::vector<std::future<void>> createRowsResults;
    createRowsResults.reserve(mThreadCount);
    mOriginalPasswords.reserve(static_cast<size_t>(mVerticalSize));
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
        {
            std::cout << "Invalid worker threads dispatch." << std::endl;
            return false;
        }

    for (auto &i : createRowsResults)
        i.wait();

    uint64_t stop = GetTime();
    uint64_t diff = static_cast<uint64_t>(static_cast<double>(stop - mStartTime) / static_cast<double>(mFreq));
    std::cout << std::endl << "Table with " << mDictionary.size() << " entries built in ";
    PrettyLogTime(diff);
    std::cout << std::endl;

    return true;
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
    ucharVector hashValue;
    hashValue.resize(mHashLen);

    ucharVector plainValue;
    plainValue.reserve(mPasswordLength);
    plainValue.assign(password.begin(), password.end());

    mHashFunc(plainValue, hashValue);
    for (uint32_t i = 0; i < mChainSteps; ++i)
    {
        mReductionFunc(CantorPairing(rowSalt, i), mPasswordLength, hashValue, plainValue);
        mHashFunc(plainValue, hashValue);
    }

    {
        std::lock_guard<std::mutex> lock(mDictionaryMutex);
        const auto res = mDictionary.insert(std::make_pair(hashValue, password));
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

bool RainbowTable::LoadText(const std::string& filename)
{
    std::ifstream file(filename);

    if (file)
    {
        std::string line1, line2;
        try
        {
            std::getline(file, line1); // dummy getline to pass the line with magic value

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
            ucharVector hash;
            hash.reserve(OSSLHasher::GetHashSize(mHashType));
            while (getline(file, line1) && getline(file, line2))
            {
                LogProgress(counter, 10000, static_cast<unsigned int>(mVerticalSize));
                StrToHash(line1, hash);
                mDictionary[hash] = line2;
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

    return false;
}

bool RainbowTable::LoadBinary(const std::string& filename)
{
    /**
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

    std::ifstream file(filename, std::ifstream::binary);

    if (file)
    {
        uint32_t hashID = 0;
        file.read(reinterpret_cast<char*>(&hashID), sizeof(hashID)); // dummy read to pass the magic value
        file.read(reinterpret_cast<char*>(&hashID), sizeof(hashID)); // hash id
        file.read(reinterpret_cast<char*>(&mVerticalSize), sizeof(mVerticalSize)); // vert size
        file.read(reinterpret_cast<char*>(&mChainSteps), sizeof(mChainSteps)); // horizontal size
        file.read(reinterpret_cast<char*>(&mPasswordLength), sizeof(mPasswordLength)); // pwd len

        mHashType = static_cast<OSSLHasher::HashType>(hashID);
        mHashLen = static_cast<uint32_t>(OSSLHasher::GetHashSize(mHashType));

        // check how much data awaits for us
        std::streampos curPos = file.tellg();
        file.seekg(0, std::ios_base::end);
        uint64_t dataSize = static_cast<uint64_t>(file.tellg() - curPos);
        file.seekg(curPos, std::ios_base::beg);

        // calculate how much data we want to read
        // datasize should be (passwordlength + size(hash)) * verticalsize
        uint64_t expectedSize = (mPasswordLength + mHashLen) * mVerticalSize;
        if (expectedSize != dataSize)
        {
            std::cout << "Incomplete file provided (difference of " << expectedSize - dataSize << " compared to expected size)" << std::endl;
            return false;
        }

        ucharVector hashBuffer;
        hashBuffer.resize(mHashLen);
        ucharVector passwordBuffer;
        passwordBuffer.resize(mPasswordLength);
        std::string passwordString;
        passwordString.resize(mPasswordLength);

        for (uint64_t i = 0; i < mVerticalSize; ++i)
        {
            file.read(reinterpret_cast<char*>(hashBuffer.data()), mHashLen);
            file.read(reinterpret_cast<char*>(passwordBuffer.data()), mPasswordLength);

            // rewrite the password from buffer to string
            for (uint32_t j = 0; j < mPasswordLength; ++j)
                passwordString[j] = passwordBuffer[j];
            mDictionary[hashBuffer] = passwordString;
        }

        return true;
    }

    return false;
}

bool RainbowTable::Load(const std::string& filename)
{
    std::cout << "Loading table from file \"" << filename << "\"\n";
    std::ifstream file(filename, std::ifstream::binary);
    mStartTime = GetTime();

    if (file)
    {
        std::lock_guard<std::mutex> lock(mDictionaryMutex);

        mDictionary.clear();

        // recognize file type and load appropriate
        char magic[5];
        file.read(magic, 4);
        magic[4] = 0;
        file.close(); // we will reopen the file in specific loaders, when we determine the type

        if (RAINBOW_MAGIC_TEXT_FILE.compare(0, 4, magic) == 0)
        {
            if (!LoadText(filename))
                return false;
        }
        else if (RAINBOW_MAGIC_BINARY_FILE.compare(0, 4, magic) == 0)
        {
            if (!LoadBinary(filename))
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
            file << HashToStr(row.first) << std::endl << row.second << std::endl;
        }

        file.close();
    }
}

void RainbowTable::SaveBinary(const std::string& filename)
{
    std::ofstream file(filename, std::ofstream::binary);

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

        size_t hashSize = OSSLHasher::GetHashSize(mHashType);
        for (const auto& row: mDictionary)
        {
            file.write(reinterpret_cast<const char*>(row.first.data()), hashSize);
            file.write(reinterpret_cast<const char*>(row.second.data()), mPasswordLength);
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

    // hash in string form takes two chars for each byte
    if (hashedPassword.size() != (mHashLen * 2))
    {
        std::cout << "Hash length mismatch! Hashed passwords in the table are " << mHashLen*2 << " chars long (" << mHashLen << " bytes)" << std::endl;
        std::cout << "and provided hash is " << hashedPassword.size() << " chars long (" << hashedPassword.size() / 2 << " bytes)" << std::endl;
        return "";
    }

    ucharVector hashValue;
    hashValue.reserve(mHashLen);
    StrToHash(hashedPassword, hashValue);

    ucharVector plainValue;
    plainValue.reserve(mPasswordLength);

    if (mDictionary.count(hashValue) > 0)
    {
        // then the right chain is found
        // the position of the password is in that chain, step i
        return FindPasswordInChain(hashValue, hashValue);
    }
    else
    {
        std::vector<std::future<std::string>> asyncFindPassResults;
        asyncFindPassResults.reserve(mThreadCount);
        for (unsigned int i = 0; i < mThreadCount; ++i)
        {
            asyncFindPassResults.push_back(std::async(std::launch::async, &RainbowTable::FindPasswordInChainParallel, this, hashValue, mChainSteps - 1 - i));
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

std::string RainbowTable::FindPasswordInChain(const ucharVector& startingHashedPassword, const ucharVector& hashedPassword)
{
    std::string startPlain = mDictionary[hashedPassword];

    ucharVector hashValue;
    hashValue.resize(mHashLen);

    ucharVector plainValue;
    plainValue.reserve(startPlain.length());
    plainValue.assign(startPlain.begin(), startPlain.end());

    for (uint32_t i = 0; i < mChainSteps; ++i)
    {
        mHashFunc(plainValue, hashValue);

        if (hashValue == startingHashedPassword)
        {
            // found password = prehashvalue
            std::string password;
            password.reserve(plainValue.size());
            for (const auto& i : plainValue)
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

std::string RainbowTable::FindPasswordInChainParallel(const ucharVector& startingHashedPassword, int startIndex)
{
    ucharVector hashValue;
    hashValue.reserve(mHashLen);

    ucharVector plainValue;
    plainValue.reserve(mPasswordLength);

    for (int i = startIndex; i >= 0; i -= mThreadCount)
    {
        for (uint32_t y = i; y < mChainSteps; y++)
        {
            mReductionFunc(y, mPasswordLength, hashValue, plainValue);
            mHashFunc(plainValue, hashValue);
        }

        if (mDictionary.count(hashValue) > 0)
        {
            return FindPasswordInChain(startingHashedPassword, hashValue);
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
