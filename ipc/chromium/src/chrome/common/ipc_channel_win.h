



#ifndef CHROME_COMMON_IPC_CHANNEL_WIN_H_
#define CHROME_COMMON_IPC_CHANNEL_WIN_H_

#include "chrome/common/ipc_channel.h"

#include <queue>
#include <string>

#include "base/message_loop.h"
#include "mozilla/UniquePtr.h"

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
  HANDLE GetServerPipeHandle() const;
  Listener* set_listener(Listener* listener) {
    Listener* old = listener_;
    listener_ = listener;
    return old;
  }
  bool Send(Message* message);

  
  
  bool Unsound_IsClosed() const;
  uint32_t Unsound_NumQueuedMessages() const;

 private:
  void Init(Mode mode, Listener* listener);

  void OutputQueuePush(Message* msg);
  void OutputQueuePop();

  const std::wstring PipeName(const std::wstring& channel_id,
                              int32_t* secret) const;
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

  
  bool closed_;

  
  
  
  
  size_t output_queue_length_;

  ScopedRunnableMethodFactory<ChannelImpl> factory_;

  
  
  
  
  int32_t shared_secret_;

  
  
  bool waiting_for_shared_secret_;

  mozilla::UniquePtr<NonThreadSafe> thread_check_;

  DISALLOW_COPY_AND_ASSIGN(ChannelImpl);
};

}  

#endif  
