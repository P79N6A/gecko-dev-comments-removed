



#include "chrome/common/ipc_channel_posix.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <string>
#include <map>

#include "base/command_line.h"
#include "base/eintr_wrapper.h"
#include "base/lock.h"
#include "base/logging.h"
#include "base/process_util.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "base/singleton.h"
#include "base/stats_counters.h"
#include "chrome/common/chrome_counters.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/file_descriptor_set_posix.h"
#include "chrome/common/ipc_logging.h"
#include "chrome/common/ipc_message_utils.h"

namespace IPC {













namespace {




























class PipeMap {
 public:
  
  int Lookup(const std::string& channel_id) {
    AutoLock locked(lock_);

    ChannelToFDMap::const_iterator i = map_.find(channel_id);
    if (i == map_.end())
      return -1;
    return i->second;
  }

  
  
  void Remove(const std::string& channel_id) {
    AutoLock locked(lock_);

    ChannelToFDMap::iterator i = map_.find(channel_id);
    if (i != map_.end())
      map_.erase(i);
  }

  
  
  void Insert(const std::string& channel_id, int fd) {
    AutoLock locked(lock_);
    DCHECK(fd != -1);

    ChannelToFDMap::const_iterator i = map_.find(channel_id);
    CHECK(i == map_.end()) << "Creating second IPC server for '"
                           << channel_id
                           << "' while first still exists";
    map_[channel_id] = fd;
  }

 private:
  Lock lock_;
  typedef std::map<std::string, int> ChannelToFDMap;
  ChannelToFDMap map_;
};



static const int kClientChannelFd = 3;


int ChannelNameToClientFD(const std::string& channel_id) {
  
  const int fd = Singleton<PipeMap>()->Lookup(channel_id);
  if (fd != -1)
    return dup(fd);

  
  
  return kClientChannelFd;
}


sockaddr_un sizecheck;
const size_t kMaxPipeNameLength = sizeof(sizecheck.sun_path);


bool CreateServerFifo(const std::string& pipe_name, int* server_listen_fd) {
  DCHECK(server_listen_fd);
  DCHECK_GT(pipe_name.length(), 0u);
  DCHECK_LT(pipe_name.length(), kMaxPipeNameLength);

  if (pipe_name.length() == 0 || pipe_name.length() >= kMaxPipeNameLength) {
    return false;
  }

  
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    return false;
  }

  
  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
    HANDLE_EINTR(close(fd));
    return false;
  }

  
  unlink(pipe_name.c_str());

  
  struct sockaddr_un unix_addr;
  memset(&unix_addr, 0, sizeof(unix_addr));
  unix_addr.sun_family = AF_UNIX;
  snprintf(unix_addr.sun_path, kMaxPipeNameLength, "%s", pipe_name.c_str());
  size_t unix_addr_len = offsetof(struct sockaddr_un, sun_path) +
      strlen(unix_addr.sun_path) + 1;

  
  if (bind(fd, reinterpret_cast<const sockaddr*>(&unix_addr),
           unix_addr_len) != 0) {
    HANDLE_EINTR(close(fd));
    return false;
  }

  
  const int listen_queue_length = 1;
  if (listen(fd, listen_queue_length) != 0) {
    HANDLE_EINTR(close(fd));
    return false;
  }

  *server_listen_fd = fd;
  return true;
}


bool ServerAcceptFifoConnection(int server_listen_fd, int* server_socket) {
  DCHECK(server_socket);

  int accept_fd = HANDLE_EINTR(accept(server_listen_fd, NULL, 0));
  if (accept_fd < 0)
    return false;
  if (fcntl(accept_fd, F_SETFL, O_NONBLOCK) == -1) {
    HANDLE_EINTR(close(accept_fd));
    return false;
  }

  *server_socket = accept_fd;
  return true;
}

