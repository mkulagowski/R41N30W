#include <iostream>
#include <stdlib.h>
#include <string>
#include "OSSLHasher.hpp"
#include "Utils.hpp"
#include "ArgParser.hpp"


const char* desc = "Description:\n\
Hasher is a simple tool utilizing mechanics used in R41N30W in order to quickly\n\
digest simple messages. The tool was made to help debugging issues with R41N30W.\n\
\n\
To hash something, specify the hash type (default is BLAKE512) and an input\n\
message to be digested. Result will be printed to stdout in hex form.\n\
";

int main(int argc, char * argv[])
{
    ArgParser parser;

    parser.Add("t,type", "Type of hash to be used (SHA1, SHA256, BLAKE512)", ArgType::STRING, "BLAKE512")
          .Add("i,input", "Input string to be hashed", ArgType::STRING)
          .Add("h,help", "Display this message", ArgType::FLAG);

    if (!parser.Parse(argc, argv))
    {
        parser.PrintUsage();
        return 1;
    }

    if (parser.GetFlag('h'))
    {
        std::cout << "Hasher - Simple hash tool" << std::endl;
        std::cout << "Made by mkk13 & LK, 2017, uses OpenSSL" << std::endl << std::endl;
        parser.PrintUsage();
        std::cout << desc << std::endl;
        return 0;
    }

    std::string input = parser.GetString('i');
    if (input.empty())
    {
        std::cout << "Please specify an input string to be hashed." << std::endl;
        return 1;
    }

    ucharVector plainValue;
    plainValue.assign(input.begin(), input.end());

    ucharVector hashValue;
    OSSLHasher::HashType type = OSSLHasher::GetHashTypeFromString(parser.GetString('t'));
    if (type == OSSLHasher::HashType::UNKNOWN)
    {
        std::cout << "Incorrect hash type provided. Supported are:" << std::endl;
        std::cout << "\tSHA1, SHA256, BLAKE512" << std::endl;
        return 2;
    }

    hashValue.resize(OSSLHasher::GetHashSize(type));

    OSSLHasher::Hash(type, plainValue, hashValue);

    std::string hash = HashToStr(hashValue);
    std::cout << hash << "\n";
    return 0;
}
