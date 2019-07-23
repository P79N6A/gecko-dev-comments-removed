



#ifndef CHROME_COMMON_IPC_SYNC_MESSAGE_H__
#define CHROME_COMMON_IPC_SYNC_MESSAGE_H__

#if defined(OS_WIN)
#include <windows.h>
#endif
#include <string>
#include "base/basictypes.h"
#include "chrome/common/ipc_message.h"

namespace base {
class WaitableEvent;
}

namespace IPC {

class MessageReplyDeserializer;

class SyncMessage : public Message {
 public:
  SyncMessage(int32 routing_id, uint16 type, PriorityValue priority,
              MessageReplyDeserializer* deserializer);

  
  
  
  MessageReplyDeserializer* GetReplyDeserializer();

  
  
  
  
  
  
  void set_pump_messages_event(base::WaitableEvent* event) {
    pump_messages_event_ = event;
    if (event) {
      header()->flags |= PUMPING_MSGS_BIT;
    } else {
      header()->flags &= ~PUMPING_MSGS_BIT;
    }
  }

  
  
  void EnableMessagePumping();

  base::WaitableEvent* pump_messages_event() const {
    return pump_messages_event_;
  }

  
  static bool IsMessageReplyTo(const Message& msg, int request_id);

  
  
  static void* GetDataIterator(const Message* msg);

  
  static int GetMessageId(const Message& msg);

  
  static Message* GenerateReply(const Message* msg);

 private:
  struct SyncHeader {
    
    int message_id;
  };

  static bool ReadSyncHeader(const Message& msg, SyncHeader* header);
  static bool WriteSyncHeader(Message* msg, const SyncHeader& header);

  MessageReplyDeserializer* deserializer_;
  base::WaitableEvent* pump_messages_event_;

  static uint32 next_id_;  
};


class MessageReplyDeserializer {
 public:
  bool SerializeOutputParameters(const Message& msg);
 private:
  
  
  virtual bool SerializeOutputParameters(const Message& msg, void* iter) = 0;
};

}  

#endif  