bool ClientConnectToFifo(const std::string &pipe_name, int* client_socket) {
  DCHECK(client_socket);
  DCHECK_LT(pipe_name.length(), kMaxPipeNameLength);

  
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    LOG(ERROR) << "fd is invalid";
    return false;
  }

  
  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
    LOG(ERROR) << "fcntl failed";
    HANDLE_EINTR(close(fd));
    return false;
  }

  
  struct sockaddr_un  server_unix_addr;
  memset(&server_unix_addr, 0, sizeof(server_unix_addr));
  server_unix_addr.sun_family = AF_UNIX;
  snprintf(server_unix_addr.sun_path, kMaxPipeNameLength, "%s",
           pipe_name.c_str());
  size_t server_unix_addr_len = offsetof(struct sockaddr_un, sun_path) +
      strlen(server_unix_addr.sun_path) + 1;

  if (HANDLE_EINTR(connect(fd, reinterpret_cast<sockaddr*>(&server_unix_addr),
                           server_unix_addr_len)) != 0) {
    HANDLE_EINTR(close(fd));
    return false;
  }

  *client_socket = fd;
  return true;
}

}  


Channel::ChannelImpl::ChannelImpl(const std::wstring& channel_id, Mode mode,
                                  Listener* listener)
    : mode_(mode),
      is_blocked_on_write_(false),
      message_send_bytes_written_(0),
      uses_fifo_(CommandLine::ForCurrentProcess()->HasSwitch(
                     switches::kIPCUseFIFO)),
      server_listen_pipe_(-1),
      pipe_(-1),
      client_pipe_(-1),
      listener_(listener),
      waiting_connect_(true),
      processing_incoming_(false),
      factory_(this) {
  if (!CreatePipe(channel_id, mode)) {
    
    LOG(WARNING) << "Unable to create pipe named \"" << channel_id <<
                    "\" in " << (mode == MODE_SERVER ? "server" : "client") <<
                    " mode error(" << strerror(errno) << ").";
  }
}

bool Channel::ChannelImpl::CreatePipe(const std::wstring& channel_id,
                                      Mode mode) {
  DCHECK(server_listen_pipe_ == -1 && pipe_ == -1);

  if (uses_fifo_) {
    
    
    
    
    pipe_name_ = "/var/tmp/chrome_" + WideToASCII(channel_id);
    if (mode == MODE_SERVER) {
      if (!CreateServerFifo(pipe_name_, &server_listen_pipe_)) {
        return false;
      }
    } else {
      if (!ClientConnectToFifo(pipe_name_, &pipe_)) {
        return false;
      }
      waiting_connect_ = false;
    }
  } else {
    
    pipe_name_ = WideToASCII(channel_id);
    if (mode == MODE_SERVER) {
      int pipe_fds[2];
      if (socketpair(AF_UNIX, SOCK_STREAM, 0, pipe_fds) != 0) {
        return false;
      }
      
      if (fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK) == -1 ||
          fcntl(pipe_fds[1], F_SETFL, O_NONBLOCK) == -1) {
        HANDLE_EINTR(close(pipe_fds[0]));
        HANDLE_EINTR(close(pipe_fds[1]));
        return false;
      }
      pipe_ = pipe_fds[0];
      client_pipe_ = pipe_fds[1];

      Singleton<PipeMap>()->Insert(pipe_name_, client_pipe_);
    } else {
      pipe_ = ChannelNameToClientFD(pipe_name_);
      DCHECK(pipe_ > 0);
      waiting_connect_ = false;
    }
  }

  
  scoped_ptr<Message> msg(new Message(MSG_ROUTING_NONE,
                                      HELLO_MESSAGE_TYPE,
                                      IPC::Message::PRIORITY_NORMAL));
  if (!msg->WriteInt(base::GetCurrentProcId())) {
    Close();
    return false;
  }

  output_queue_.push(msg.release());
  return true;
}

bool Channel::ChannelImpl::Connect() {
  if (mode_ == MODE_SERVER && uses_fifo_) {
    if (server_listen_pipe_ == -1) {
      return false;
    }
    MessageLoopForIO::current()->WatchFileDescriptor(
        server_listen_pipe_,
        true,
        MessageLoopForIO::WATCH_READ,
        &server_listen_connection_watcher_,
        this);
  } else {
    if (pipe_ == -1) {
      return false;
    }
    MessageLoopForIO::current()->WatchFileDescriptor(
        pipe_,
        true,
        MessageLoopForIO::WATCH_READ,
        &read_watcher_,
        this);
    waiting_connect_ = false;
  }

  if (!waiting_connect_)
    return ProcessOutgoingMessages();
  return true;
}

