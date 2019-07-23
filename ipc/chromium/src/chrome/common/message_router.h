



#ifndef CHROME_COMMON_MESSAGE_ROUTER_H__
#define CHROME_COMMON_MESSAGE_ROUTER_H__

#include "base/id_map.h"
#include "chrome/common/ipc_channel.h"



















class MessageRouter : public IPC::Channel::Listener,
                      public IPC::Message::Sender {
 public:
  MessageRouter() {}
  virtual ~MessageRouter() {}

  
  virtual void OnControlMessageReceived(const IPC::Message& msg);

  
  virtual void OnMessageReceived(const IPC::Message& msg);

  
  
  
  virtual bool RouteMessage(const IPC::Message& msg);

  
  virtual bool Send(IPC::Message* msg);

  
  void AddRoute(int32 routing_id, IPC::Channel::Listener* listener);
  void RemoveRoute(int32 routing_id);

 private:
  
  IDMap<IPC::Channel::Listener> routes_;

  DISALLOW_EVIL_CONSTRUCTORS(MessageRouter);
};

#endif  
