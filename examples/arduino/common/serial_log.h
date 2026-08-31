#pragma once

#include <Arduino.h>

namespace waveshare {
namespace logging {

inline void beginSerialLog(unsigned long baud = 115200) {
  Serial.begin(baud);

#if ARDUINO_USB_CDC_ON_BOOT
  // Native USB CDC and Hardware CDC/JTAG expose the same timeout API.  A zero
  // timeout keeps optional logging from delaying application startup when no
  // host is connected or when the host stops draining CDC output.
  Serial.setTxTimeoutMs(0);
#endif
}

}  // namespace logging
}  // namespace waveshare
