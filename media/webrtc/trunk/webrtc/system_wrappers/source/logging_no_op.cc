









#include "webrtc/system_wrappers/interface/logging.h"

namespace webrtc {

LogMessage::LogMessage(const char*, int, LoggingSeverity) {
  
  (void)severity_;
}

LogMessage::~LogMessage() {
}

}  
