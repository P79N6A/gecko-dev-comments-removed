






#include "base/process_util.h"

#include "mozilla/ipc/MessageChannel.h"
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/ipc/Transport.h"

#if defined(MOZ_SANDBOX) && defined(XP_WIN)
#define TARGET_SANDBOX_EXPORTS
#include "mozilla/sandboxTarget.h"
#endif

using namespace IPC;

using base::GetCurrentProcId;
using base::ProcessHandle;
using base::ProcessId;

namespace mozilla {
namespace ipc {

static Atomic<size_t> gNumProtocols;
static StaticAutoPtr<Mutex> gProtocolMutex;

IToplevelProtocol::IToplevelProtocol(ProtocolId aProtoId)
 : mOpener(nullptr)
 , mProtocolId(aProtoId)
 , mTrans(nullptr)
{
  size_t old = gNumProtocols++;

  if (!old) {
    
    
    MOZ_ASSERT(NS_IsMainThread());
    gProtocolMutex = new Mutex("ITopLevelProtocol::ProtocolMutex");
  }
}

IToplevelProtocol::~IToplevelProtocol()
{
  bool last = false;

  {
    MutexAutoLock al(*gProtocolMutex);

    for (IToplevelProtocol* actor = mOpenActors.getFirst();
         actor;
         actor = actor->getNext()) {
      actor->mOpener = nullptr;
    }

    mOpenActors.clear();

    if (mOpener) {
      removeFrom(mOpener->mOpenActors);
    }

    gNumProtocols--;
    last = gNumProtocols == 0;
  }

  if (last) {
    gProtocolMutex = nullptr;
  }
}

void
IToplevelProtocol::AddOpenedActorLocked(IToplevelProtocol* aActor)
{
  gProtocolMutex->AssertCurrentThreadOwns();

#ifdef DEBUG
  for (const IToplevelProtocol* actor = mOpenActors.getFirst();
       actor;
       actor = actor->getNext()) {
    NS_ASSERTION(actor != aActor,
                 "Open the same protocol for more than one time");
  }
#endif

  aActor->mOpener = this;
  mOpenActors.insertBack(aActor);
}

void
IToplevelProtocol::AddOpenedActor(IToplevelProtocol* aActor)
{
  MutexAutoLock al(*gProtocolMutex);
  AddOpenedActorLocked(aActor);
}

void
IToplevelProtocol::GetOpenedActorsLocked(nsTArray<IToplevelProtocol*>& aActors)
{
  gProtocolMutex->AssertCurrentThreadOwns();

  for (IToplevelProtocol* actor = mOpenActors.getFirst();
       actor;
       actor = actor->getNext()) {
    aActors.AppendElement(actor);
  }
}

void
IToplevelProtocol::GetOpenedActors(nsTArray<IToplevelProtocol*>& aActors)
{
  MutexAutoLock al(*gProtocolMutex);
  GetOpenedActorsLocked(aActors);
}

size_t
IToplevelProtocol::GetOpenedActorsUnsafe(IToplevelProtocol** aActors, size_t aActorsMax)
{
  size_t count = 0;
  for (IToplevelProtocol* actor = mOpenActors.getFirst();
       actor;
       actor = actor->getNext()) {
    MOZ_RELEASE_ASSERT(count < aActorsMax);
    aActors[count++] = actor;
  }
  return count;
}

IToplevelProtocol*
IToplevelProtocol::CloneToplevel(const InfallibleTArray<ProtocolFdMapping>& aFds,
                                 base::ProcessHandle aPeerProcess,
                                 ProtocolCloneContext* aCtx)
{
  NS_NOTREACHED("Clone() for this protocol actor is not implemented");
  return nullptr;
}

void
IToplevelProtocol::CloneOpenedToplevels(IToplevelProtocol* aTemplate,
                                        const InfallibleTArray<ProtocolFdMapping>& aFds,
                                        base::ProcessHandle aPeerProcess,
                                        ProtocolCloneContext* aCtx)
{
  MutexAutoLock al(*gProtocolMutex);

  nsTArray<IToplevelProtocol*> actors;
  aTemplate->GetOpenedActorsLocked(actors);

  for (size_t i = 0; i < actors.Length(); i++) {
    IToplevelProtocol* newactor = actors[i]->CloneToplevel(aFds, aPeerProcess, aCtx);
    AddOpenedActorLocked(newactor);
  }
}

class ChannelOpened : public IPC::Message
{
public:
  ChannelOpened(TransportDescriptor aDescriptor,
                ProcessId aOtherProcess,
                ProtocolId aProtocol,
                PriorityValue aPriority = PRIORITY_NORMAL)
    : IPC::Message(MSG_ROUTING_CONTROL, 
                   CHANNEL_OPENED_MESSAGE_TYPE,
                   aPriority)
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
       MessageChannel* aParentChannel, ProcessId aParentPid,
       MessageChannel* aChildChannel, ProcessId aChildPid,
       ProtocolId aProtocol, ProtocolId aChildProtocol)
{
  if (!aParentPid || !aChildPid) {
    return false;
  }

  TransportDescriptor parentSide, childSide;
  if (!CreateTransport(aParentPid, &parentSide, &childSide)) {
    return false;
  }

  if (!aParentChannel->Send(new ChannelOpened(parentSide,
                                              aChildPid,
                                              aProtocol,
                                              IPC::Message::PRIORITY_URGENT)) ||
      !aChildChannel->Send(new ChannelOpened(childSide,
                                             aParentPid,
                                             aChildProtocol,
                                             IPC::Message::PRIORITY_URGENT))) {
    CloseDescriptor(parentSide);
    CloseDescriptor(childSide);
    return false;
  }
  return true;
}

bool
Open(const PrivateIPDLInterface&,
     MessageChannel* aOpenerChannel, ProcessId aOtherProcessId,
     Transport::Mode aOpenerMode,
     ProtocolId aProtocol, ProtocolId aChildProtocol)
{
  bool isParent = (Transport::MODE_SERVER == aOpenerMode);
  ProcessId thisPid = GetCurrentProcId();
  ProcessId parentId = isParent ? thisPid : aOtherProcessId;
  ProcessId childId = !isParent ? thisPid : aOtherProcessId;
  if (!parentId || !childId) {
    return false;
  }

  TransportDescriptor parentSide, childSide;
  if (!CreateTransport(parentId, &parentSide, &childSide)) {
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

#if defined(XP_WIN)
bool DuplicateHandle(HANDLE aSourceHandle,
                     DWORD aTargetProcessId,
                     HANDLE* aTargetHandle,
                     DWORD aDesiredAccess,
                     DWORD aOptions) {
  
  if (aTargetProcessId == base::GetCurrentProcId()) {
    return !!::DuplicateHandle(::GetCurrentProcess(), aSourceHandle,
                               ::GetCurrentProcess(), aTargetHandle,
                               aDesiredAccess, false, aOptions);

  }

#if defined(MOZ_SANDBOX)
  
  if (SandboxTarget::Instance()->BrokerDuplicateHandle(aSourceHandle,
                                                       aTargetProcessId,
                                                       aTargetHandle,
                                                       aDesiredAccess,
                                                       aOptions)) {
    return true;
  }
#endif

  
  ScopedProcessHandle targetProcess;
  if (!base::OpenProcessHandle(aTargetProcessId, &targetProcess.rwget())) {
    return false;
  }

  return !!::DuplicateHandle(::GetCurrentProcess(), aSourceHandle,
                              targetProcess, aTargetHandle,
                              aDesiredAccess, FALSE, aOptions);
}
#endif

void
ProtocolErrorBreakpoint(const char* aMsg)
{
    
    
    
    printf_stderr("IPDL protocol error: %s\n", aMsg);
}

void
FatalError(const char* aProtocolName, const char* aMsg,
           ProcessId aOtherPid, bool aIsParent)
{
  ProtocolErrorBreakpoint(aMsg);

  nsAutoCString formattedMessage("IPDL error [");
  formattedMessage.AppendASCII(aProtocolName);
  formattedMessage.AppendLiteral("]: \"");
  formattedMessage.AppendASCII(aMsg);
  if (aIsParent) {
    formattedMessage.AppendLiteral("\". Killing child side as a result.");
    NS_ERROR(formattedMessage.get());

    if (aOtherPid != kInvalidProcessId && aOtherPid != base::GetCurrentProcId()) {
      ScopedProcessHandle otherProcessHandle;
      if (base::OpenProcessHandle(aOtherPid, &otherProcessHandle.rwget())) {
        if (!base::KillProcess(otherProcessHandle,
                               base::PROCESS_END_KILLED_BY_USER, false)) {
          NS_ERROR("May have failed to kill child!");
        }
      } else {
        NS_ERROR("Failed to open child process when attempting kill.");
      }
    }
  } else {
    formattedMessage.AppendLiteral("\". abort()ing as a result.");
    NS_RUNTIMEABORT(formattedMessage.get());
  }
}

} 
} 
