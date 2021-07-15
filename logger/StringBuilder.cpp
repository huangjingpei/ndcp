/*
 *  Copyright 2018 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "StringBuilder.h"

#include <stdarg.h>

#include <cstdio>
#include <cstring>
namespace tuya {


StringBuilder& StringBuilder::AppendFormat(const char* fmt, ...) {
  va_list args, copy;
  va_start(args, fmt);
  va_copy(copy, args);
  const int predicted_length = std::vsnprintf(nullptr, 0, fmt, copy);
  va_end(copy);

  if (predicted_length > 0) {
    const size_t size = str_.size();
    str_.resize(size + predicted_length);
    // Pass "+ 1" to vsnprintf to include space for the '\0'.
	std::vsnprintf(&str_[size], predicted_length + 1, fmt, args);
  }
  va_end(args);
  return *this;
}

} // namespace tuya
