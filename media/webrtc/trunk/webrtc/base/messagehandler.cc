









#include "webrtc/base/messagehandler.h"
#include "webrtc/base/messagequeue.h"

namespace rtc {

MessageHandler::~MessageHandler() {
  MessageQueueManager::Clear(this);
}

} 
