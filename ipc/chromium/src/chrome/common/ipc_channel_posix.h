



#ifndef CHROME_COMMON_IPC_CHANNEL_POSIX_H_
#define CHROME_COMMON_IPC_CHANNEL_POSIX_H_

#include "chrome/common/ipc_channel.h"

#include <sys/socket.h>  

#include <queue>
#include <string>
#include <vector>
#include <list>

#include "base/message_loop.h"
#include "chrome/common/file_descriptor_set_posix.h"

#include "nsAutoPtr.h"

namespace IPC {



class Channel::ChannelImpl : public MessageLoopForIO::Watcher {
 public:
  
  ChannelImpl(const std::wstring& channel_id, Mode mode, Listener* listener);
  ChannelImpl(int fd, Mode mode, Listener* listener);
  ~ChannelImpl() { Close(); }
  bool Connect();
  void Close();
  Listener* set_listener(Listener* listener) {
    Listener* old = listener_;
    listener_ = listener;
    return old;
  }
  bool Send(Message* message);
  void GetClientFileDescriptorMapping(int *src_fd, int *dest_fd) const;

  void ResetFileDescriptor(int fd);

  int GetFileDescriptor() const {
      return pipe_;
  }
  void CloseClientFileDescriptor();

  
  
  bool Unsound_IsClosed() const;
  uint32_t Unsound_NumQueuedMessages() const;

 private:
  void Init(Mode mode, Listener* listener);
  bool CreatePipe(const std::wstring& channel_id, Mode mode);
  bool EnqueueHelloMessage();

  bool ProcessIncomingMessages();
  bool ProcessOutgoingMessages();

  
  virtual void OnFileCanReadWithoutBlocking(int fd);
  virtual void OnFileCanWriteWithoutBlocking(int fd);

#if defined(OS_MACOSX)
  void CloseDescriptors(uint32_t pending_fd_id);
#endif

  void OutputQueuePush(Message* msg);
  void OutputQueuePop();

  Mode mode_;

  
  
  MessageLoopForIO::FileDescriptorWatcher server_listen_connection_watcher_;
  MessageLoopForIO::FileDescriptorWatcher read_watcher_;
  MessageLoopForIO::FileDescriptorWatcher write_watcher_;

  
  bool is_blocked_on_write_;

  
  
  size_t message_send_bytes_written_;

  
  
  bool uses_fifo_;

  int server_listen_pipe_;
  int pipe_;
  int client_pipe_;  

  
  
  std::string pipe_name_;

  Listener* listener_;

  
  std::queue<Message*> output_queue_;

  
  char input_buf_[Channel::kReadBufferSize];

  
  
  
  
  
  
  
  enum {
    kControlBufferSlopBytes = 32
  };

  
  
  
  
  
  
  
  
  
  char input_cmsg_buf_[Channel::kReadBufferSize + kControlBufferSlopBytes];

  
  
  std::string input_overflow_buf_;
  std::vector<int> input_overflow_fds_;

  
  
  
  bool waiting_connect_;

  
  
  
  bool processing_incoming_;

  
  bool closed_;

#if defined(OS_MACOSX)
  struct PendingDescriptors {
    uint32_t id;
    nsRefPtr<FileDescriptorSet> fds;

    PendingDescriptors() : id(0) { }
    PendingDescriptors(uint32_t id, FileDescriptorSet *fds)
      : id(id),
        fds(fds)
    { }
  };

  std::list<PendingDescriptors> pending_fds_;

  
  uint32_t last_pending_fd_id_;
#endif





  size_t output_queue_length_;

  ScopedRunnableMethodFactory<ChannelImpl> factory_;

  DISALLOW_COPY_AND_ASSIGN(ChannelImpl);
};

}  

#endif
