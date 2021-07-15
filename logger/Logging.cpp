/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "Logging.h"
#include "ThreadTypes.h"

#include <string.h>
#include <mutex>
#include <time.h>


#if defined(WIN)
#include <windows.h>
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
#undef ERROR  // wingdi.h
#endif


#if defined(__POSIX__)
#include <sys/time.h>
#endif
#if defined(__APPLE__)
#include <sys/time.h>
#include <mach/mach_time.h>
#endif

#if defined(WEBRTC_WIN)
// clang-format off
// clang formatting would put <windows.h> last,
// which leads to compilation failure.
#include <windows.h>
#include <mmsystem.h>
#include <sys/timeb.h>
// clang-format on
#endif


#if defined(__APPLE__)
#include <CoreServices/CoreServices.h>
#elif defined(__ANDROID__)
#include <android/log.h>
static const int kMaxLogLineSize = 1024 - 60;
#endif  // __APPLE__ || __ANDROID__

#include <inttypes.h>
#include <stdio.h>
#include <time.h>

#include <algorithm>
#include <cstdarg>
#include <vector>

#include "StringBuilder.h"
namespace tuya {

// By default, release builds don't log, debug builds at info level
#if !defined(NDEBUG)
static LoggingSeverity g_min_sev = LS_INFO;
static LoggingSeverity g_dbg_sev = LS_INFO;
#else
static LoggingSeverity g_min_sev = LS_NONE;
static LoggingSeverity g_dbg_sev = LS_NONE;
#endif

// Return the filename portion of the string (that following the last slash).
const char* FilenameFromPath(const char* file) {
  const char* end1 = ::strrchr(file, '/');
  const char* end2 = ::strrchr(file, '\\');
  if (!end1 && !end2)
    return file;
  else
    return (end1 > end2) ? end1 + 1 : end2 + 1;
}

// Global lock for log subsystem, only needed to serialize access to streams_.
// TODO(bugs.webrtc.org/11665): this is not currently constant initialized and
// trivially destructible.
std::mutex g_log_mutex_;

/////////////////////////////////////////////////////////////////////////////
// LogMessage
/////////////////////////////////////////////////////////////////////////////

bool LogMessage::log_to_stderr_ = true;

// The list of logging streams currently configured.
// Note: we explicitly do not clean this up, because of the uncertain ordering
// of destructors at program exit.  Let the person who sets the stream trigger
// cleanup by setting to null, or let it leak (safe at program exit).
LogSink* LogMessage::streams_  = nullptr;
std::atomic<bool> LogMessage::streams_empty_ = {true};

// Boolean options default to false (0)
bool LogMessage::thread_, LogMessage::timestamp_;

LogMessage::LogMessage(const char* file, int line, LoggingSeverity sev)
    : LogMessage(file, line, sev, ERRCTX_NONE, 0) {}

LogMessage::LogMessage(const char* file,
                       int line,
                       LoggingSeverity sev,
                       LogErrorContext err_ctx,
                       int err)
    : severity_(sev) {
  if (timestamp_) {
	struct tm tm;
    struct timeval tv;
    #if defined(__APPLE__)
    int64_t time = SystemTimeMillis();
    int second = time/1000;
    int ms = time % 1000;
    tv.tv_usec = ms;
    gmtime_r((const time_t *)(&second), &tm);
    #else
      gettimeofday(&tv, NULL);
      localtime_r(&tv.tv_sec, &tm);
    #endif



    char timestamp[50];  // Maximum string length of an int64_t is 20.
    int len = snprintf(timestamp, sizeof(timestamp), "%4d-%02d-%02d %02d:%02d:%02d:%03d",
    		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec/1000);
    print_stream_ << timestamp;
  }

  if (thread_) {
    PlatformThreadId id = CurrentThreadId();
    print_stream_ << "[" << id << "] ";
  }

  if (file != nullptr) {
#if defined(WEBRTC_ANDROID)
    tag_ = FilenameFromPath(file);
    print_stream_ << "(line " << line << "): ";
#else
    print_stream_ << "(" << FilenameFromPath(file) << ":" << line << "): ";
#endif
  }

  if (err_ctx != ERRCTX_NONE) {
    char tmp_buf[1024] = {0};
    StringBuilder tmp(tmp_buf);
    std::string format;
    tmp.AppendFormat("[0x%08X]", err);
    switch (err_ctx) {
      case ERRCTX_ERRNO:
        tmp << " " << strerror(err);
        break;
#ifdef WIN
      case ERRCTX_HRESULT: {
        char msgbuf[256];
        DWORD flags =
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
        if (DWORD len = FormatMessageA(
                flags, nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                msgbuf, sizeof(msgbuf) / sizeof(msgbuf[0]), nullptr)) {
          while ((len > 0) &&
                 isspace(static_cast<unsigned char>(msgbuf[len - 1]))) {
            msgbuf[--len] = 0;
          }
          tmp << " " << msgbuf;
        }
        break;
      }
#endif  // WEBRTC_WIN
      default:
        break;
    }
    extra_ = tmp.str();
  }
}

#if defined(WEBRTC_ANDROID)
LogMessage::LogMessage(const char* file,
                       int line,
                       LoggingSeverity sev,
                       const char* tag)
    : LogMessage(file, line, sev, ERRCTX_NONE, 0 /* err */) {
  tag_ = tag;
  print_stream_ << tag << ": ";
}
#endif


LogMessage::~LogMessage() {
  FinishPrintStream();
  
  //std::string str = std::move(print_stream_.str());

  if (severity_ >= g_dbg_sev) {
#if defined(WEBRTC_ANDROID)
    OutputToDebug(print_stream_.str(), severity_, tag_);
#else
    OutputToDebug(print_stream_.str(), severity_);
#endif
  }

  std::lock_guard<std::mutex> _(g_log_mutex_);
  for (LogSink* entry = streams_; entry != nullptr; entry = entry->next_) {
    if (severity_ >= entry->min_severity_) {
#if defined(WEBRTC_ANDROID)
      entry->OnLogMessage(str, severity_, tag_);
#else
      entry->OnLogMessage(print_stream_.str(), severity_);
#endif
    }
  }
}

void LogMessage::AddTag(const char* tag) {
#ifdef WEBRTC_ANDROID
  tag_ = tag;
#endif
}

#ifndef USE_STD_STREAM
  StringBuilder& LogMessage::stream() {
    return print_stream_;
  }
#else
  std::ostringstream& LogMessage::stream() {
    return print_stream_;
  }
#endif


int LogMessage::GetMinLogSeverity() {
  return g_min_sev;
}

LoggingSeverity LogMessage::GetLogToDebug() {
  return g_dbg_sev;
}
int64_t LogMessage::LogStartTime() {
  static const int64_t g_start = SystemTimeMillis();
  return g_start;
}

uint32_t LogMessage::WallClockStartTime() {
  static const uint32_t g_start_wallclock = time(nullptr);
  return g_start_wallclock;
}

void LogMessage::LogThreads(bool on) {
  thread_ = on;
}

void LogMessage::LogTimestamps(bool on) {
  timestamp_ = on;
}

void LogMessage::LogToDebug(LoggingSeverity min_sev) {
  g_dbg_sev = min_sev;
  std::lock_guard<std::mutex> _(g_log_mutex_);
  UpdateMinLogSeverity();
}

void LogMessage::SetLogToStderr(bool log_to_stderr) {
  log_to_stderr_ = log_to_stderr;
}

int LogMessage::GetLogToStream(LogSink* stream) {
  std::lock_guard<std::mutex> _(g_log_mutex_);
  LoggingSeverity sev = LS_NONE;
  for (LogSink* entry = streams_; entry != nullptr; entry = entry->next_) {
    if (stream == nullptr || stream == entry) {
      sev = std::min(sev, entry->min_severity_);
    }
  }
  return sev;
}

void LogMessage::AddLogToStream(LogSink* stream, LoggingSeverity min_sev) {
  std::lock_guard<std::mutex> _(g_log_mutex_);
  stream->min_severity_ = min_sev;
  stream->next_ = streams_;
  streams_ = stream;
  streams_empty_.store(false, std::memory_order_relaxed);
  UpdateMinLogSeverity();
}

void LogMessage::RemoveLogToStream(LogSink* stream) {
  std::lock_guard<std::mutex> _(g_log_mutex_);
  for (LogSink** entry = &streams_; *entry != nullptr;
       entry = &(*entry)->next_) {
    if (*entry == stream) {
      *entry = (*entry)->next_;
      break;
    }
  }
  streams_empty_.store(streams_ == nullptr, std::memory_order_relaxed);
  UpdateMinLogSeverity();
}

void LogMessage::UpdateMinLogSeverity() {
  LoggingSeverity min_sev = g_dbg_sev;
  for (LogSink* entry = streams_; entry != nullptr; entry = entry->next_) {
    min_sev = std::min(min_sev, entry->min_severity_);
  }
  g_min_sev = min_sev;
}

#if defined(__ANDROID__)
void LogMessage::OutputToDebug(const std::string& str,
                               LoggingSeverity severity,
                               const char* tag) {
#else
void LogMessage::OutputToDebug(const std::string& str,
                               LoggingSeverity severity) {
#endif
  bool log_to_stderr = log_to_stderr_;
#if defined(__APPLE__) && defined(NDEBUG)
  // On the Mac, all stderr output goes to the Console log and causes clutter.
  // So in opt builds, don't log to stderr unless the user specifically sets
  // a preference to do so.
  CFStringRef key = CFStringCreateWithCString(
      kCFAllocatorDefault, "logToStdErr", kCFStringEncodingUTF8);
  CFStringRef domain = CFBundleGetIdentifier(CFBundleGetMainBundle());
  if (key != nullptr && domain != nullptr) {
    Boolean exists_and_is_valid;
    Boolean should_log =
        CFPreferencesGetAppBooleanValue(key, domain, &exists_and_is_valid);
    // If the key doesn't exist or is invalid or is false, we will not log to
    // stderr.
    log_to_stderr = exists_and_is_valid && should_log;
  }
  if (key != nullptr) {
    CFRelease(key);
  }
#endif  // defined(WEBRTC_MAC) && !defined(WEBRTC_IOS) && defined(NDEBUG)

#if defined(WIN)
  // Always log to the debugger.
  // Perhaps stderr should be controlled by a preference, as on Mac?
  OutputDebugStringA(str.c_str());
  if (log_to_stderr) {
    // This handles dynamically allocated consoles, too.
    if (HANDLE error_handle = ::GetStdHandle(STD_ERROR_HANDLE)) {
      log_to_stderr = false;
      DWORD written = 0;
      ::WriteFile(error_handle, str.data(), static_cast<DWORD>(str.size()),
                  &written, 0);
    }
  }
#endif  // WEBRTC_WIN

#if defined(__ANDROID__)
  // Android's logging facility uses severity to log messages but we
  // need to map libjingle's severity levels to Android ones first.
  // Also write to stderr which maybe available to executable started
  // from the shell.
  int prio;
  switch (severity) {
    case LS_VERBOSE:
      prio = ANDROID_LOG_VERBOSE;
      break;
    case LS_INFO:
      prio = ANDROID_LOG_INFO;
      break;
    case LS_WARNING:
      prio = ANDROID_LOG_WARN;
      break;
    case LS_ERROR:
      prio = ANDROID_LOG_ERROR;
      break;
    default:
      prio = ANDROID_LOG_UNKNOWN;
  }

  int size = str.size();
  int line = 0;
  int idx = 0;
  const int max_lines = size / kMaxLogLineSize + 1;
  if (max_lines == 1) {
    __android_log_print(prio, tag, "%.*s", size, str.c_str());
  } else {
    while (size > 0) {
      const int len = std::min(size, kMaxLogLineSize);
      // Use the size of the string in the format (str may have \0 in the
      // middle).
      __android_log_print(prio, tag, "[%d/%d] %.*s", line + 1, max_lines, len,
                          str.c_str() + idx);
      idx += len;
      size -= len;
      ++line;
    }
  }
#endif  // WEBRTC_ANDROID
  if (log_to_stderr) {
    fprintf(stderr, "%s", str.c_str());
    fflush(stderr);
  }
}

// static
bool LogMessage::IsNoop(LoggingSeverity severity) {
  if (severity >= g_dbg_sev || severity >= g_min_sev)
    return false;
  return streams_empty_.load(std::memory_order_relaxed);
}

void LogMessage::FinishPrintStream() {
  if (!extra_.empty())
    print_stream_ << " : " << extra_;
  print_stream_ << "\n";
}

int64_t LogMessage::SystemTimeSeconds() {
  return static_cast<int64_t>(SystemTimeNanos() / 1000000000);
}

int64_t LogMessage::SystemTimeMillis() {
  return static_cast<int64_t>(SystemTimeNanos() / 1000000);
}

int64_t LogMessage::SystemTimeNanos() {
  int64_t ticks;

#if defined(__APPLE__)
  static mach_timebase_info_data_t timebase;
  if (timebase.denom == 0) {
    // Get the timebase if this is the first time we run.
    // Recommended by Apple's QA1398.
    if (mach_timebase_info(&timebase) != KERN_SUCCESS) {
      //RTC_NOTREACHED();
    }
  }
  // Use timebase to convert absolute time tick units into nanoseconds.
  const auto mul = [](uint64_t a, uint32_t b) -> int64_t {
    //RTC_DCHECK_NE(b, 0);
    //RTC_DCHECK_LE(a, std::numeric_limits<int64_t>::max() / b)
      //  << "The multiplication " << a << " * " << b << " overflows";
    //return rtc::dchecked_cast<int64_t>(a * b);
    return (int64_t)(a * b);
  };
  ticks = mul(mach_absolute_time(), timebase.numer) / timebase.denom;
#elif defined(__POSIX__)
  struct timespec ts;
  // TODO(deadbeef): Do we need to handle the case when CLOCK_MONOTONIC is not
  // supported?
  clock_gettime(CLOCK_MONOTONIC, &ts);
  ticks = kNumNanosecsPerSec * static_cast<int64_t>(ts.tv_sec) +
          static_cast<int64_t>(ts.tv_nsec);
#elif defined(WINUWP)
  ticks = TimeHelper::TicksNs();
#elif defined(WIN)
  static volatile LONG last_timegettime = 0;
  static volatile int64_t num_wrap_timegettime = 0;
  volatile LONG* last_timegettime_ptr = &last_timegettime;
  DWORD now = timeGetTime();
  // Atomically update the last gotten time
  DWORD old = InterlockedExchange(last_timegettime_ptr, now);
  if (now < old) {
    // If now is earlier than old, there may have been a race between threads.
    // 0x0fffffff ~3.1 days, the code will not take that long to execute
    // so it must have been a wrap around.
    if (old > 0xf0000000 && now < 0x0fffffff) {
      num_wrap_timegettime++;
    }
  }
  ticks = now + (num_wrap_timegettime << 32);
  // TODO(deadbeef): Calculate with nanosecond precision. Otherwise, we're
  // just wasting a multiply and divide when doing Time() on Windows.
  ticks = ticks * kNumNanosecsPerMillisec;
#else
#error Unsupported platform.
#endif
  return ticks;
}

std::string ToHex(const int i) {
  char buffer[50];
  snprintf(buffer, sizeof(buffer), "%x", i);

  return std::string(buffer);
}
void Log(const LogArgType* fmt, ...) {
  va_list args;
  printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  va_start(args, fmt);

  LogMetadataErr meta;
  const char* tag = nullptr;
  switch (*fmt) {
    case LogArgType::kLogMetadata: {
      meta = {va_arg(args, LogMetadata), ERRCTX_NONE, 0};
      break;
    }
    case LogArgType::kLogMetadataErr: {
      meta = va_arg(args, LogMetadataErr);
      break;
    }
#ifdef __ANDROID__
    case LogArgType::kLogMetadataTag: {
      const LogMetadataTag tag_meta = va_arg(args, LogMetadataTag);
      meta = {{nullptr, 0, tag_meta.severity}, ERRCTX_NONE, 0};
      tag = tag_meta.tag;
      break;
    }
#endif
    default: {
      va_end(args);
      return;
    }
  }

  LogMessage logMessage(meta.meta.File(), meta.meta.Line(),
                         meta.meta.Severity(), meta.err_ctx, meta.err);
  if (tag) {
    logMessage.AddTag(tag);
  }

  for (++fmt; *fmt != LogArgType::kEnd; ++fmt) {
    switch (*fmt) {
      case LogArgType::kInt:
        logMessage.stream() << va_arg(args, int);
        break;
      case LogArgType::kLong:
        logMessage.stream() << va_arg(args, long);
        break;
      case LogArgType::kLongLong:
        logMessage.stream() << va_arg(args, long long);
        break;
      case LogArgType::kUInt:
        logMessage.stream() << va_arg(args, unsigned);
        break;
      case LogArgType::kULong:
        logMessage.stream() << va_arg(args, unsigned long);
        break;
      case LogArgType::kULongLong:
        logMessage.stream() << va_arg(args, unsigned long long);
        break;
      case LogArgType::kDouble:
        logMessage.stream() << va_arg(args, double);
        break;
      case LogArgType::kLongDouble:
        logMessage.stream() << va_arg(args, long double);
        break;
      case LogArgType::kCharP: {
        const char* s = va_arg(args, const char*);
        logMessage.stream() << (s ? s : "(null)");
        break;
      }
      case LogArgType::kStdString:
        logMessage.stream() << *va_arg(args, const std::string*);
        break;
      case LogArgType::kVoidP:
        logMessage.stream() << ToHex(
            reinterpret_cast<uintptr_t>(va_arg(args, const void*)));
        break;
      default:
        va_end(args);
        return;
    }
  }

  va_end(args);
}


} // namespace tuya



