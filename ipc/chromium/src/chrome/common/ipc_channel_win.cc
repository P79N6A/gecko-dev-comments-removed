



#include "chrome/common/ipc_channel_win.h"

#include <windows.h>
#include <sstream>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/non_thread_safe.h"
#include "base/stats_counters.h"
#include "base/win_util.h"
#include "chrome/common/chrome_counters.h"
#include "chrome/common/ipc_logging.h"
#include "chrome/common/ipc_message_utils.h"

namespace IPC {


Channel::ChannelImpl::State::State(ChannelImpl* channel) : is_pending(false) {
  memset(&context.overlapped, 0, sizeof(context.overlapped));
  context.handler = channel;
}

Channel::ChannelImpl::State::~State() {
  COMPILE_ASSERT(!offsetof(Channel::ChannelImpl::State, context),
                 starts_with_io_context);
}



Channel::ChannelImpl::ChannelImpl(const std::wstring& channel_id, Mode mode,
                              Listener* listener)
    : ALLOW_THIS_IN_INITIALIZER_LIST(input_state_(this)),
      ALLOW_THIS_IN_INITIALIZER_LIST(output_state_(this)),
      pipe_(INVALID_HANDLE_VALUE),
      listener_(listener),
      waiting_connect_(mode == MODE_SERVER),
      processing_incoming_(false),
      ALLOW_THIS_IN_INITIALIZER_LIST(factory_(this)) {
  if (!CreatePipe(channel_id, mode)) {
    
    LOG(WARNING) << "Unable to create pipe named \"" << channel_id <<
                    "\" in " << (mode == 0 ? "server" : "client") << " mode.";
  }
}

void Channel::ChannelImpl::Close() {
  if (thread_check_.get()) {
    DCHECK(thread_check_->CalledOnValidThread());
  }

  bool waited = false;
  if (input_state_.is_pending || output_state_.is_pending) {
    CancelIo(pipe_);
    waited = true;
  }

  
  
  if (pipe_ != INVALID_HANDLE_VALUE) {
    CloseHandle(pipe_);
    pipe_ = INVALID_HANDLE_VALUE;
  }

  
  base::Time start = base::Time::Now();
  while (input_state_.is_pending || output_state_.is_pending) {
    MessageLoopForIO::current()->WaitForIOCompletion(INFINITE, this);
  }
  if (waited) {
    
    UMA_HISTOGRAM_TIMES("AsyncIO.IPCChannelClose", base::Time::Now() - start);
  }

  while (!output_queue_.empty()) {
    Message* m = output_queue_.front();
    output_queue_.pop();
    delete m;
  }
}

bool Channel::ChannelImpl::Send(Message* message) {
  DCHECK(thread_check_->CalledOnValidThread());
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
    if (!output_state_.is_pending) {
      if (!ProcessOutgoingMessages(NULL, 0))
        return false;
    }
  }

  return true;
}

const std::wstring Channel::ChannelImpl::PipeName(
    const std::wstring& channel_id) const {
  std::wostringstream ss;
  
  ss << L"\\\\.\\pipe\\chrome." << channel_id;
  return ss.str();
}

bool Channel::ChannelImpl::CreatePipe(const std::wstring& channel_id,
                                      Mode mode) {
  DCHECK(pipe_ == INVALID_HANDLE_VALUE);
  const std::wstring pipe_name = PipeName(channel_id);
  if (mode == MODE_SERVER) {
    SECURITY_ATTRIBUTES security_attributes = {0};
    security_attributes.bInheritHandle = FALSE;
    security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    if (!win_util::GetLogonSessionOnlyDACL(
        reinterpret_cast<SECURITY_DESCRIPTOR**>(
            &security_attributes.lpSecurityDescriptor))) {
      NOTREACHED();
    }

    pipe_ = CreateNamedPipeW(pipe_name.c_str(),
                             PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |
                                FILE_FLAG_FIRST_PIPE_INSTANCE,
                             PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                             1,         
                             
                             Channel::kReadBufferSize,
                             
                             Channel::kReadBufferSize,
                             5000,      
                             &security_attributes);
    LocalFree(security_attributes.lpSecurityDescriptor);
  } else {
    pipe_ = CreateFileW(pipe_name.c_str(),
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION |
                            FILE_FLAG_OVERLAPPED,
                        NULL);
  }
  if (pipe_ == INVALID_HANDLE_VALUE) {
    
    LOG(WARNING) << "failed to create pipe: " << GetLastError();
    return false;
  }

  
  scoped_ptr<Message> m(new Message(MSG_ROUTING_NONE,
                                    HELLO_MESSAGE_TYPE,
                                    IPC::Message::PRIORITY_NORMAL));
  if (!m->WriteInt(GetCurrentProcessId())) {
    CloseHandle(pipe_);
    pipe_ = INVALID_HANDLE_VALUE;
    return false;
  }

  output_queue_.push(m.release());
  return true;
}

bool Channel::ChannelImpl::Connect() {
  DLOG(WARNING) << "Connect called twice";

  if (!thread_check_.get())
    thread_check_.reset(new NonThreadSafe());

  if (pipe_ == INVALID_HANDLE_VALUE)
    return false;

  MessageLoopForIO::current()->RegisterIOHandler(pipe_, this);

  
  if (waiting_connect_)
    ProcessConnection();

  if (!input_state_.is_pending) {
    
    
    
    MessageLoopForIO::current()->PostTask(FROM_HERE, factory_.NewRunnableMethod(
        &Channel::ChannelImpl::OnIOCompleted, &input_state_.context, 0, 0));
  }

  if (!waiting_connect_)
    ProcessOutgoingMessages(NULL, 0);
  return true;
}

