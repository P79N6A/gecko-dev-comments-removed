







































#include "base/process_util.h"

#include "mozilla/ipc/AsyncChannel.h"
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/ipc/Transport.h"

using namespace base;

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
    IPC::WriteParam(this, static_cast<uint32>(aProtocol));
  }

  static bool Read(const IPC::Message& aMsg,
                   TransportDescriptor* aDescriptor,
                   ProcessId* aOtherProcess,
                   ProtocolId* aProtocol)
  {
    void* iter = nsnull;
    if (!IPC::ReadParam(&aMsg, &iter, aDescriptor) ||
        !IPC::ReadParam(&aMsg, &iter, aOtherProcess) ||
        !IPC::ReadParam(&aMsg, &iter, reinterpret_cast<uint32*>(aProtocol))) {
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
       ProtocolId aProtocol)
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
                                             aProtocol))) {
    CloseDescriptor(parentSide);
    CloseDescriptor(childSide);
    return false;
  }
  return true;
}

bool
UnpackChannelOpened(const PrivateIPDLInterface&,
                    const IPC::Message& aMsg,
                    TransportDescriptor* aTransport,
                    ProcessId* aOtherProcess,
                    ProtocolId* aProtocol)
{
  return ChannelOpened::Read(aMsg, aTransport, aOtherProcess, aProtocol);
}

} 
} 
