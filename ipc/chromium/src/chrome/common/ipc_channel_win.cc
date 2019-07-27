



#include "chrome/common/ipc_channel_win.h"

#include <windows.h>
#include <sstream>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/process_util.h"
#include "base/rand_util.h"
#include "base/string_util.h"
#include "base/non_thread_safe.h"
#include "base/win_util.h"
#include "chrome/common/ipc_message_utils.h"
#include "mozilla/ipc/ProtocolUtils.h"

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
      ALLOW_THIS_IN_INITIALIZER_LIST(factory_(this)),
      shared_secret_(0),
      waiting_for_shared_secret_(false) {
  Init(mode, listener);

  if (!CreatePipe(channel_id, mode)) {
    
    CHROMIUM_LOG(WARNING) << "Unable to create pipe named \"" << channel_id <<
                             "\" in " << (mode == 0 ? "server" : "client") << " mode.";
  }
}

Channel::ChannelImpl::ChannelImpl(const std::wstring& channel_id,
                                  HANDLE server_pipe,
                                  Mode mode, Listener* listener)
    : ALLOW_THIS_IN_INITIALIZER_LIST(input_state_(this)),
      ALLOW_THIS_IN_INITIALIZER_LIST(output_state_(this)),
      ALLOW_THIS_IN_INITIALIZER_LIST(factory_(this)),
      shared_secret_(0),
      waiting_for_shared_secret_(false) {
  Init(mode, listener);

  if (mode == MODE_SERVER) {
    
    pipe_ = server_pipe;
    EnqueueHelloMessage();
  } else {
    
    CreatePipe(channel_id, mode);
  }
}

void Channel::ChannelImpl::Init(Mode mode, Listener* listener) {
  pipe_ = INVALID_HANDLE_VALUE;
  listener_ = listener;
  waiting_connect_ = (mode == MODE_SERVER);
  processing_incoming_ = false;
  closed_ = false;
  output_queue_length_ = 0;
}

void Channel::ChannelImpl::OutputQueuePush(Message* msg)
{
  output_queue_.push(msg);
  output_queue_length_++;
}

void Channel::ChannelImpl::OutputQueuePop()
{
  output_queue_.pop();
  output_queue_length_--;
}

HANDLE Channel::ChannelImpl::GetServerPipeHandle() const {
  return pipe_;
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

  while (input_state_.is_pending || output_state_.is_pending) {
    MessageLoopForIO::current()->WaitForIOCompletion(INFINITE, this);
  }

  while (!output_queue_.empty()) {
    Message* m = output_queue_.front();
    OutputQueuePop();
    delete m;
  }

  if (thread_check_.get())
    thread_check_.reset();

  closed_ = true;
}

bool Channel::ChannelImpl::Send(Message* message) {
  if (thread_check_.get()) {
    DCHECK(thread_check_->CalledOnValidThread());
  }
#ifdef IPC_MESSAGE_DEBUG_EXTRA
  DLOG(INFO) << "sending message @" << message << " on channel @" << this
             << " with type " << message->type()
             << " (" << output_queue_.size() << " in queue)";
#endif


  if (closed_) {
    if (mozilla::ipc::LoggingEnabled()) {
      fprintf(stderr, "Can't send message %s, because this channel is closed.\n",
              message->name());
    }
    delete message;
    return false;
  }

  OutputQueuePush(message);
  
  if (!waiting_connect_) {
    if (!output_state_.is_pending) {
      if (!ProcessOutgoingMessages(NULL, 0))
        return false;
    }
  }

  return true;
}

const std::wstring Channel::ChannelImpl::PipeName(
    const std::wstring& channel_id, int32_t* secret) const {
  MOZ_ASSERT(secret);

  std::wostringstream ss;
  ss << L"\\\\.\\pipe\\chrome.";

  
  size_t index = channel_id.find_first_of(L'\\');
  if (index != std::string::npos) {
    StringToInt(channel_id.substr(index + 1), secret);
    ss << channel_id.substr(0, index - 1);
  } else {
    
    *secret = 0;
    ss << channel_id;
  }
  return ss.str();
}