bool Channel::ChannelImpl::ProcessConnection() {
  DCHECK(thread_check_->CalledOnValidThread());
  if (input_state_.is_pending)
    input_state_.is_pending = false;

  
  if (INVALID_HANDLE_VALUE == pipe_)
    return false;

  BOOL ok = ConnectNamedPipe(pipe_, &input_state_.context.overlapped);

  DWORD err = GetLastError();
  if (ok) {
    
    
    NOTREACHED();
    return false;
  }

  switch (err) {
  case ERROR_IO_PENDING:
    input_state_.is_pending = true;
    break;
  case ERROR_PIPE_CONNECTED:
    waiting_connect_ = false;
    break;
  default:
    NOTREACHED();
    return false;
  }

  return true;
}

bool Channel::ChannelImpl::ProcessIncomingMessages(
    MessageLoopForIO::IOContext* context,
    DWORD bytes_read) {
  DCHECK(thread_check_->CalledOnValidThread());
  if (input_state_.is_pending) {
    input_state_.is_pending = false;
    DCHECK(context);

    if (!context || !bytes_read)
      return false;
  } else {
    
    DCHECK(!bytes_read && context == &input_state_.context);
  }

  for (;;) {
    if (bytes_read == 0) {
      if (INVALID_HANDLE_VALUE == pipe_)
        return false;

      
      BOOL ok = ReadFile(pipe_,
                         input_buf_,
                         Channel::kReadBufferSize,
                         &bytes_read,
                         &input_state_.context.overlapped);
      if (!ok) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
          input_state_.is_pending = true;
          return true;
        }
        LOG(ERROR) << "pipe error: " << err;
        return false;
      }
      input_state_.is_pending = true;
      return true;
    }
    DCHECK(bytes_read);

    

    const char* p, *end;
    if (input_overflow_buf_.empty()) {
      p = input_buf_;
      end = p + bytes_read;
    } else {
      if (input_overflow_buf_.size() > (kMaximumMessageSize - bytes_read)) {
        input_overflow_buf_.clear();
        LOG(ERROR) << "IPC message is too big";
        return false;
      }
      input_overflow_buf_.append(input_buf_, bytes_read);
      p = input_overflow_buf_.data();
      end = p + input_overflow_buf_.size();
    }

    while (p < end) {
      const char* message_tail = Message::FindNext(p, end);
      if (message_tail) {
        int len = static_cast<int>(message_tail - p);
        const Message m(p, len);
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

    bytes_read = 0;  
  }

  return true;
}

bool Channel::ChannelImpl::ProcessOutgoingMessages(
    MessageLoopForIO::IOContext* context,
    DWORD bytes_written) {
  DCHECK(!waiting_connect_);  
                              
  DCHECK(thread_check_->CalledOnValidThread());

  if (output_state_.is_pending) {
    DCHECK(context);
    output_state_.is_pending = false;
    if (!context || bytes_written == 0) {
      DWORD err = GetLastError();
      LOG(ERROR) << "pipe error: " << err;
      return false;
    }
    
    DCHECK(!output_queue_.empty());
    Message* m = output_queue_.front();
    output_queue_.pop();
    delete m;
  }

  if (output_queue_.empty())
    return true;

  if (INVALID_HANDLE_VALUE == pipe_)
    return false;

  
  Message* m = output_queue_.front();
  BOOL ok = WriteFile(pipe_,
                      m->data(),
                      m->size(),
                      &bytes_written,
                      &output_state_.context.overlapped);
  if (!ok) {
    DWORD err = GetLastError();
    if (err == ERROR_IO_PENDING) {
      output_state_.is_pending = true;

#ifdef IPC_MESSAGE_DEBUG_EXTRA
      DLOG(INFO) << "sent pending message @" << m << " on channel @" <<
                    this << " with type " << m->type();
#endif

      return true;
    }
    LOG(ERROR) << "pipe error: " << err;
    return false;
  }

#ifdef IPC_MESSAGE_DEBUG_EXTRA
  DLOG(INFO) << "sent message @" << m << " on channel @" << this <<
                " with type " << m->type();
#endif

  output_state_.is_pending = true;
  return true;
}

void Channel::ChannelImpl::OnIOCompleted(MessageLoopForIO::IOContext* context,
                            DWORD bytes_transfered, DWORD error) {
  bool ok;
  DCHECK(thread_check_->CalledOnValidThread());
  if (context == &input_state_.context) {
    if (waiting_connect_) {
      if (!ProcessConnection())
        return;
      
      if (!output_queue_.empty() && !output_state_.is_pending)
        ProcessOutgoingMessages(NULL, 0);
      if (input_state_.is_pending)
        return;
      
    }
    
    DCHECK(!processing_incoming_);
    processing_incoming_ = true;
    ok = ProcessIncomingMessages(context, bytes_transfered);
    processing_incoming_ = false;
  } else {
    DCHECK(context == &output_state_.context);
    ok = ProcessOutgoingMessages(context, bytes_transfered);
  }
  if (!ok && INVALID_HANDLE_VALUE != pipe_) {
    
    Close();
    listener_->OnChannelError();
  }
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

}  
