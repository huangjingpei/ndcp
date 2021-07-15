#ifndef __STRING_BUILDER_H__
#define __STRING_BUILDER_H__
#include <cstdio>
#include <string>
#include <stddef.h>
#include <limits>
#include <utility>

namespace tuya {


template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
#define arraysize(array) (sizeof(ArraySizeHelper(array)))

#ifndef NDEBUG
#define CHECK_LE(x, y)
#else
#define CHECK_LE(x, y)
#endif

class StringBuilder {
 public:
  StringBuilder() {}
  explicit StringBuilder(std::string s) : str_(s) {}

  StringBuilder(const StringBuilder&) = delete;
  StringBuilder& operator=(const StringBuilder&) = delete;

  StringBuilder& operator<<(const std::string str) {
    str_.append(str.data(), str.length());
    return *this;
  }

  StringBuilder& operator<<(char c) = delete;

  StringBuilder& operator<<(int i) {
    str_ += ToString(i);
    return *this;
  }

  StringBuilder& operator<<(unsigned i) {
    str_ += ToString(i);
    return *this;
  }

  StringBuilder& operator<<(long i) {  // NOLINT
    str_ += ToString(i);
    return *this;
  }

  StringBuilder& operator<<(long long i) {  // NOLINT
    str_ += ToString(i);
    return *this;
  }

  StringBuilder& operator<<(unsigned long i) {  // NOLINT
    str_ += ToString(i);
    return *this;
  }

  StringBuilder& operator<<(unsigned long long i) {  // NOLINT
    str_ += ToString(i);
    return *this;
  }

  StringBuilder& operator<<(float f) {
    str_ += ToString((double)f);
    return *this;
  }

  StringBuilder& operator<<(double f) {
    str_ += ToString(f);
    return *this;
  }

  StringBuilder& operator<<(long double f) {
    str_ += ToString(f);
    return *this;
  }


  const std::string& str() const { return str_; }

  void Clear() { str_.clear(); }

  size_t size() const { return str_.size(); }

  std::string Release() {
    std::string ret = std::move(str_);
    str_.clear();
    return ret;
  }


  // Allows appending a printf style formatted string.
  StringBuilder& AppendFormat(const char* fmt, ...)
#if defined(__GNUC__)
      __attribute__((__format__(__printf__, 2, 3)))
#endif
      ;


  inline std::string ToString(const char* const s) {
    return std::string(s);
  }
  inline std::string ToString(const std::string s) {
    return s;
  }

  inline std::string ToString(const short s) {
    char buf[32];
    const int len = std::snprintf(&buf[0], arraysize(buf), "%hd", s);
    CHECK_LE(len, arraysize(buf));
    return std::string(&buf[0], len);
  }
  inline std::string ToString(const unsigned short s) {
    char buf[32];
    const int len = std::snprintf(&buf[0], arraysize(buf), "%hu", s);
    CHECK_LE(len, arraysize(buf));
    return std::string(&buf[0], len);
  }
  inline std::string ToString(const int s) {
    char buf[32];
    const int len = std::snprintf(&buf[0], arraysize(buf), "%d", s);
    CHECK_LE(len, arraysize(buf));
    return std::string(&buf[0], len);
  }
  inline std::string ToString(const unsigned int s) {
    char buf[32];
    const int len = std::snprintf(&buf[0], arraysize(buf), "%u", s);
    CHECK_LE(len, arraysize(buf));
    return std::string(&buf[0], len);
  }
  inline std::string ToString(const long int s) {
    char buf[32];
    const int len = std::snprintf(&buf[0], arraysize(buf), "%ld", s);
    CHECK_LE(len, arraysize(buf));
    return std::string(&buf[0], len);
  }
  inline std::string ToString(const unsigned long int s) {
    char buf[32];
    const int len = std::snprintf(&buf[0], arraysize(buf), "%lu", s);
    CHECK_LE(len, arraysize(buf));
    return std::string(&buf[0], len);
  }
  inline std::string ToString(const long long int s) {
    char buf[32];
    const int len = std::snprintf(&buf[0], arraysize(buf), "%lld", s);
    CHECK_LE(len, arraysize(buf));
    return std::string(&buf[0], len);
  }
  inline std::string ToString(const unsigned long long int s) {
    char buf[32];
    const int len = std::snprintf(&buf[0], arraysize(buf), "%llu", s);
    CHECK_LE(len, arraysize(buf));
    return std::string(&buf[0], len);
  }

  inline std::string ToString(const double d) {
    char buf[32];
    const int len = std::snprintf(&buf[0], arraysize(buf), "%g", d);
    CHECK_LE(len, arraysize(buf));
    return std::string(&buf[0], len);
  }

  inline std::string ToString(const long double d) {
    char buf[32];
    const int len = std::snprintf(&buf[0], arraysize(buf), "%Lg", d);
    CHECK_LE(len, arraysize(buf));
    return std::string(&buf[0], len);
  }

  inline std::string ToString(const void* const p) {
    char buf[32];
    const int len = std::snprintf(&buf[0], arraysize(buf), "%p", p);
    CHECK_LE(len, arraysize(buf));
    return std::string(&buf[0], len);
  }


 private:
  std::string str_;
};
} // namespace tuya

#endif
