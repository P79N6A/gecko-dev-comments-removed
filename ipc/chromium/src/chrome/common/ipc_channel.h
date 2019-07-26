



#ifndef CHROME_COMMON_IPC_CHANNEL_H_
#define CHROME_COMMON_IPC_CHANNEL_H_

#include <queue>
#include "chrome/common/ipc_message.h"

namespace IPC {



class Channel : public Message::Sender {
  
  friend class ChannelTest;

 public:
  
  class Listener {
   public:
    virtual ~Listener() {}

    
    virtual void OnMessageReceived(const Message& message) = 0;

    
    
    virtual void OnChannelConnected(int32_t peer_pid) {}

    
    
    virtual void OnChannelError() {}

    
    
    virtual void GetQueuedMessages(std::queue<Message>& queue) {}
  };

  enum Mode {
    MODE_SERVER,
    MODE_CLIENT
  };

  enum {
    
    
    kMaximumMessageSize = 256 * 1024 * 1024,

    
    kReadBufferSize = 4 * 1024
  };

  
  
  
  
  
  
  
  
  
  
  Channel(const std::wstring& channel_id, Mode mode, Listener* listener);

  
  
# if defined(OS_POSIX)
  
  Channel(int fd, Mode mode, Listener* listener);
# elif defined(OS_WIN)
  
  
  Channel(const std::wstring& channel_id, void* server_pipe,
	  Mode mode, Listener* listener);
# endif

  ~Channel();

  
  
  
  
  
  bool Connect();

  
  void Close();

  
  Listener* set_listener(Listener* listener);

  
  
  
  
  
  
  
  
  
  virtual bool Send(Message* message);

#if defined(OS_POSIX)
  
  
  
  
  
  
  
  
  void GetClientFileDescriptorMapping(int *src_fd, int *dest_fd) const;

  
  int GetServerFileDescriptor() const;
#elif defined(OS_WIN)
  
  void* GetServerPipeHandle() const;
#endif  

 private:
  
  class ChannelImpl;
  ChannelImpl *channel_impl_;

  
  
  
  
  enum {
    HELLO_MESSAGE_TYPE = kuint16max  
                                     
                                     
                                     
  };
};

}  

#endif  
