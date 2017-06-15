#include "adriansReduction.hpp"
#include <iostream>

static const char mCharset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._";
static const unsigned int mCharsetLength = 64;

void AdrianReduction(const int salt, const size_t resultLength, const ucharVectorPtr& hashValue, ucharVectorPtr& plainValue)
{
    // clear() leaves capacity unchanged - no need to reserve
    plainValue->clear();

    for (size_t i = 0; i < resultLength; i++)
    {
        unsigned int index = (*hashValue)[i] % mCharsetLength;
        plainValue->push_back(mCharset[index]);
    }
}
