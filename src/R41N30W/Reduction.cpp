#include "Reduction.hpp"
#include "Common.hpp"


namespace Reduction {

void Adrian(const int salt, const size_t resultLength, const ucharVectorPtr& hashValue, ucharVectorPtr& plainValue)
{
    // clear() leaves capacity unchanged - no need to reserve
    plainValue->clear();

    for (size_t i = 0; i < resultLength; i++)
    {
        unsigned int index = (*hashValue)[i] % Common::CharsetLength;
        plainValue->push_back(Common::Charset[index]);
    }
}

void Salted(const int salt, const size_t resultLength, const ucharVectorPtr& hashValue, ucharVectorPtr& plainValue)
{
    // clear() leaves capacity unchanged - no need to reserve
    plainValue->clear();

    for (size_t i = 0; i < resultLength; i++)
    {
        unsigned int index = (*hashValue)[i] + (*hashValue)[i +      resultLength ]
                                             + (*hashValue)[i + (2 * resultLength)]
                                             + (*hashValue)[i + (3 * resultLength)]
                                             + (*hashValue)[i + (4 * resultLength)] + salt;
        plainValue->push_back(Common::Charset[index % Common::CharsetLength]);
    }
}

}