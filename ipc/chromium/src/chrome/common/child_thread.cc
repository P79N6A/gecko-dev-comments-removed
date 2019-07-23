



#include "chrome/common/child_thread.h"

#include "base/string_util.h"
#include "base/command_line.h"
#include "chrome/common/child_process.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/ipc_logging.h"
#ifndef CHROMIUM_MOZILLA_BUILD
#include "chrome/common/plugin_messages.h"
#include "webkit/glue/webkit_glue.h"
#endif


const size_t ChildThread::kV8StackSize = 1024 * 1024;

ChildThread::ChildThread(Thread::Options options)
    : Thread("Chrome_ChildThread"),
      owner_loop_(MessageLoop::current()),
      options_(options),
      check_with_browser_before_shutdown_(false) {
  DCHECK(owner_loop_);
  channel_name_ = CommandLine::ForCurrentProcess()->GetSwitchValue(
      switches::kProcessChannelID);

  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kUserAgent)) {
#ifndef CHROMIUM_MOZILLA_BUILD
    webkit_glue::SetUserAgent(WideToUTF8(
        CommandLine::ForCurrentProcess()->GetSwitchValue(
            switches::kUserAgent)));
#endif
  }
}

ChildThread::~ChildThread() {
}

bool ChildThread::Run() {
  return StartWithOptions(options_);
}

void ChildThread::OnChannelError() {
  owner_loop_->PostTask(FROM_HERE, new MessageLoop::QuitTask());
}

bool ChildThread::Send(IPC::Message* msg) {
  if (!channel_.get()) {
    delete msg;
    return false;
  }

  return channel_->Send(msg);
}

void ChildThread::AddRoute(int32 routing_id, IPC::Channel::Listener* listener) {
  DCHECK(MessageLoop::current() == message_loop());

  router_.AddRoute(routing_id, listener);
}

void ChildThread::RemoveRoute(int32 routing_id) {
  DCHECK(MessageLoop::current() == message_loop());

  router_.RemoveRoute(routing_id);
}

void ChildThread::OnMessageReceived(const IPC::Message& msg) {
#ifndef CHROMIUM_MOZILLA_BUILD
  
  if (resource_dispatcher_->OnMessageReceived(msg))
    return;

  if (msg.type() == PluginProcessMsg_AskBeforeShutdown::ID) {
    check_with_browser_before_shutdown_ = true;
    return;
  }

  if (msg.type() == PluginProcessMsg_Shutdown::ID) {
    owner_loop_->PostTask(FROM_HERE, new MessageLoop::QuitTask());
    return;
  }
#endif

  if (msg.routing_id() == MSG_ROUTING_CONTROL) {
    OnControlMessageReceived(msg);
  } else {
    router_.OnMessageReceived(msg);
  }
}

ChildThread* ChildThread::current() {
  return ChildProcess::current()->child_thread();
}

void ChildThread::Init() {
#ifndef CHROMIUM_MOZILLA_BUILD
  channel_.reset(new IPC::SyncChannel(channel_name_,
      IPC::Channel::MODE_CLIENT, this, NULL, owner_loop_, true,
      ChildProcess::current()->GetShutDownEvent()));
#else
  channel_.reset(new IPC::Channel(channel_name_,
                                  IPC::Channel::MODE_CLIENT,
                                  this));
#endif

#ifdef IPC_MESSAGE_LOG_ENABLED
  IPC::Logging::current()->SetIPCSender(this);
#endif

#ifndef CHROMIUM_MOZILLA_BUILD
  resource_dispatcher_.reset(new ResourceDispatcher(this));
#endif
}

void ChildThread::CleanUp() {
#ifdef IPC_MESSAGE_LOG_ENABLED
  IPC::Logging::current()->SetIPCSender(NULL);
#endif
  
  
  channel_.reset();
#ifndef CHROMIUM_MOZILLA_BUILD
  resource_dispatcher_.reset();
#endif
}

void ChildThread::OnProcessFinalRelease() {
  if (!check_with_browser_before_shutdown_) {
    owner_loop_->PostTask(FROM_HERE, new MessageLoop::QuitTask());
    return;
  }

#ifndef CHROMIUM_MOZILLA_BUILD
  
  
  
  
  Send(new PluginProcessHostMsg_ShutdownRequest);
#endif
}
