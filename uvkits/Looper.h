#ifndef __OSCP_LOOPER_H__
#define __OSCP_LOOPER_H__

#include <stdint.h>
#include "uv.h"

namespace ndcp {

class Looper {
public:
	static void init();
	static void destory();
	static void loop();
	static uv_loop_t* getLooper();
	static uint64_t getTimeMs();
	static uint64_t getTimeUs();
	static uint64_t getTimeNs();

private:
	static uv_loop_t *loop_;
};

/* Inline static methods. */

inline uv_loop_t* Looper::getLooper() {
	if (loop_ == nullptr) {
		Looper::init();
	}
	return Looper::loop_;
}

inline uint64_t Looper::getTimeMs() {
	return static_cast<uint64_t>(uv_hrtime() / 1000000u);
}

inline uint64_t Looper::getTimeUs() {
	return static_cast<uint64_t>(uv_hrtime() / 1000u);
}

inline uint64_t Looper::getTimeNs() {
	return uv_hrtime();
}

};
#endif //__OSCP_LOOPER_H__
