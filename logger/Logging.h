
#ifndef __LOGGING_H_
#define __LOGGING_H_

#include <errno.h>

#include <atomic>
#include <sstream>
#include <string>
#include <utility>

#define USE_STD_STREAM
#ifndef USE_STD_STREAM
#include "StringBuilder.h"
#else
#include <sstream>
#endif


#if !defined(NDEBUG)
#define LOG_IS_ON 1
#else
#define LOG_IS_ON 0
#endif

namespace tuya {

enum LoggingSeverity {
  LS_VERBOSE,
  LS_INFO,
  LS_WARNING,
  LS_ERROR,
  LS_NONE,
  INFO = LS_INFO,
  WARNING = LS_WARNING,
  LERROR = LS_ERROR
};

// LogErrorContext assists in interpreting the meaning of an error value.
enum LogErrorContext {
  ERRCTX_NONE,
  ERRCTX_ERRNO,    // System-local errno
  ERRCTX_HRESULT,  // Windows HRESULT

  // Abbreviations for LOG_E macro
  ERRCTX_EN = ERRCTX_ERRNO,    // LOG_E(sev, EN, x)
  ERRCTX_HR = ERRCTX_HRESULT,  // LOG_E(sev, HR, x)
};

class LogMessage;
// Virtual sink interface that can receive log messages.
class LogSink {
 public:
  LogSink() {}
  virtual ~LogSink() {}
  virtual void OnLogMessage(const std::string& msg,
                            LoggingSeverity severity,
                            const char* tag) {
      OnLogMessage(tag + (": " + msg), severity);
  }
  virtual void OnLogMessage(const std::string& message,
                            LoggingSeverity severity) {
      OnLogMessage(message);
  }
  virtual void OnLogMessage(const std::string& message) = 0;

 private:
  friend class LogMessage;
  // Members for LogMessage class to keep linked list of the registered sinks.
  LogSink* next_ = nullptr;
  LoggingSeverity min_severity_;
};


class LogMetadata {
 public:
  LogMetadata(const char* file, int line, LoggingSeverity severity)
      : file_(file),
        line_and_sev_(static_cast<uint32_t>(line) << 3 | severity) {}
  LogMetadata() = default;

  const char* File() const { return file_; }
  int Line() const { return line_and_sev_ >> 3; }
  LoggingSeverity Severity() const {
    return static_cast<LoggingSeverity>(line_and_sev_ & 0x7);
  }

 private:
  const char* file_;

  // Line number and severity, the former in the most significant 29 bits, the
  // latter in the least significant 3 bits. (This is an optimization; since
  // both numbers are usually compile-time constants, this way we can load them
  // both with a single instruction.)
  uint32_t line_and_sev_;
};

struct LogMetadataErr {
  LogMetadata meta;
  LogErrorContext err_ctx;
  int err;
};

#ifdef __ANDROID__
struct LogMetadataTag {
  LoggingSeverity severity;
  const char* tag;
};
#endif

enum class LogArgType : int8_t {
  kEnd = 0,
  kInt,
  kLong,
  kLongLong,
  kUInt,
  kULong,
  kULongLong,
  kDouble,
  kLongDouble,
  kCharP,
  kStdString,
  kStringView,
  kVoidP,
  kLogMetadata,
  kLogMetadataErr,
#ifdef __ANDROID__
  kLogMetadataTag,
#endif
};


// Direct use of this class is deprecated; please use the logging macros
// instead.
// TODO(bugs.webrtc.org/9278): Move this class to an unnamed namespace in the
// .cc file.
class LogMessage {
 public:
  // Same as the above, but using a compile-time constant for the logging
  // severity. This saves space at the call site, since passing an empty struct
  // is generally the same as not passing an argument at all.
  template <LoggingSeverity S>
  LogMessage(const char* file,
                           int line,
                           std::integral_constant<LoggingSeverity, S>)
      : LogMessage(file, line, S) {}

  LogMessage(const char* file, int line, LoggingSeverity sev);
  LogMessage(const char* file,
             int line,
             LoggingSeverity sev,
             LogErrorContext err_ctx,
             int err);
#if defined(__ANDROID__)
  LogMessage(const char* file, int line, LoggingSeverity sev, const char* tag);
#endif

  ~LogMessage();

  void AddTag(const char* tag);
#ifndef USE_STD_STREAM
  StringBuilder& stream();
#else
  std::ostringstream& stream();
#endif
  // Returns the time at which this function was called for the first time.
  // The time will be used as the logging start time.
  // If this is not called externally, the LogMessage ctor also calls it, in
  // which case the logging start time will be the time of the first LogMessage
  // instance is created.
  static int64_t LogStartTime();
  // Returns the wall clock equivalent of |LogStartTime|, in seconds from the
  // epoch.
  static uint32_t WallClockStartTime();
  //  LogThreads: Display the thread identifier of the current thread
  static void LogThreads(bool on = true);
  //  LogTimestamps: Display the elapsed time of the program
  static void LogTimestamps(bool on = true);
  // These are the available logging channels
  //  Debug: Debug console on Windows, otherwise stderr
  static void LogToDebug(LoggingSeverity min_sev);
  static LoggingSeverity GetLogToDebug();
  // Sets whether logs will be directed to stderr in debug mode.
  static void SetLogToStderr(bool log_to_stderr);
  // Stream: Any non-blocking stream interface.
  // Installs the |stream| to collect logs with severtiy |min_sev| or higher.
  // |stream| must live until deinstalled by RemoveLogToStream.
  // If |stream| is the first stream added to the system, we might miss some
  // early concurrent log statement happening from another thread happening near
  // this instant.
  static void AddLogToStream(LogSink* stream, LoggingSeverity min_sev);
  // Removes the specified stream, without destroying it. When the method
  // has completed, it's guaranteed that |stream| will receive no more logging
  // calls.
  static void RemoveLogToStream(LogSink* stream);
  // Returns the severity for the specified stream, of if none is specified,
  // the minimum stream severity.
  static int GetLogToStream(LogSink* stream = nullptr);
  // Testing against MinLogSeverity allows code to avoid potentially expensive
  // logging operations by pre-checking the logging level.
  static int GetMinLogSeverity();
  // Parses the provided parameter stream to configure the options above.
  // Useful for configuring logging from the command line.
  static void ConfigureLogging(const char* params);
  // Checks the current global debug severity and if the |streams_| collection
  // is empty. If |severity| is smaller than the global severity and if the
  // |streams_| collection is empty, the LogMessage will be considered a noop
  // LogMessage.
  static bool IsNoop(LoggingSeverity severity);
  // Version of IsNoop that uses fewer instructions at the call site, since the
  // caller doesn't have to pass an argument.
  template <LoggingSeverity S>
  static bool IsNoop() {
    return IsNoop(S);
  }


