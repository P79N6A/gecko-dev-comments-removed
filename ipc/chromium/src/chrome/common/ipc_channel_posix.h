



#ifndef CHROME_COMMON_IPC_CHANNEL_POSIX_H_
#define CHROME_COMMON_IPC_CHANNEL_POSIX_H_

#include "chrome/common/ipc_channel.h"

#include <sys/socket.h>  

#include <queue>
#include <string>
#include <vector>

#include "base/message_loop.h"
#include "chrome/common/file_descriptor_set_posix.h"

namespace IPC {



class Channel::ChannelImpl : public MessageLoopForIO::Watcher {
 public:
  
  ChannelImpl(const std::wstring& channel_id, Mode mode, Listener* listener);
  ~ChannelImpl() { Close(); }
  bool Connect();
  void Close();
  void set_listener(Listener* listener) { listener_ = listener; }
  bool Send(Message* message);
  void GetClientFileDescriptorMapping(int *src_fd, int *dest_fd) const;

 private:
  bool CreatePipe(const std::wstring& channel_id, Mode mode);

  bool ProcessIncomingMessages();
  bool ProcessOutgoingMessages();

  
  virtual void OnFileCanReadWithoutBlocking(int fd);
  virtual void OnFileCanWriteWithoutBlocking(int fd);

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
    
    
    MAX_READ_FDS = (Channel::kReadBufferSize / sizeof(IPC::Message::Header)) *
                   FileDescriptorSet::MAX_DESCRIPTORS_PER_MESSAGE,
  };

  
#if defined(OS_MACOSX)
  
  char input_cmsg_buf_[1024];
#else
  char input_cmsg_buf_[CMSG_SPACE(sizeof(int) * MAX_READ_FDS)];
#endif

  
  
  std::string input_overflow_buf_;
  std::vector<int> input_overflow_fds_;

  
  
  
  bool waiting_connect_;

  
  
  
  bool processing_incoming_;

  ScopedRunnableMethodFactory<ChannelImpl> factory_;

  DISALLOW_COPY_AND_ASSIGN(ChannelImpl);
};

}  

#endif  
