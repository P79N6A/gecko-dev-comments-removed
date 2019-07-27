






#ifndef mozilla_ipc_ProtocolUtils_h
#define mozilla_ipc_ProtocolUtils_h 1

#include "base/process.h"
#include "base/process_util.h"
#include "chrome/common/ipc_message_utils.h"

#include "prenv.h"

#include "IPCMessageStart.h"
#include "mozilla/Attributes.h"
#include "mozilla/ipc/FileDescriptor.h"
#include "mozilla/ipc/Shmem.h"
#include "mozilla/ipc/Transport.h"
#include "mozilla/ipc/MessageLink.h"
#include "mozilla/LinkedList.h"
#include "mozilla/Mutex.h"
#include "MainThreadUtils.h"

#if defined(ANDROID) && defined(DEBUG)
#include <android/log.h>
#endif



namespace {






enum {
    CHANNEL_OPENED_MESSAGE_TYPE = kuint16max - 5,
    SHMEM_DESTROYED_MESSAGE_TYPE = kuint16max - 4,
    SHMEM_CREATED_MESSAGE_TYPE = kuint16max - 3,
    GOODBYE_MESSAGE_TYPE       = kuint16max - 2

    
};
}

namespace mozilla {
namespace dom {
class ContentParent;
}

namespace net {
class NeckoParent;
}

namespace ipc {

#ifdef XP_WIN
const base::ProcessHandle kInvalidProcessHandle = INVALID_HANDLE_VALUE;







const base::ProcessId kInvalidProcessId = kuint32max;
#else
const base::ProcessHandle kInvalidProcessHandle = -1;
const base::ProcessId kInvalidProcessId = -1;
#endif


struct ScopedProcessHandleTraits
{
  typedef base::ProcessHandle type;

  static type empty()
  {
    return kInvalidProcessHandle;
  }

  static void release(type aProcessHandle)
  {
    if (aProcessHandle && aProcessHandle != kInvalidProcessHandle) {
      base::CloseProcessHandle(aProcessHandle);
    }
  }
};
typedef mozilla::Scoped<ScopedProcessHandleTraits> ScopedProcessHandle;

class ProtocolFdMapping;
class ProtocolCloneContext;




struct ActorHandle
{
    int mId;
};






struct Trigger
{
    enum Action { Send, Recv };

    Trigger(Action action, int32_t msg) :
        mAction(action),
        mMsg(msg)
    {}

    Action mAction;
    int32_t mMsg;
};

class ProtocolCloneContext
{
  typedef mozilla::dom::ContentParent ContentParent;
  typedef mozilla::net::NeckoParent NeckoParent;

  ContentParent* mContentParent;
  NeckoParent* mNeckoParent;

public:
  ProtocolCloneContext()
    : mContentParent(nullptr)
    , mNeckoParent(nullptr)
  {}

  void SetContentParent(ContentParent* aContentParent)
  {
    mContentParent = aContentParent;
  }

  ContentParent* GetContentParent() { return mContentParent; }

  void SetNeckoParent(NeckoParent* aNeckoParent)
  {
    mNeckoParent = aNeckoParent;
  }

  NeckoParent* GetNeckoParent() { return mNeckoParent; }
};

template<class ListenerT>
class  IProtocolManager
{
public:
    enum ActorDestroyReason {
        FailedConstructor,
        Deletion,
        AncestorDeletion,
        NormalShutdown,
        AbnormalShutdown
    };

    typedef base::ProcessId ProcessId;

    virtual int32_t Register(ListenerT*) = 0;
    virtual int32_t RegisterID(ListenerT*, int32_t) = 0;
    virtual ListenerT* Lookup(int32_t) = 0;
    virtual void Unregister(int32_t) = 0;
    virtual void RemoveManagee(int32_t, ListenerT*) = 0;

    virtual Shmem::SharedMemory* CreateSharedMemory(
        size_t, SharedMemory::SharedMemoryType, bool, int32_t*) = 0;
    virtual bool AdoptSharedMemory(Shmem::SharedMemory*, int32_t*) = 0;
    virtual Shmem::SharedMemory* LookupSharedMemory(int32_t) = 0;
    virtual bool IsTrackingSharedMemory(Shmem::SharedMemory*) = 0;
    virtual bool DestroySharedMemory(Shmem&) = 0;

    
    virtual ProcessId OtherPid() const = 0;
    virtual MessageChannel* GetIPCChannel() = 0;

    
    virtual void CloneManagees(ListenerT* aSource,
                               ProtocolCloneContext* aCtx) = 0;
};

typedef IPCMessageStart ProtocolId;




class IProtocol : protected MessageListener
{
public:
    