bool Channel::ChannelImpl::ProcessIncomingMessages() {
  ssize_t bytes_read = 0;

  struct msghdr msg = {0};
  struct iovec iov = {input_buf_, Channel::kReadBufferSize};

  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = input_cmsg_buf_;

  for (;;) {
    msg.msg_controllen = sizeof(input_cmsg_buf_);

    if (bytes_read == 0) {
      if (pipe_ == -1)
        return false;

      
      
      
      bytes_read = HANDLE_EINTR(recvmsg(pipe_, &msg, MSG_DONTWAIT));

      if (bytes_read < 0) {
        if (errno == EAGAIN) {
          return true;
        } else {
          LOG(ERROR) << "pipe error (" << pipe_ << "): " << strerror(errno);
          return false;
        }
      } else if (bytes_read == 0) {
        
        Close();
        return false;
      }
    }
    DCHECK(bytes_read);

    if (client_pipe_ != -1) {
      Singleton<PipeMap>()->Remove(pipe_name_);
      HANDLE_EINTR(close(client_pipe_));
      client_pipe_ = -1;
    }

    
    const int* wire_fds = NULL;
    unsigned num_wire_fds = 0;

    
    

    
    
    
    
    
    
    
    
    
    
    
    if (msg.msg_controllen > 0) {
      
      
      for (struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg;
           cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET &&
            cmsg->cmsg_type == SCM_RIGHTS) {
          const unsigned payload_len = cmsg->cmsg_len - CMSG_LEN(0);
          DCHECK(payload_len % sizeof(int) == 0);
          wire_fds = reinterpret_cast<int*>(CMSG_DATA(cmsg));
          num_wire_fds = payload_len / 4;

          if (msg.msg_flags & MSG_CTRUNC) {
            LOG(ERROR) << "SCM_RIGHTS message was truncated"
                       << " cmsg_len:" << cmsg->cmsg_len
                       << " fd:" << pipe_;
            for (unsigned i = 0; i < num_wire_fds; ++i)
              HANDLE_EINTR(close(wire_fds[i]));
            return false;
          }
          break;
        }
      }
    }

    
    const char *p;
    const char *end;
    if (input_overflow_buf_.empty()) {
      p = input_buf_;
      end = p + bytes_read;
    } else {
      if (input_overflow_buf_.size() >
         static_cast<size_t>(kMaximumMessageSize - bytes_read)) {
        input_overflow_buf_.clear();
        LOG(ERROR) << "IPC message is too big";
        return false;
      }
      input_overflow_buf_.append(input_buf_, bytes_read);
      p = input_overflow_buf_.data();
      end = p + input_overflow_buf_.size();
    }

    
    
    const int* fds;
    unsigned num_fds;
    unsigned fds_i = 0;  

    if (input_overflow_fds_.empty()) {
      fds = wire_fds;
      num_fds = num_wire_fds;
    } else {
      const size_t prev_size = input_overflow_fds_.size();
      input_overflow_fds_.resize(prev_size + num_wire_fds);
      memcpy(&input_overflow_fds_[prev_size], wire_fds,
             num_wire_fds * sizeof(int));
      fds = &input_overflow_fds_[0];
      num_fds = input_overflow_fds_.size();
    }

    while (p < end) {
      const char* message_tail = Message::FindNext(p, end);
      if (message_tail) {
        int len = static_cast<int>(message_tail - p);
        Message m(p, len);
        if (m.header()->num_fds) {
          
          const char* error = NULL;
          if (m.header()->num_fds > num_fds - fds_i) {
            
            
            error = "Message needs unreceived descriptors";
          }

          if (m.header()->num_fds >
              FileDescriptorSet::MAX_DESCRIPTORS_PER_MESSAGE) {
            
            error = "Message requires an excessive number of descriptors";
          }

          if (error) {
            LOG(WARNING) << error
                         << " channel:" << this
                         << " message-type:" << m.type()
                         << " header()->num_fds:" << m.header()->num_fds
                         << " num_fds:" << num_fds
                         << " fds_i:" << fds_i;
            
            for (unsigned i = fds_i; i < num_fds; ++i)
              HANDLE_EINTR(close(fds[i]));
            input_overflow_fds_.clear();
            
            return false;
          }

          m.file_descriptor_set()->SetDescriptors(
              &fds[fds_i], m.header()->num_fds);
          fds_i += m.header()->num_fds;
        }
#ifdef IPC_MESSAGE_DEBUG_EXTRA
        DLOG(INFO) << "received message on channel @" << this <<
                      " with type " << m.type();
#endif
        if (m.routing_id() == MSG_ROUTING_NONE &&
            m.type() == HELLO_MESSAGE_TYPE) {
          
          listener_->OnChannelConnected(MessageIterator(m).NextInt());
        } else {
          listener_->OnMessageReceived(m);
        }
        p = message_tail;
      } else {
        
        break;
      }
    }
    input_overflow_buf_.assign(p, end - p);
    input_overflow_fds_ = std::vector<int>(&fds[fds_i], &fds[num_fds]);

    
    
    
    if (input_overflow_buf_.empty() && !input_overflow_fds_.empty()) {
      
      return false;
    }

    bytes_read = 0;  
  }

  return true;
}

