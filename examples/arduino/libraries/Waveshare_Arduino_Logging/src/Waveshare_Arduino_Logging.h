#pragma once

#include <Arduino.h>

namespace waveshare {
namespace logging {

inline void beginSerialLog(unsigned long baud = 115200) {
  Serial.begin(baud);

#if ARDUINO_USB_CDC_ON_BOOT
  // A zero timeout keeps optional logging from delaying application startup
  // when USB CDC is not connected or the host is not draining output.
  Serial.setTxTimeoutMs(0);
#endif
}

}  // namespace logging
}  // namespace waveshare
