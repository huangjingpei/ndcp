#ifndef __NDCP_EXCEPTION_H__
#define __NDCP_EXCEPTION_H__
#include <exception>
#include <string>

namespace ndcp {


class Exception: public std::exception
{

public:
	Exception(const std::string &message);

    const char* what() const throw() override;

    const char* stackTrace() const noexcept
    {
      return stack_.c_str();
    }
private:
    std::string message_;
    std::string stack_;

};

} //namespace ndcp
#endif //__NDCP_EXCEPTION_H__
