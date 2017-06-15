#include <iostream>
#include <stdlib.h>
#include <string>
#include "OSSLHasher.hpp"
#include "Utils.hpp"


int main(int argc, char * argv[])
{
    if (argc != 2)
    {
        std::cout << "Wrong number of arguments - 1 needed!\n";
        return -1;
    }

    std::string plain(argv[1]);
    ucharVectorPtr plainValue(new std::vector<unsigned char>());
    plainValue->assign(plain.begin(), plain.end());

    ucharVectorPtr hashValue(new std::vector<unsigned char>());
    hashValue->resize(OSSLHasher::GetHashSize(OSSLHasher::HashType::BLAKE512));

    OSSLHasher::BLAKE512(plainValue, hashValue);

    std::string hash = HashToStr(hashValue);
    std::cout << "\n" << hash << "\n";
    return 0;
}
