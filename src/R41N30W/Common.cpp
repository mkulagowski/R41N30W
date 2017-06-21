#include "Common.hpp"
#include <string.h>

namespace Common {

const char Charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._";
const unsigned int CharsetLength = static_cast<unsigned int>(strlen(Charset));

} // namespace Common
