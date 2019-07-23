



#include "base/message_loop.h"
#include "base/thread.h"
#include "chrome/common/ipc_channel_proxy.h"
#include "chrome/common/ipc_logging.h"
#include "chrome/common/ipc_message_utils.h"

namespace IPC {



ChannelProxy::Context::Context(Channel::Listener* listener,
                               MessageFilter* filter,
                               MessageLoop* ipc_message_loop)
    : listener_message_loop_(MessageLoop::current()),
      listener_(listener),
      ipc_message_loop_(ipc_message_loop),
      channel_(NULL),
      peer_pid_(0),
      channel_connected_called_(false) {
  if (filter)
    filters_.push_back(filter);
}

void ChannelProxy::Context::CreateChannel(const std::wstring& id,
                                          const Channel::Mode& mode) {
  DCHECK(channel_ == NULL);
  channel_id_ = id;
  channel_ = new Channel(id, mode, this);
}

bool ChannelProxy::Context::TryFilters(const Message& message) {
#ifdef IPC_MESSAGE_LOG_ENABLED
  Logging* logger = Logging::current();
  if (logger->Enabled())
    logger->OnPreDispatchMessage(message);
#endif

  for (size_t i = 0; i < filters_.size(); ++i) {
    if (filters_[i]->OnMessageReceived(message)) {
#ifdef IPC_MESSAGE_LOG_ENABLED
      if (logger->Enabled())
        logger->OnPostDispatchMessage(message, channel_id_);
#endif
      return true;
    }
  }
  return false;
}


void ChannelProxy::Context::OnMessageReceived(const Message& message) {
  
  if (!TryFilters(message))
    OnMessageReceivedNoFilter(message);
}


void ChannelProxy::Context::OnMessageReceivedNoFilter(const Message& message) {
  
  
  
  
  listener_message_loop_->PostTask(FROM_HERE, NewRunnableMethod(
      this, &Context::OnDispatchMessage, message));
}


void ChannelProxy::Context::OnChannelConnected(int32 peer_pid) {
  peer_pid_ = peer_pid;
  for (size_t i = 0; i < filters_.size(); ++i)
    filters_[i]->OnChannelConnected(peer_pid);

  
  listener_message_loop_->PostTask(FROM_HERE, NewRunnableMethod(
      this, &Context::OnDispatchConnected));
}


void ChannelProxy::Context::OnChannelError() {
  for (size_t i = 0; i < filters_.size(); ++i)
    filters_[i]->OnChannelError();

  
  listener_message_loop_->PostTask(FROM_HERE, NewRunnableMethod(
      this, &Context::OnDispatchError));
}


void ChannelProxy::Context::OnChannelOpened() {
  DCHECK(channel_ != NULL);

  
  
  AddRef();

  if (!channel_->Connect()) {
    OnChannelError();
    return;
  }

  for (size_t i = 0; i < filters_.size(); ++i)
    filters_[i]->OnFilterAdded(channel_);
}


void ChannelProxy::Context::OnChannelClosed() {
  
  
  if (!channel_)
    return;

  for (size_t i = 0; i < filters_.size(); ++i) {
    filters_[i]->OnChannelClosing();
    filters_[i]->OnFilterRemoved();
  }

  
  filters_.clear();

  delete channel_;
  channel_ = NULL;

  
  
  Release();
}


void ChannelProxy::Context::OnSendMessage(Message* message) {
  if (!channel_->Send(message))
    OnChannelError();
}


void ChannelProxy::Context::OnAddFilter(MessageFilter* filter) {
  filters_.push_back(filter);

  
  
  if (channel_)
    filter->OnFilterAdded(channel_);

  
  filter->Release();
}


void ChannelProxy::Context::OnRemoveFilter(MessageFilter* filter) {
  for (size_t i = 0; i < filters_.size(); ++i) {
    if (filters_[i].get() == filter) {
      filter->OnFilterRemoved();
      filters_.erase(filters_.begin() + i);
      return;
    }
  }

  NOTREACHED() << "filter to be removed not found";
}


void ChannelProxy::Context::OnDispatchMessage(const Message& message) {
  if (!listener_)
    return;

  OnDispatchConnected();

#ifdef IPC_MESSAGE_LOG_ENABLED
  Logging* logger = Logging::current();
  if (message.type() == IPC_LOGGING_ID) {
    logger->OnReceivedLoggingMessage(message);
    return;
  }

  if (logger->Enabled())
    logger->OnPreDispatchMessage(message);
#endif

  listener_->OnMessageReceived(message);

#ifdef IPC_MESSAGE_LOG_ENABLED
  if (logger->Enabled())
    logger->OnPostDispatchMessage(message, channel_id_);
#endif
}


void ChannelProxy::Context::OnDispatchConnected() {
  if (channel_connected_called_)
    return;

  channel_connected_called_ = true;
  if (listener_)
    listener_->OnChannelConnected(peer_pid_);
}


void ChannelProxy::Context::OnDispatchError() {
  if (listener_)
    listener_->OnChannelError();
}



ChannelProxy::ChannelProxy(const std::wstring& channel_id, Channel::Mode mode,
                           Channel::Listener* listener, MessageFilter* filter,
                           MessageLoop* ipc_thread)
    : context_(new Context(listener, filter, ipc_thread)) {
  Init(channel_id, mode, ipc_thread, true);
}

ChannelProxy::ChannelProxy(const std::wstring& channel_id, Channel::Mode mode,
                           MessageLoop* ipc_thread, Context* context,
                           bool create_pipe_now)
    : context_(context) {
  Init(channel_id, mode, ipc_thread, create_pipe_now);
}

void ChannelProxy::Init(const std::wstring& channel_id, Channel::Mode mode,
                        MessageLoop* ipc_thread_loop, bool create_pipe_now) {
  if (create_pipe_now) {
    
    
    
    
    context_->CreateChannel(channel_id, mode);
  } else {
#if defined(OS_POSIX)
    
    
    
    
    
    
    
    
    
    
    NOTIMPLEMENTED();
#endif  
    context_->ipc_message_loop()->PostTask(FROM_HERE, NewRunnableMethod(
        context_.get(), &Context::CreateChannel, channel_id, mode));
  }

  
  context_->ipc_message_loop()->PostTask(FROM_HERE, NewRunnableMethod(
      context_.get(), &Context::OnChannelOpened));
}

void ChannelProxy::Close() {
  
  
  
  context_->Clear();

  if (context_->ipc_message_loop()) {
    context_->ipc_message_loop()->PostTask(FROM_HERE, NewRunnableMethod(
        context_.get(), &Context::OnChannelClosed));
  }
}

bool ChannelProxy::Send(Message* message) {
#ifdef IPC_MESSAGE_LOG_ENABLED
  Logging::current()->OnSendMessage(message, context_->channel_id());
#endif

  context_->ipc_message_loop()->PostTask(FROM_HERE, NewRunnableMethod(
      context_.get(), &Context::OnSendMessage, message));
  return true;
}

void ChannelProxy::AddFilter(MessageFilter* filter) {
  
  
  filter->AddRef();
  context_->ipc_message_loop()->PostTask(FROM_HERE, NewRunnableMethod(
      context_.get(), &Context::OnAddFilter, filter));
}

void ChannelProxy::RemoveFilter(MessageFilter* filter) {
  context_->ipc_message_loop()->PostTask(FROM_HERE, NewRunnableMethod(
      context_.get(), &Context::OnRemoveFilter, filter));
}

#if defined(OS_POSIX)



void ChannelProxy::GetClientFileDescriptorMapping(int *src_fd,
                                                  int *dest_fd) const {
  Channel *channel = context_.get()->channel_;
  DCHECK(channel); 
  channel->GetClientFileDescriptorMapping(src_fd, dest_fd);
}
#endif



}  
