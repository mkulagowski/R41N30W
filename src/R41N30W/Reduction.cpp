#include "Reduction.hpp"
#include "Common.hpp"


namespace Reduction {

void Adrian(const unsigned int salt, const size_t resultLength, const ucharVector& hashValue, ucharVector& plainValue)
{
    // clear() leaves capacity unchanged - no need to reserve
    plainValue.clear();

    for (size_t i = 0; i < resultLength; i++)
    {
        unsigned int index = hashValue[i] % Common::CharsetLength;
        plainValue.push_back(Common::Charset[index]);
    }
}

void Salted(const unsigned int salt, const size_t resultLength, const ucharVector& hashValue, ucharVector& plainValue)
{
    // clear() leaves capacity unchanged - no need to reserve
    plainValue.clear();

    for (size_t i = 0; i < resultLength; i++)
    {
        unsigned int index = hashValue[i] + hashValue[(i +      resultLength ) % hashValue.size()]
                                          + hashValue[(i + (2 * resultLength)) % hashValue.size()]
                                          + hashValue[(i + (3 * resultLength)) % hashValue.size()]
                                          + hashValue[(i + (4 * resultLength)) % hashValue.size()] + salt;
        plainValue.push_back(Common::Charset[index % Common::CharsetLength]);
    }
}

}