bool Channel::ChannelImpl::CreatePipe(const std::wstring& channel_id,
                                      Mode mode) {
  DCHECK(pipe_ == INVALID_HANDLE_VALUE);
  const std::wstring pipe_name = PipeName(channel_id, &shared_secret_);
  if (mode == MODE_SERVER) {
    waiting_for_shared_secret_ = !!shared_secret_;
    pipe_ = CreateNamedPipeW(pipe_name.c_str(),
                             PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |
                                FILE_FLAG_FIRST_PIPE_INSTANCE,
                             PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                             1,         
                             
                             Channel::kReadBufferSize,
                             
                             Channel::kReadBufferSize,
                             5000,      
                             NULL);
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
    
    CHROMIUM_LOG(WARNING) << "failed to create pipe: " << GetLastError();
    return false;
  }

  
  return EnqueueHelloMessage();
}

bool Channel::ChannelImpl::EnqueueHelloMessage() {
  mozilla::UniquePtr<Message> m = mozilla::MakeUnique<Message>(MSG_ROUTING_NONE,
                                                               HELLO_MESSAGE_TYPE,
                                                               IPC::Message::PRIORITY_NORMAL);

  
  
  int32_t secret = waiting_for_shared_secret_ ? 0 : shared_secret_;

  
  if (!m->WriteInt(GetCurrentProcessId()) ||
      (secret && !m->WriteUInt32(secret)))
  {
    CloseHandle(pipe_);
    pipe_ = INVALID_HANDLE_VALUE;
    return false;
  }

  OutputQueuePush(m.release());
  return true;
}

bool Channel::ChannelImpl::Connect() {
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
        CHROMIUM_LOG(ERROR) << "pipe error: " << err;
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
        CHROMIUM_LOG(ERROR) << "IPC message is too big";
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
          
          
          MessageIterator it = MessageIterator(m);
          int32_t claimed_pid = it.NextInt();
          if (waiting_for_shared_secret_ && (it.NextInt() != shared_secret_)) {
            NOTREACHED();
            
            Close();
            listener_->OnChannelError();
            return false;
          }
          waiting_for_shared_secret_ = false;
          listener_->OnChannelConnected(claimed_pid);
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
      CHROMIUM_LOG(ERROR) << "pipe error: " << err;
      return false;
    }
    
    DCHECK(!output_queue_.empty());
    Message* m = output_queue_.front();
    OutputQueuePop();
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
    CHROMIUM_LOG(ERROR) << "pipe error: " << err;
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

bool Channel::ChannelImpl::Unsound_IsClosed() const
{
  return closed_;
}

uint32_t Channel::ChannelImpl::Unsound_NumQueuedMessages() const
{
  return output_queue_length_;
}



Channel::Channel(const std::wstring& channel_id, Mode mode,
                 Listener* listener)
    : channel_impl_(new ChannelImpl(channel_id, mode, listener)) {
}

Channel::Channel(const std::wstring& channel_id, void* server_pipe,
                 Mode mode, Listener* listener)
   : channel_impl_(new ChannelImpl(channel_id, server_pipe, mode, listener)) {
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

void* Channel::GetServerPipeHandle() const {
  return channel_impl_->GetServerPipeHandle();
}

Channel::Listener* Channel::set_listener(Listener* listener) {
  return channel_impl_->set_listener(listener);
}

bool Channel::Send(Message* message) {
  return channel_impl_->Send(message);
}

bool Channel::Unsound_IsClosed() const {
  return channel_impl_->Unsound_IsClosed();
}

uint32_t Channel::Unsound_NumQueuedMessages() const {
  return channel_impl_->Unsound_NumQueuedMessages();
}


std::wstring Channel::GenerateVerifiedChannelID(const std::wstring& prefix) {
  
  
  
  
  std::wstring id = prefix;
  if (!id.empty())
    id.append(L".");
  int secret;
  do {  
    secret = base::RandInt(0, std::numeric_limits<int>::max());
  } while (secret == 0);
  id.append(GenerateUniqueRandomChannelID());
  return id.append(StringPrintf(L"\\%d", secret));
}

}  
