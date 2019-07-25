



#ifndef CHROME_COMMON_IPC_CHANNEL_WIN_H_
#define CHROME_COMMON_IPC_CHANNEL_WIN_H_

#include "chrome/common/ipc_channel.h"

#include <queue>
#include <string>

#include "base/message_loop.h"

class NonThreadSafe;

namespace IPC {

class Channel::ChannelImpl : public MessageLoopForIO::IOHandler {
 public:
  
  ChannelImpl(const std::wstring& channel_id, Mode mode, Listener* listener);
  ChannelImpl(const std::wstring& channel_id, HANDLE server_pipe,
              Mode mode, Listener* listener);
  ~ChannelImpl() { 
    if (pipe_ != INVALID_HANDLE_VALUE) {
      Close();
    }
  }
  bool Connect();
  void Close();
#ifdef CHROMIUM_MOZILLA_BUILD
  HANDLE GetServerPipeHandle() const;

  Listener* set_listener(Listener* listener) {
    Listener* old = listener_;
    listener_ = listener;
    return old;
  }
#else
  void set_listener(Listener* listener) { listener_ = listener; }
#endif
  bool Send(Message* message);
 private:
  void Init(Mode mode, Listener* listener);
  const std::wstring PipeName(const std::wstring& channel_id) const;
  bool CreatePipe(const std::wstring& channel_id, Mode mode);
  bool EnqueueHelloMessage();

  bool ProcessConnection();
  bool ProcessIncomingMessages(MessageLoopForIO::IOContext* context,
                               DWORD bytes_read);
  bool ProcessOutgoingMessages(MessageLoopForIO::IOContext* context,
                               DWORD bytes_written);

  
  virtual void OnIOCompleted(MessageLoopForIO::IOContext* context,
                             DWORD bytes_transfered, DWORD error);
 private:
  struct State {
    explicit State(ChannelImpl* channel);
    ~State();
    MessageLoopForIO::IOContext context;
    bool is_pending;
  };

  State input_state_;
  State output_state_;

  HANDLE pipe_;

  Listener* listener_;

  
  std::queue<Message*> output_queue_;

  
  char input_buf_[Channel::kReadBufferSize];

  
  
  std::string input_overflow_buf_;

  
  
  
  bool waiting_connect_;

  
  
  
  bool processing_incoming_;

  ScopedRunnableMethodFactory<ChannelImpl> factory_;

  scoped_ptr<NonThreadSafe> thread_check_;

  DISALLOW_COPY_AND_ASSIGN(ChannelImpl);
};

}  

#endif  
