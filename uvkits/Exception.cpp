#include "Exception.h"

namespace ndcp {

Exception::Exception(const std::string &message) : message_(message) {}

const char *Exception::what() const throw()
{
    return message_.c_str();
}


} // namespace ndcp