 private:
  friend class LogMessageForTesting;

  // Updates min_sev_ appropriately when debug sinks change.
  static void UpdateMinLogSeverity();

// These write out the actual log messages.
#if defined(__ANDROID__)
  static void OutputToDebug(const std::string& msg,
                            LoggingSeverity severity,
                            const char* tag);
#else
  static void OutputToDebug(const std::string& msg, LoggingSeverity severity);
#endif  // defined(WEBRTC_ANDROID)


  static int64_t SystemTimeSeconds();
  static int64_t SystemTimeNanos();
  static int64_t SystemTimeMillis();

  // Called from the dtor (or from a test) to append optional extra error
  // information to the log stream and a newline character.
  void FinishPrintStream();

  // The severity level of this message
  LoggingSeverity severity_;

#if defined(__ANDROID__)
  // The default Android debug output tag.
  const char* tag_ = "Native";
#endif

  // String data generated in the constructor, that should be appended to
  // the message before output.
  std::string extra_;

  // The output streams and their associated severities
  static LogSink* streams_;

  // Holds true with high probability if |streams_| is empty, false with high
  // probability otherwise. Operated on with std::memory_order_relaxed because
  // it's ok to lose or log some additional statements near the instant streams
  // are added/removed.
  static std::atomic<bool> streams_empty_;

  // Flags for formatting options
  static bool thread_, timestamp_;

  // Determines if logs will be directed to stderr in debug mode.
  static bool log_to_stderr_;

#ifndef USE_STD_STREAM
  StringBuilder print_stream_;
#else
  std::ostringstream print_stream_;
#endif

 private:
  LogMessage(const LogMessage&) = delete;
  LogMessage& operator=(const LogMessage&) = delete;
};

#ifndef __ANDROID__
#define LOGV if (LOG_IS_ON && (tuya::LoggingSeverity::LS_VERBOSE <= tuya::LogMessage::GetMinLogSeverity())) \
		tuya::LogMessage(__FILE__, __LINE__, tuya::LoggingSeverity::LS_VERBOSE).stream()
#define LOGI if (LOG_IS_ON && (tuya::LoggingSeverity::LS_INFO <= tuya::LogMessage::GetMinLogSeverity())) \
		tuya::LogMessage(__FILE__, __LINE__, tuya::LoggingSeverity::LS_INFO).stream()
#define LOGW if (LOG_IS_ON && (tuya::LoggingSeverity::LS_WARNING <= tuya::LogMessage::GetMinLogSeverity())) \
		tuya::LogMessage(__FILE__, __LINE__, tuya::LoggingSeverity::LS_WARNING).stream()
#define LOGE if (LOG_IS_ON) tuya::LogMessage(__FILE__, __LINE__, tuya::LoggingSeverity::LS_ERROR).stream()
#define LOGS if (LOG_IS_ON) tuya::LogMessage(__FILE__, __LINE__, tuya::LoggingSeverity::LS_ERROR, tuya::LogErrorContext::ERRCTX_ERRNO, errno).stream()
#else
#define LOGV if (LOG_IS_ON && (tuya::LoggingSeverity::LS_VERBOSE <= tuya::LogMessage::GetMinLogSeverity())) \
		tuya::LogMessage(__FILE__, __LINE__, LoggingSeverity::LS_VERBOSE, "Native").stream()
#define LOGI if (LOG_IS_ON && (tuya::LoggingSeverity::LS_INFO <= tuya::LogMessage::GetMinLogSeverity())) \
		tuya::LogMessage(__FILE__, __LINE__, LoggingSeverity::LS_INFO, "Native").stream()
#define LOGW if (LOG_IS_ON && (tuya::LoggingSeverity::LS_WARNING <= tuya::LogMessage::GetMinLogSeverity())) \
		tuya::LogMessage(__FILE__, __LINE__, tuya::LoggingSeverity::LS_WARNING, "Native").stream()
#define LOGE if (LOG_IS_ON) tuya::LogMessage(__FILE__, __LINE__, tuya::LoggingSeverity::LS_ERROR, "Native").stream()
#define LOGS if (LOG_IS_ON) tuya::LogMessage(__FILE__, __LINE__, tuya::LoggingSeverity::LS_ERROR, tuya::LogErrorContext::ERRCTX_ERRNO, errno).stream()
#endif


#if defined(__cplusplus)
extern "C" {
void Log(const LogArgType* fmt, ...);


#endif
#if defined(__cplusplus)
}
#endif  /* end 'extern "C"' wrapper */

} // namespace tuya

#endif  // __LOGGING_H_
