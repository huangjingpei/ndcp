#ifndef __THREAD_TYPES_H__
#define __THREAD_TYPES_H__


#if defined(WIN)
// Include winsock2.h before including <windows.h> to maintain consistency with
// win32.h. To include win32.h directly, it must be broken out into its own
// build target.
#include <winsock2.h>
#include <windows.h>
#elif defined(__FUCHSIA__)
#include <zircon/types.h>
#include <zircon/process.h>
#elif defined(__LINUX__) || defined(__ANDROID__) || defined(__APPLE__)
#include <pthread.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <pthread_spis.h>
#endif
#endif
// clang-format on
namespace tuya {


#if defined(WIN)
typedef DWORD PlatformThreadId;
typedef DWORD PlatformThreadRef;
#elif defined(__FUCHSIA__)
typedef zx_handle_t PlatformThreadId;
typedef zx_handle_t PlatformThreadRef;
#elif defined(__LINUX__) || defined(__ANDROID__) || defined(__APPLE__)
typedef pid_t PlatformThreadId;
typedef pthread_t PlatformThreadRef;
#endif

// Retrieve the ID of the current thread.
PlatformThreadId CurrentThreadId();

// Retrieves a reference to the current thread. On Windows, this is the same
// as CurrentThreadId. On other platforms it's the pthread_t returned by
// pthread_self().
PlatformThreadRef CurrentThreadRef();

// Compares two thread identifiers for equality.
bool IsThreadRefEqual(const PlatformThreadRef& a, const PlatformThreadRef& b);

// Sets the current thread name.
void SetCurrentThreadName(const char* name);

} // namespace tuya


#endif//__THREAD_TYPES_H__