bool Channel::ChannelImpl::ProcessOutgoingMessages() {
  DCHECK(!waiting_connect_);  
                              
  is_blocked_on_write_ = false;

  if (output_queue_.empty())
    return true;

  if (pipe_ == -1)
    return false;

  
  
  while (!output_queue_.empty()) {
    Message* msg = output_queue_.front();

    size_t amt_to_write = msg->size() - message_send_bytes_written_;
    DCHECK(amt_to_write != 0);
    const char *out_bytes = reinterpret_cast<const char*>(msg->data()) +
        message_send_bytes_written_;

    struct msghdr msgh = {0};
    struct iovec iov = {const_cast<char*>(out_bytes), amt_to_write};
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    char buf[CMSG_SPACE(
        sizeof(int[FileDescriptorSet::MAX_DESCRIPTORS_PER_MESSAGE]))];

    if (message_send_bytes_written_ == 0 &&
        !msg->file_descriptor_set()->empty()) {
      
      struct cmsghdr *cmsg;
      const unsigned num_fds = msg->file_descriptor_set()->size();

      DCHECK_LE(num_fds, FileDescriptorSet::MAX_DESCRIPTORS_PER_MESSAGE);

      msgh.msg_control = buf;
      msgh.msg_controllen = CMSG_SPACE(sizeof(int) * num_fds);
      cmsg = CMSG_FIRSTHDR(&msgh);
      cmsg->cmsg_level = SOL_SOCKET;
      cmsg->cmsg_type = SCM_RIGHTS;
      cmsg->cmsg_len = CMSG_LEN(sizeof(int) * num_fds);
      msg->file_descriptor_set()->GetDescriptors(
          reinterpret_cast<int*>(CMSG_DATA(cmsg)));
      msgh.msg_controllen = cmsg->cmsg_len;

      msg->header()->num_fds = num_fds;
    }

    ssize_t bytes_written = HANDLE_EINTR(sendmsg(pipe_, &msgh, MSG_DONTWAIT));
    if (bytes_written > 0)
      msg->file_descriptor_set()->CommitAll();

    if (bytes_written < 0 && errno != EAGAIN) {
      LOG(ERROR) << "pipe error: " << strerror(errno);
      return false;
    }

    if (static_cast<size_t>(bytes_written) != amt_to_write) {
      if (bytes_written > 0) {
        
        message_send_bytes_written_ += bytes_written;
      }

      
      is_blocked_on_write_ = true;
      MessageLoopForIO::current()->WatchFileDescriptor(
          pipe_,
          false,  
          MessageLoopForIO::WATCH_WRITE,
          &write_watcher_,
          this);
      return true;
    } else {
      message_send_bytes_written_ = 0;

      
#ifdef IPC_MESSAGE_DEBUG_EXTRA
      DLOG(INFO) << "sent message @" << msg << " on channel @" << this <<
                    " with type " << msg->type();
#endif
      output_queue_.pop();
      delete msg;
    }
  }
  return true;
}

