#include "AdrianReduction.hpp"
#include "Common.hpp"


namespace Adrian {

void Reduction(const int salt, const size_t resultLength, const ucharVectorPtr& hashValue, ucharVectorPtr& plainValue)
{
    // clear() leaves capacity unchanged - no need to reserve
    plainValue->clear();

    for (size_t i = 0; i < resultLength; i++)
    {
        unsigned int index = (*hashValue)[i] % Common::CharsetLength;
        plainValue->push_back(Common::Charset[index]);
    }
}

} // namespace Adrian
