#include "Looper.h"
#include <cstdlib> // std::abort()
#include "uv.h"

namespace ndcp {



/* Static variables. */

uv_loop_t *Looper::loop_ { nullptr };

/* Static methods. */

void Looper::init() {
	// NOTE: Logger depends on this so we cannot log anything here.

	Looper::loop_ = new uv_loop_t;

	int err = uv_loop_init(Looper::loop_);

	if (err != 0) {
		printf("libuv initialization failed\n");
	}
}

void Looper::destory() {
	if (Looper::loop_ != nullptr) {
		uv_loop_close(Looper::loop_);
		delete Looper::loop_;
	}
}



void Looper::loop() {
	// This should never happen.
	//printf(Looper::loop_ != nullptr, "loop unset");

	uv_run(Looper::loop_, UV_RUN_DEFAULT);
}


} // namespace ndcp
