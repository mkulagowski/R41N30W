#include "adriansReduction.hpp"
#include <iostream>

static const char mCharset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._";
static const unsigned int mCharsetLength = 64;

void AdrianReduction(const int salt, const int resultLength, const RainbowTable::ucharVectorPtr& hashValue, RainbowTable::ucharVectorPtr& plainValue)
{
    // clear() leaves capacity unchanged - no need to reserve
    plainValue->clear();

    for (int i = 0; i < resultLength; i++)
    {
        unsigned int index = (*hashValue)[i] % mCharsetLength;
        if (mCharset[index] == ' ')
            std::cout << "printed ' ' for: salt=" << salt << ", resLen= " << resultLength << ", idx= " << i << ", indexVal= " << index << ", hashValue = " << (*hashValue)[i] << ", char = " << mCharset[index];
        plainValue->push_back(mCharset[index]);
    }
}