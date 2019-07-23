



#ifndef CHROME_COMMON_IPC_CHANNEL_PROXY_H__
#define CHROME_COMMON_IPC_CHANNEL_PROXY_H__

#include <vector>
#include "base/lock.h"
#include "base/ref_counted.h"
#include "chrome/common/ipc_channel.h"

class MessageLoop;

namespace IPC {





























class ChannelProxy : public Message::Sender {
 public:
  
  
  class MessageFilter : public base::RefCountedThreadSafe<MessageFilter> {
   public:
    virtual ~MessageFilter() {}

    
    
    
    virtual void OnFilterAdded(Channel* channel) {}

    
    
    
    virtual void OnFilterRemoved() {}

    
    
    virtual void OnChannelConnected(int32 peer_pid) {}

    
    
    virtual void OnChannelError() {}

    
    
    virtual void OnChannelClosing() {}

    
    
    virtual bool OnMessageReceived(const Message& message) {
      return false;
    }
  };

  
  
  
  
  
  
  
  
  ChannelProxy(const std::wstring& channel_id, Channel::Mode mode,
               Channel::Listener* listener, MessageFilter* filter,
               MessageLoop* ipc_thread_loop);

  ~ChannelProxy() {
    Close();
  }

  
  
  
  
  
  
  
  
  void Close();

  
  
  virtual bool Send(Message* message);

  
  
  
  
  
  void AddFilter(MessageFilter* filter);
  void RemoveFilter(MessageFilter* filter);

#if defined(OS_POSIX)
  
  
  
  void GetClientFileDescriptorMapping(int *src_fd, int *dest_fd) const;
#endif  

 protected:
  class Context;
  
  
  
  ChannelProxy(const std::wstring& channel_id, Channel::Mode mode,
               MessageLoop* ipc_thread_loop, Context* context,
               bool create_pipe_now);

  
  class Context : public base::RefCountedThreadSafe<Context>,
                  public Channel::Listener {
   public:
    Context(Channel::Listener* listener, MessageFilter* filter,
            MessageLoop* ipc_thread);
    virtual ~Context() { }
    MessageLoop* ipc_message_loop() const { return ipc_message_loop_; }
    const std::wstring& channel_id() const { return channel_id_; }

    
    void OnDispatchMessage(const Message& message);

   protected:
    
    virtual void OnMessageReceived(const Message& message);
    virtual void OnChannelConnected(int32 peer_pid);
    virtual void OnChannelError();

    
    void OnMessageReceivedNoFilter(const Message& message);

    
    
    bool TryFilters(const Message& message);

    
    virtual void OnChannelOpened();
    virtual void OnChannelClosed();

    
    
    
    virtual void Clear() { listener_ = NULL; }

   private:
    friend class ChannelProxy;
    
    void CreateChannel(const std::wstring& id, const Channel::Mode& mode);

    
    void OnSendMessage(Message* message_ptr);
    void OnAddFilter(MessageFilter* filter);
    void OnRemoveFilter(MessageFilter* filter);
    void OnDispatchConnected();
    void OnDispatchError();

    MessageLoop* listener_message_loop_;
    Channel::Listener* listener_;

    
    std::vector<scoped_refptr<MessageFilter> > filters_;
    MessageLoop* ipc_message_loop_;
    Channel* channel_;
    std::wstring channel_id_;
    int peer_pid_;
    bool channel_connected_called_;
  };

  Context* context() { return context_; }

 private:
  void Init(const std::wstring& channel_id, Channel::Mode mode,
            MessageLoop* ipc_thread_loop, bool create_pipe_now);

  
  
  
  scoped_refptr<Context> context_;
};

}  

#endif  