    virtual IProtocol*
    CloneProtocol(MessageChannel* aChannel,
                  ProtocolCloneContext* aCtx) = 0;
};







class IToplevelProtocol : private LinkedListElement<IToplevelProtocol>
{
    friend class LinkedList<IToplevelProtocol>;
    friend class LinkedListElement<IToplevelProtocol>;

protected:
    explicit IToplevelProtocol(ProtocolId aProtoId);
    ~IToplevelProtocol();

    



    void AddOpenedActor(IToplevelProtocol* aActor);

public:
    void SetTransport(Transport* aTrans)
    {
        mTrans = aTrans;
    }

    Transport* GetTransport() const { return mTrans; }

    ProtocolId GetProtocolId() const { return mProtocolId; }

    void GetOpenedActors(nsTArray<IToplevelProtocol*>& aActors);

    
    
    
    
    size_t GetOpenedActorsUnsafe(IToplevelProtocol** aActors, size_t aActorsMax);

    virtual IToplevelProtocol*
    CloneToplevel(const InfallibleTArray<ProtocolFdMapping>& aFds,
                  base::ProcessHandle aPeerProcess,
                  ProtocolCloneContext* aCtx);

    void CloneOpenedToplevels(IToplevelProtocol* aTemplate,
                              const InfallibleTArray<ProtocolFdMapping>& aFds,
                              base::ProcessHandle aPeerProcess,
                              ProtocolCloneContext* aCtx);

private:
    void AddOpenedActorLocked(IToplevelProtocol* aActor);
    void GetOpenedActorsLocked(nsTArray<IToplevelProtocol*>& aActors);

    LinkedList<IToplevelProtocol> mOpenActors; 
    IToplevelProtocol* mOpener;

    ProtocolId mProtocolId;
    Transport* mTrans;
};


inline bool
LoggingEnabled()
{
#if defined(DEBUG)
    return !!PR_GetEnv("MOZ_IPC_MESSAGE_LOG");
#else
    return false;
#endif
}

inline bool
LoggingEnabledFor(const char *aTopLevelProtocol)
{
#if defined(DEBUG)
    const char *filter = PR_GetEnv("MOZ_IPC_MESSAGE_LOG");
    if (!filter) {
        return false;
    }
    return strcmp(filter, "1") == 0 || strcmp(filter, aTopLevelProtocol) == 0;
#else
    return false;
#endif
}

MOZ_NEVER_INLINE void
ProtocolErrorBreakpoint(const char* aMsg);

MOZ_NEVER_INLINE void
FatalError(const char* aProtocolName, const char* aMsg,
           base::ProcessId aOtherPid, bool aIsParent);

struct PrivateIPDLInterface {};

bool
Bridge(const PrivateIPDLInterface&,
       MessageChannel*, base::ProcessId, MessageChannel*, base::ProcessId,
       ProtocolId, ProtocolId);

bool
Open(const PrivateIPDLInterface&,
     MessageChannel*, base::ProcessId, Transport::Mode,
     ProtocolId, ProtocolId);

bool
UnpackChannelOpened(const PrivateIPDLInterface&,
                    const IPC::Message&,
                    TransportDescriptor*, base::ProcessId*, ProtocolId*);

#if defined(XP_WIN)




bool
DuplicateHandle(HANDLE aSourceHandle,
                DWORD aTargetProcessId,
                HANDLE* aTargetHandle,
                DWORD aDesiredAccess,
                DWORD aOptions);
#endif

} 
} 


namespace IPC {

template <>
struct ParamTraits<mozilla::ipc::ActorHandle>
{
    typedef mozilla::ipc::ActorHandle paramType;

    static void Write(Message* aMsg, const paramType& aParam)
    {
        IPC::WriteParam(aMsg, aParam.mId);
    }

    static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
    {
        int id;
        if (IPC::ReadParam(aMsg, aIter, &id)) {
            aResult->mId = id;
            return true;
        }
        return false;
    }

    static void Log(const paramType& aParam, std::wstring* aLog)
    {
        aLog->append(StringPrintf(L"(%d)", aParam.mId));
    }
};

} 


#endif  