bool Channel::ChannelImpl::Send(Message* message) {
  chrome::Counters::ipc_send_counter().Increment();
#ifdef IPC_MESSAGE_DEBUG_EXTRA
  DLOG(INFO) << "sending message @" << message << " on channel @" << this
             << " with type " << message->type()
             << " (" << output_queue_.size() << " in queue)";
#endif

#ifdef IPC_MESSAGE_LOG_ENABLED
  Logging::current()->OnSendMessage(message, L"");
#endif

  output_queue_.push(message);
  if (!waiting_connect_) {
    if (!is_blocked_on_write_) {
      if (!ProcessOutgoingMessages())
        return false;
    }
  }

  return true;
}

void Channel::ChannelImpl::GetClientFileDescriptorMapping(int *src_fd,
                                                          int *dest_fd) const {
  DCHECK(mode_ == MODE_SERVER);
  *src_fd = client_pipe_;
  *dest_fd = kClientChannelFd;
}


void Channel::ChannelImpl::OnFileCanReadWithoutBlocking(int fd) {
  bool send_server_hello_msg = false;
  if (waiting_connect_ && mode_ == MODE_SERVER) {
    
    
    DCHECK(uses_fifo_);

    if (!ServerAcceptFifoConnection(server_listen_pipe_, &pipe_)) {
      Close();
    }

    
    
    server_listen_connection_watcher_.StopWatchingFileDescriptor();

    
    MessageLoopForIO::current()->WatchFileDescriptor(
        pipe_,
        true,
        MessageLoopForIO::WATCH_READ,
        &read_watcher_,
        this);

    waiting_connect_ = false;
    send_server_hello_msg = true;
  }

  if (!waiting_connect_ && fd == pipe_) {
    if (!ProcessIncomingMessages()) {
      Close();
      listener_->OnChannelError();
    }
  }

  
  
  
  
  if (send_server_hello_msg) {
    
    DCHECK(is_blocked_on_write_ == false);
    ProcessOutgoingMessages();
  }
}


void Channel::ChannelImpl::OnFileCanWriteWithoutBlocking(int fd) {
  if (!ProcessOutgoingMessages()) {
    Close();
    listener_->OnChannelError();
  }
}

void Channel::ChannelImpl::Close() {
  
  

  
  server_listen_connection_watcher_.StopWatchingFileDescriptor();

  if (server_listen_pipe_ != -1) {
    HANDLE_EINTR(close(server_listen_pipe_));
    server_listen_pipe_ = -1;
  }

  
  read_watcher_.StopWatchingFileDescriptor();
  write_watcher_.StopWatchingFileDescriptor();
  if (pipe_ != -1) {
    HANDLE_EINTR(close(pipe_));
    pipe_ = -1;
  }
  if (client_pipe_ != -1) {
    Singleton<PipeMap>()->Remove(pipe_name_);
    HANDLE_EINTR(close(client_pipe_));
    client_pipe_ = -1;
  }

  if (uses_fifo_) {
    
    unlink(pipe_name_.c_str());
  }

  while (!output_queue_.empty()) {
    Message* m = output_queue_.front();
    output_queue_.pop();
    delete m;
  }

  
  for (std::vector<int>::iterator
       i = input_overflow_fds_.begin(); i != input_overflow_fds_.end(); ++i) {
    HANDLE_EINTR(close(*i));
  }
  input_overflow_fds_.clear();
}



Channel::Channel(const std::wstring& channel_id, Mode mode,
                 Listener* listener)
    : channel_impl_(new ChannelImpl(channel_id, mode, listener)) {
}

Channel::~Channel() {
  delete channel_impl_;
}

bool Channel::Connect() {
  return channel_impl_->Connect();
}

void Channel::Close() {
  channel_impl_->Close();
}

void Channel::set_listener(Listener* listener) {
  channel_impl_->set_listener(listener);
}

bool Channel::Send(Message* message) {
  return channel_impl_->Send(message);
}

void Channel::GetClientFileDescriptorMapping(int *src_fd, int *dest_fd) const {
  return channel_impl_->GetClientFileDescriptorMapping(src_fd, dest_fd);
}

}  
