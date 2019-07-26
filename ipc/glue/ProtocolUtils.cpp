






#include "base/process_util.h"

#include "mozilla/ipc/AsyncChannel.h"
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/ipc/Transport.h"

using namespace base;
using namespace IPC;

namespace mozilla {
namespace ipc {

class ChannelOpened : public IPC::Message
{
public:
  ChannelOpened(TransportDescriptor aDescriptor,
                ProcessId aOtherProcess,
                ProtocolId aProtocol)
    : IPC::Message(MSG_ROUTING_CONTROL, 
                   CHANNEL_OPENED_MESSAGE_TYPE,
                   PRIORITY_NORMAL)
  {
    IPC::WriteParam(this, aDescriptor);
    IPC::WriteParam(this, aOtherProcess);
    IPC::WriteParam(this, static_cast<uint32_t>(aProtocol));
  }

  static bool Read(const IPC::Message& aMsg,
                   TransportDescriptor* aDescriptor,
                   ProcessId* aOtherProcess,
                   ProtocolId* aProtocol)
  {
    void* iter = nullptr;
    if (!IPC::ReadParam(&aMsg, &iter, aDescriptor) ||
        !IPC::ReadParam(&aMsg, &iter, aOtherProcess) ||
        !IPC::ReadParam(&aMsg, &iter, reinterpret_cast<uint32_t*>(aProtocol))) {
      return false;
    }
    aMsg.EndRead(iter);
    return true;
  }
};

bool
Bridge(const PrivateIPDLInterface&,
       AsyncChannel* aParentChannel, ProcessHandle aParentProcess,
       AsyncChannel* aChildChannel, ProcessHandle aChildProcess,
       ProtocolId aProtocol, ProtocolId aChildProtocol)
{
  ProcessId parentId = GetProcId(aParentProcess);
  ProcessId childId = GetProcId(aChildProcess);
  if (!parentId || !childId) {
    return false;
  }

  TransportDescriptor parentSide, childSide;
  if (!CreateTransport(aParentProcess, aChildProcess,
                       &parentSide, &childSide)) {
    return false;
  }

  if (!aParentChannel->Send(new ChannelOpened(parentSide,
                                              childId,
                                              aProtocol)) ||
      !aChildChannel->Send(new ChannelOpened(childSide,
                                             parentId,
                                             aChildProtocol))) {
    CloseDescriptor(parentSide);
    CloseDescriptor(childSide);
    return false;
  }
  return true;
}

bool
Open(const PrivateIPDLInterface&,
     AsyncChannel* aOpenerChannel, ProcessHandle aOtherProcess,
     Transport::Mode aOpenerMode,
     ProtocolId aProtocol, ProtocolId aChildProtocol)
{
  bool isParent = (Transport::MODE_SERVER == aOpenerMode);
  ProcessHandle thisHandle = GetCurrentProcessHandle();
  ProcessHandle parentHandle = isParent ? thisHandle : aOtherProcess;
  ProcessHandle childHandle = !isParent ? thisHandle : aOtherProcess;
  ProcessId parentId = GetProcId(parentHandle);
  ProcessId childId = GetProcId(childHandle);
  if (!parentId || !childId) {
    return false;
  }

  TransportDescriptor parentSide, childSide;
  if (!CreateTransport(parentHandle, childHandle,
                       &parentSide, &childSide)) {
    return false;
  }

  Message* parentMsg = new ChannelOpened(parentSide, childId, aProtocol);
  Message* childMsg = new ChannelOpened(childSide, parentId, aChildProtocol);
  nsAutoPtr<Message> messageForUs(isParent ? parentMsg : childMsg);
  nsAutoPtr<Message> messageForOtherSide(!isParent ? parentMsg : childMsg);
  if (!aOpenerChannel->Echo(messageForUs.forget()) ||
      !aOpenerChannel->Send(messageForOtherSide.forget())) {
    CloseDescriptor(parentSide);
    CloseDescriptor(childSide);
    return false;
  }
  return true;
}

bool
UnpackChannelOpened(const PrivateIPDLInterface&,
                    const Message& aMsg,
                    TransportDescriptor* aTransport,
                    ProcessId* aOtherProcess,
                    ProtocolId* aProtocol)
{
  return ChannelOpened::Read(aMsg, aTransport, aOtherProcess, aProtocol);
}

void
ProtocolErrorBreakpoint(const char* aMsg)
{
    
    
    
    printf_stderr("IPDL protocol error: %s\n", aMsg);
}

void
FatalError(const char* aProtocolName, const char* aMsg,
           ProcessHandle aHandle, bool aIsParent)
{
  ProtocolErrorBreakpoint(aMsg);

  nsAutoCString formattedMessage("IPDL error [");
  formattedMessage.AppendASCII(aProtocolName);
  formattedMessage.AppendLiteral("]: \"");
  formattedMessage.AppendASCII(aMsg);
  if (aIsParent) {
    formattedMessage.AppendLiteral("\". Killing child side as a result.");
    NS_ERROR(formattedMessage.get());

    if (!base::KillProcess(aHandle, base::PROCESS_END_KILLED_BY_USER, false)) {
      NS_ERROR("May have failed to kill child!");
    }
  } else {
    formattedMessage.AppendLiteral("\". abort()ing as a result.");
    NS_RUNTIMEABORT(formattedMessage.get());
  }
}

} 
} 
