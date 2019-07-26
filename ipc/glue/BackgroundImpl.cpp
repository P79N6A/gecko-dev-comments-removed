



#include "base/process_util.h"
#include "mozilla/Assertions.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/ipc/ProtocolTypes.h"
#include "BackgroundChild.h"
#include "BackgroundChildImpl.h"
#include "BackgroundParent.h"
#include "BackgroundParentImpl.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIEventTarget.h"
#include "nsIIPCBackgroundChildCreateCallback.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIRunnable.h"
#include "nsISupportsImpl.h"
#include "nsIThread.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "nsTraceRefcnt.h"
#include "nsXULAppAPI.h"
#include "nsXPCOMPrivate.h"
#include "prthread.h"

#define CRASH_IN_CHILD_PROCESS(_msg)                                           \
  do {                                                                         \
    if (IsMainProcess()) {                                                     \
      MOZ_ASSERT(false, _msg);                                                 \
    } else {                                                                   \
      MOZ_CRASH(_msg);                                                         \
    }                                                                          \
  }                                                                            \
  while (0)

using namespace mozilla;
using namespace mozilla::ipc;

using mozilla::dom::ContentChild;
using mozilla::dom::ContentParent;

namespace {





bool
IsMainProcess()
{
  static const bool isMainProcess =
    XRE_GetProcessType() == GeckoProcessType_Default;
  return isMainProcess;
}

bool
IsChildProcess()
{
  return !IsMainProcess();
}

void
AssertIsInMainProcess()
{
  MOZ_ASSERT(IsMainProcess());
}

void
AssertIsInChildProcess()
{
  MOZ_ASSERT(IsChildProcess());
}

void
AssertIsOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
}





class ParentImpl MOZ_FINAL : public BackgroundParentImpl
{
  friend class mozilla::ipc::BackgroundParent;

public:
  class CreateCallback;

private:
  class ShutdownObserver;
  class RequestMessageLoopRunnable;
  class CreateCallbackRunnable;
  class ConnectActorRunnable;

  
  
  static StaticRefPtr<nsIThread> sBackgroundThread;

  
  
  static PRThread* sBackgroundPRThread;

  
  
  static MessageLoop* sBackgroundThreadMessageLoop;

  
  
  
  static uint64_t sLiveActorCount;

  
  
  static bool sShutdownObserverRegistered;

  
  
  static bool sShutdownHasStarted;

  
  
  static StaticAutoPtr<nsTArray<nsRefPtr<CreateCallback>>> sPendingCallbacks;

  
  nsRefPtr<ContentParent> mContent;

  
  
  
  Transport* mTransport;

  
  bool mActorAlive;

  
  DebugOnly<bool> mIsOtherProcessActorDEBUG;

public:
  static bool
  CreateActorForSameProcess(CreateCallback* aCallback);

  static bool
  IsOnBackgroundThread()
  {
    return PR_GetCurrentThread() == sBackgroundPRThread;
  }

  static void
  AssertIsOnBackgroundThread()
  {
    MOZ_ASSERT(IsOnBackgroundThread());
  }

  NS_INLINE_DECL_REFCOUNTING(ParentImpl)

  bool
  ActorIsAlive() const
  {
    AssertIsOnBackgroundThread();
    return mActorAlive;
  }

  void
  Destroy();

private:
  
  static PBackgroundParent*
  Alloc(ContentParent* aContent,
        Transport* aTransport,
        ProcessId aOtherProcess);

  static bool
  CreateBackgroundThread();

  static void
  ShutdownBackgroundThread();

  
  ParentImpl()
  : mTransport(nullptr), mActorAlive(true), mIsOtherProcessActorDEBUG(false)
  {
    AssertIsInMainProcess();
    AssertIsOnMainThread();
  }

  
  ParentImpl(ContentParent* aContent, Transport* aTransport)
  : mContent(aContent), mTransport(aTransport), mActorAlive(false),
    mIsOtherProcessActorDEBUG(true)
  {
    AssertIsInMainProcess();
    AssertIsOnMainThread();
    MOZ_ASSERT(aContent);
    MOZ_ASSERT(aTransport);
  }

  ~ParentImpl()
  {
    AssertIsInMainProcess();
    AssertIsOnMainThread();
    MOZ_ASSERT(!mContent);
    MOZ_ASSERT(!mTransport);
  }

  void
  MainThreadActorDestroy();

  
  virtual IToplevelProtocol*
  CloneToplevel(const InfallibleTArray<ProtocolFdMapping>& aFds,
                ProcessHandle aPeerProcess,
                ProtocolCloneContext* aCtx) MOZ_OVERRIDE;

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
};





class ChildImpl MOZ_FINAL : public BackgroundChildImpl
{
  friend class mozilla::ipc::BackgroundChild;

  typedef base::ProcessId ProcessId;
  typedef mozilla::ipc::Transport Transport;

  class ShutdownObserver;
  class CreateActorRunnable;
  class ParentCreateCallback;
  class CreateCallbackRunnable;
  class OpenChildProcessActorRunnable;
  class OpenMainProcessActorRunnable;

  
  static const unsigned int kBadThreadLocalIndex =
    static_cast<unsigned int>(-1);

  
  
  static unsigned int sThreadLocalIndex;

  struct ThreadLocalInfo
  {
    ThreadLocalInfo(nsIIPCBackgroundChildCreateCallback* aCallback)
    {
      mCallbacks.AppendElement(aCallback);
    }

    nsRefPtr<ChildImpl> mActor;
    nsTArray<nsCOMPtr<nsIIPCBackgroundChildCreateCallback>> mCallbacks;
  };

  
  
  static StaticAutoPtr<nsTArray<nsCOMPtr<nsIEventTarget>>> sPendingTargets;

  
  
  static bool sShutdownHasStarted;

  DebugOnly<nsIThread*> mBoundThread;

public:
  static bool
  OpenProtocolOnMainThread(nsIEventTarget* aEventTarget);

  static void
  Shutdown();

  void
  AssertIsOnBoundThread()
  {
    MOZ_ASSERT(mBoundThread);

    DebugOnly<bool> current;
    MOZ_ASSERT(NS_SUCCEEDED(mBoundThread->IsOnCurrentThread(&current)));

    MOZ_ASSERT(current);
  }


  ChildImpl()
  : mBoundThread(nullptr)
  {
    AssertIsOnMainThread();
  }

  NS_INLINE_DECL_REFCOUNTING(ChildImpl)

private:
  
  static void
  Startup();

  
  static PBackgroundChild*
  Alloc(Transport* aTransport, ProcessId aOtherProcess);

  
  static PBackgroundChild*
  GetForCurrentThread();

  
  static bool
  GetOrCreateForCurrentThread(nsIIPCBackgroundChildCreateCallback* aCallback);

  static void
  ThreadLocalDestructor(void* aThreadLocal)
  {
    auto threadLocalInfo = static_cast<ThreadLocalInfo*>(aThreadLocal);

    if (threadLocalInfo) {
      if (threadLocalInfo->mActor) {
        threadLocalInfo->mActor->Close();
      }
      delete threadLocalInfo;
    }
  }

  static void
  DispatchFailureCallback(nsIEventTarget* aEventTarget);

  
  ~ChildImpl()
  { }

  void
  SetBoundThread()
  {
#ifdef DEBUG
    MOZ_ASSERT(!mBoundThread);
    mBoundThread = NS_GetCurrentThread();
    MOZ_ASSERT(mBoundThread);
#endif
  }

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
};





class ParentImpl::ShutdownObserver MOZ_FINAL : public nsIObserver
{
public:
  ShutdownObserver()
  {
    AssertIsOnMainThread();
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

private:
  ~ShutdownObserver()
  {
    AssertIsOnMainThread();
  }
};

class ParentImpl::RequestMessageLoopRunnable MOZ_FINAL :
  public nsRunnable
{
  nsCOMPtr<nsIThread> mTargetThread;
  MessageLoop* mMessageLoop;

public:
  RequestMessageLoopRunnable(nsIThread* aTargetThread)
  : mTargetThread(aTargetThread), mMessageLoop(nullptr)
  {
    AssertIsInMainProcess();
    AssertIsOnMainThread();
    MOZ_ASSERT(aTargetThread);
  }

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~RequestMessageLoopRunnable()
  { }

  NS_DECL_NSIRUNNABLE
};

class ParentImpl::CreateCallbackRunnable MOZ_FINAL : public nsRunnable
{
  nsRefPtr<CreateCallback> mCallback;

public:
  CreateCallbackRunnable(CreateCallback* aCallback)
  : mCallback(aCallback)
  {
    AssertIsInMainProcess();
    AssertIsOnMainThread();
    MOZ_ASSERT(aCallback);
  }

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~CreateCallbackRunnable()
  { }

  NS_DECL_NSIRUNNABLE
};

class ParentImpl::ConnectActorRunnable MOZ_FINAL : public nsRunnable
{
  nsRefPtr<ParentImpl> mActor;
  Transport* mTransport;
  ProcessHandle mProcessHandle;

public:
  ConnectActorRunnable(ParentImpl* aActor,
                       Transport* aTransport,
                       ProcessHandle aProcessHandle)
  : mActor(aActor), mTransport(aTransport), mProcessHandle(aProcessHandle)
  {
    AssertIsInMainProcess();
    AssertIsOnMainThread();
    MOZ_ASSERT(aActor);
    MOZ_ASSERT(aTransport);
  }

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~ConnectActorRunnable()
  {
    AssertIsInMainProcess();
  }

  NS_DECL_NSIRUNNABLE
};

class NS_NO_VTABLE ParentImpl::CreateCallback
{
public:
  NS_INLINE_DECL_REFCOUNTING(CreateCallback)

  virtual void
  Success(already_AddRefed<ParentImpl> aActor, MessageLoop* aMessageLoop) = 0;

  virtual void
  Failure() = 0;

protected:
  virtual ~CreateCallback()
  { }
};





class ChildImpl::ShutdownObserver MOZ_FINAL : public nsIObserver
{
public:
  ShutdownObserver()
  {
    AssertIsOnMainThread();
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

private:
  ~ShutdownObserver()
  {
    AssertIsOnMainThread();
  }
};

class ChildImpl::CreateActorRunnable MOZ_FINAL : public nsRunnable
{
  nsCOMPtr<nsIEventTarget> mEventTarget;

public:
  CreateActorRunnable()
  : mEventTarget(NS_GetCurrentThread())
  {
    MOZ_ASSERT(mEventTarget);
  }

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~CreateActorRunnable()
  { }

  NS_DECL_NSIRUNNABLE
};

class ChildImpl::ParentCreateCallback MOZ_FINAL :
  public ParentImpl::CreateCallback
{
  nsCOMPtr<nsIEventTarget> mEventTarget;

public:
  ParentCreateCallback(nsIEventTarget* aEventTarget)
  : mEventTarget(aEventTarget)
  {
    AssertIsInMainProcess();
    AssertIsOnMainThread();
    MOZ_ASSERT(aEventTarget);
  }

private:
  ~ParentCreateCallback()
  { }

  virtual void
  Success(already_AddRefed<ParentImpl> aActor, MessageLoop* aMessageLoop)
          MOZ_OVERRIDE;

  virtual void
  Failure() MOZ_OVERRIDE;
};

class ChildImpl::CreateCallbackRunnable : public nsRunnable
{
protected:
  nsRefPtr<ChildImpl> mActor;

public:
  CreateCallbackRunnable(already_AddRefed<ChildImpl> aActor)
  : mActor(aActor)
  {
    

    MOZ_ASSERT(aActor.get());
  }

  CreateCallbackRunnable()
  {
    
  }

  NS_DECL_ISUPPORTS_INHERITED

protected:
  virtual ~CreateCallbackRunnable();

  static already_AddRefed<nsIIPCBackgroundChildCreateCallback>
  GetNextCallback();

  NS_DECL_NSIRUNNABLE
};

class ChildImpl::OpenChildProcessActorRunnable MOZ_FINAL :
  public ChildImpl::CreateCallbackRunnable
{
  nsAutoPtr<Transport> mTransport;
  ProcessHandle mProcessHandle;

public:
  OpenChildProcessActorRunnable(already_AddRefed<ChildImpl> aActor,
                                Transport* aTransport,
                                ProcessHandle aProcessHandle)
  : CreateCallbackRunnable(aActor), mTransport(aTransport),
    mProcessHandle(aProcessHandle)
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(aTransport);
  }

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~OpenChildProcessActorRunnable()
  {
    if (mTransport) {
      CRASH_IN_CHILD_PROCESS("Leaking transport!");
      mTransport.forget();
    }
  }

  NS_DECL_NSIRUNNABLE
};

class ChildImpl::OpenMainProcessActorRunnable MOZ_FINAL :
  public ChildImpl::CreateCallbackRunnable
{
  nsRefPtr<ParentImpl> mParentActor;
  MessageLoop* mParentMessageLoop;

public:
  OpenMainProcessActorRunnable(already_AddRefed<ChildImpl> aChildActor,
                               already_AddRefed<ParentImpl> aParentActor,
                               MessageLoop* aParentMessageLoop)
  : CreateCallbackRunnable(aChildActor), mParentActor(aParentActor),
    mParentMessageLoop(aParentMessageLoop)
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(aParentActor.get());
    MOZ_ASSERT(aParentMessageLoop);
  }

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~OpenMainProcessActorRunnable()
  { }

  NS_DECL_NSIRUNNABLE
};

} 

namespace mozilla {
namespace ipc {

bool
IsOnBackgroundThread()
{
  return ParentImpl::IsOnBackgroundThread();
}

#ifdef DEBUG

void
AssertIsOnBackgroundThread()
{
  ParentImpl::AssertIsOnBackgroundThread();
}

#endif 

} 
} 






PBackgroundParent*
BackgroundParent::Alloc(ContentParent* aContent,
                        Transport* aTransport,
                        ProcessId aOtherProcess)
{
  return ParentImpl::Alloc(aContent, aTransport, aOtherProcess);
}






void
BackgroundChild::Startup()
{
  ChildImpl::Startup();
}


PBackgroundChild*
BackgroundChild::Alloc(Transport* aTransport, ProcessId aOtherProcess)
{
  return ChildImpl::Alloc(aTransport, aOtherProcess);
}


PBackgroundChild*
BackgroundChild::GetForCurrentThread()
{
  return ChildImpl::GetForCurrentThread();
}


bool
BackgroundChild::GetOrCreateForCurrentThread(
                                 nsIIPCBackgroundChildCreateCallback* aCallback)
{
  return ChildImpl::GetOrCreateForCurrentThread(aCallback);
}





StaticRefPtr<nsIThread> ParentImpl::sBackgroundThread;

PRThread* ParentImpl::sBackgroundPRThread = nullptr;

MessageLoop* ParentImpl::sBackgroundThreadMessageLoop = nullptr;

uint64_t ParentImpl::sLiveActorCount = 0;

bool ParentImpl::sShutdownObserverRegistered = false;

bool ParentImpl::sShutdownHasStarted = false;

StaticAutoPtr<nsTArray<nsRefPtr<ParentImpl::CreateCallback>>>
  ParentImpl::sPendingCallbacks;





unsigned int ChildImpl::sThreadLocalIndex = kBadThreadLocalIndex;

StaticAutoPtr<nsTArray<nsCOMPtr<nsIEventTarget>>> ChildImpl::sPendingTargets;

bool ChildImpl::sShutdownHasStarted = false;






PBackgroundParent*
ParentImpl::Alloc(ContentParent* aContent,
                  Transport* aTransport,
                  ProcessId aOtherProcess)
{
  AssertIsInMainProcess();
  AssertIsOnMainThread();
  MOZ_ASSERT(aTransport);

  ProcessHandle processHandle;
  if (!base::OpenProcessHandle(aOtherProcess, &processHandle)) {
    
    return nullptr;
  }

  if (!sBackgroundThread && !CreateBackgroundThread()) {
    NS_WARNING("Failed to create background thread!");
    return nullptr;
  }

  sLiveActorCount++;

  nsRefPtr<ParentImpl> actor = new ParentImpl(aContent, aTransport);

  nsCOMPtr<nsIRunnable> connectRunnable =
    new ConnectActorRunnable(actor, aTransport, processHandle);

  if (NS_FAILED(sBackgroundThread->Dispatch(connectRunnable,
                                            NS_DISPATCH_NORMAL))) {
    NS_WARNING("Failed to dispatch connect runnable!");

    MOZ_ASSERT(sLiveActorCount);
    sLiveActorCount--;

    if (!sLiveActorCount) {
      ShutdownBackgroundThread();
    }

    return nullptr;
  }

  return actor;
}


bool
ParentImpl::CreateActorForSameProcess(CreateCallback* aCallback)
{
  AssertIsInMainProcess();
  AssertIsOnMainThread();
  MOZ_ASSERT(aCallback);

  if (!sBackgroundThread && !CreateBackgroundThread()) {
    NS_WARNING("Failed to create background thread!");
    return false;
  }

  MOZ_ASSERT(!sShutdownHasStarted);

  sLiveActorCount++;

  if (sBackgroundThreadMessageLoop) {
    nsCOMPtr<nsIRunnable> callbackRunnable =
      new CreateCallbackRunnable(aCallback);
    if (NS_FAILED(NS_DispatchToCurrentThread(callbackRunnable))) {
      NS_WARNING("Failed to dispatch callback runnable!");
      return false;
    }

    return true;
  }

  if (!sPendingCallbacks) {
    sPendingCallbacks = new nsTArray<nsRefPtr<CreateCallback>>();
  }

  sPendingCallbacks->AppendElement(aCallback);
  return true;
}


bool
ParentImpl::CreateBackgroundThread()
{
  AssertIsInMainProcess();
  AssertIsOnMainThread();
  MOZ_ASSERT(!sBackgroundThread);

  if (sShutdownHasStarted) {
    NS_WARNING("Trying to create background thread after shutdown has "
               "already begun!");
    return false;
  }

  if (!sShutdownObserverRegistered) {
    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    if (NS_WARN_IF(!obs)) {
      return false;
    }

    nsCOMPtr<nsIObserver> observer = new ShutdownObserver();

    nsresult rv =
      obs->AddObserver(observer, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID, false);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return false;
    }

    sShutdownObserverRegistered = true;
  }

  nsCOMPtr<nsIThread> thread;
  if (NS_FAILED(NS_NewNamedThread("IPDL Background", getter_AddRefs(thread)))) {
    NS_WARNING("NS_NewNamedThread failed!");
    return false;
  }

  nsCOMPtr<nsIRunnable> messageLoopRunnable =
    new RequestMessageLoopRunnable(thread);
  if (NS_FAILED(thread->Dispatch(messageLoopRunnable, NS_DISPATCH_NORMAL))) {
    NS_WARNING("Failed to dispatch RequestMessageLoopRunnable!");
    return false;
  }

  sBackgroundThread = thread;
  return true;
}


void
ParentImpl::ShutdownBackgroundThread()
{
  AssertIsInMainProcess();
  AssertIsOnMainThread();
  MOZ_ASSERT_IF(!sBackgroundThread, !sBackgroundThreadMessageLoop);
  MOZ_ASSERT_IF(!sShutdownHasStarted, !sLiveActorCount);

  if (sPendingCallbacks) {
    if (!sPendingCallbacks->IsEmpty()) {
      nsTArray<nsRefPtr<CreateCallback>> callbacks;
      sPendingCallbacks->SwapElements(callbacks);

      for (uint32_t index = 0; index < callbacks.Length(); index++) {
        nsRefPtr<CreateCallback> callback;
        callbacks[index].swap(callback);
        MOZ_ASSERT(callback);

        callback->Failure();
      }
    }

    if (sShutdownHasStarted) {
      sPendingCallbacks = nullptr;
    }
  }

  if (sBackgroundThread) {
    nsCOMPtr<nsIThread> thread = sBackgroundThread;

    sBackgroundThread = nullptr;
    sBackgroundThreadMessageLoop = nullptr;

    if (NS_FAILED(thread->Shutdown())) {
      NS_WARNING("Failed to shut down background thread!");
    }

    
    
    if (sShutdownHasStarted && sLiveActorCount) {
      nsIThread* currentThread = NS_GetCurrentThread();
      MOZ_ASSERT(currentThread);

      while (sLiveActorCount) {
        NS_ProcessNextEvent(currentThread);
      }
    }
  }
}

void
ParentImpl::Destroy()
{
  

  AssertIsInMainProcess();

  nsCOMPtr<nsIRunnable> destroyRunnable =
    NS_NewNonOwningRunnableMethod(this, &ParentImpl::MainThreadActorDestroy);

  if (NS_FAILED(NS_DispatchToMainThread(destroyRunnable,
                                        NS_DISPATCH_NORMAL))) {
    NS_WARNING("Failed to dispatch destroyed runnable!");
  }
}

void
ParentImpl::MainThreadActorDestroy()
{
  AssertIsInMainProcess();
  AssertIsOnMainThread();
  MOZ_ASSERT_IF(mIsOtherProcessActorDEBUG, mContent);
  MOZ_ASSERT_IF(!mIsOtherProcessActorDEBUG, !mContent);
  MOZ_ASSERT_IF(mIsOtherProcessActorDEBUG, mTransport);
  MOZ_ASSERT_IF(!mIsOtherProcessActorDEBUG, !mTransport);

  if (mTransport) {
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     new DeleteTask<Transport>(mTransport));
    mTransport = nullptr;
  }

  mContent = nullptr;

  MOZ_ASSERT(sLiveActorCount);
  sLiveActorCount--;

  if (!sLiveActorCount) {
    ShutdownBackgroundThread();
  }

  
  Release();
}

IToplevelProtocol*
ParentImpl::CloneToplevel(const InfallibleTArray<ProtocolFdMapping>& aFds,
                          ProcessHandle aPeerProcess,
                          ProtocolCloneContext* aCtx)
{
  AssertIsInMainProcess();
  AssertIsOnMainThread();

  const ProtocolId protocolId = GetProtocolId();

  for (unsigned int i = 0; i < aFds.Length(); i++) {
    if (static_cast<ProtocolId>(aFds[i].protocolId()) != protocolId) {
      continue;
    }

    Transport* transport = OpenDescriptor(aFds[i].fd(),
                                          Transport::MODE_SERVER);
    if (!transport) {
      NS_WARNING("Failed to open transport!");
      break;
    }

    PBackgroundParent* clonedActor =
      Alloc(mContent, transport, base::GetProcId(aPeerProcess));
    MOZ_ASSERT(clonedActor);

    clonedActor->CloneManagees(this, aCtx);
    clonedActor->SetTransport(transport);

    return clonedActor;
  }

  return nullptr;
}

void
ParentImpl::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsInMainProcess();
  AssertIsOnBackgroundThread();

  BackgroundParentImpl::ActorDestroy(aWhy);

  Destroy();
}

NS_IMPL_ISUPPORTS1(ParentImpl::ShutdownObserver, nsIObserver)

NS_IMETHODIMP
ParentImpl::ShutdownObserver::Observe(nsISupports* aSubject,
                                      const char* aTopic,
                                      const char16_t* aData)
{
  AssertIsInMainProcess();
  AssertIsOnMainThread();
  MOZ_ASSERT(!sShutdownHasStarted);
  MOZ_ASSERT(!strcmp(aTopic, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID));

  sShutdownHasStarted = true;

  
  
  ChildImpl::Shutdown();

  ShutdownBackgroundThread();

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED0(ParentImpl::RequestMessageLoopRunnable,
                             nsRunnable)

NS_IMETHODIMP
ParentImpl::RequestMessageLoopRunnable::Run()
{
  AssertIsInMainProcess();
  MOZ_ASSERT(mTargetThread);

  if (NS_IsMainThread()) {
    MOZ_ASSERT(mMessageLoop);

    if (!sBackgroundThread ||
        !SameCOMIdentity(mTargetThread.get(), sBackgroundThread.get())) {
      return NS_OK;
    }

    MOZ_ASSERT(!sBackgroundThreadMessageLoop);
    sBackgroundThreadMessageLoop = mMessageLoop;

    if (sPendingCallbacks && !sPendingCallbacks->IsEmpty()) {
      nsTArray<nsRefPtr<CreateCallback>> callbacks;
      sPendingCallbacks->SwapElements(callbacks);

      for (uint32_t index = 0; index < callbacks.Length(); index++) {
        MOZ_ASSERT(callbacks[index]);

        nsCOMPtr<nsIRunnable> callbackRunnable =
          new CreateCallbackRunnable(callbacks[index]);
        if (NS_FAILED(NS_DispatchToCurrentThread(callbackRunnable))) {
          NS_WARNING("Failed to dispatch callback runnable!");
        }
      }
    }

    return NS_OK;
  }

#ifdef DEBUG
  {
    PRThread* currentPRThread = PR_GetCurrentThread();
    MOZ_ASSERT(currentPRThread);
    MOZ_ASSERT_IF(sBackgroundPRThread, currentPRThread != sBackgroundPRThread);

    bool correctThread;
    MOZ_ASSERT(NS_SUCCEEDED(mTargetThread->IsOnCurrentThread(&correctThread)));
    MOZ_ASSERT(correctThread);
  }
#endif

  sBackgroundPRThread = PR_GetCurrentThread();

  MOZ_ASSERT(!mMessageLoop);

  mMessageLoop = MessageLoop::current();
  MOZ_ASSERT(mMessageLoop);

  if (NS_FAILED(NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL))) {
    NS_WARNING("Failed to dispatch RequestMessageLoopRunnable to main thread!");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED0(ParentImpl::CreateCallbackRunnable, nsRunnable)

NS_IMETHODIMP
ParentImpl::CreateCallbackRunnable::Run()
{
  AssertIsInMainProcess();
  AssertIsOnMainThread();
  MOZ_ASSERT(sBackgroundThreadMessageLoop);
  MOZ_ASSERT(mCallback);

  nsRefPtr<CreateCallback> callback;
  mCallback.swap(callback);

  nsRefPtr<ParentImpl> actor = new ParentImpl();

  callback->Success(actor.forget(), sBackgroundThreadMessageLoop);

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED0(ParentImpl::ConnectActorRunnable, nsRunnable)

NS_IMETHODIMP
ParentImpl::ConnectActorRunnable::Run()
{
  AssertIsInMainProcess();
  AssertIsOnBackgroundThread();

  
  
  ParentImpl* actor;
  mActor.forget(&actor);

  if (!actor->Open(mTransport, mProcessHandle, XRE_GetIOMessageLoop(),
                   ParentSide)) {
    actor->Destroy();
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}






void
ChildImpl::Startup()
{
  
  

  MOZ_ASSERT(sThreadLocalIndex == kBadThreadLocalIndex,
             "BackgroundChild::Startup() called more than once!");

  PRStatus status =
    PR_NewThreadPrivateIndex(&sThreadLocalIndex, ThreadLocalDestructor);
  MOZ_RELEASE_ASSERT(status == PR_SUCCESS, "PR_NewThreadPrivateIndex failed!");

  MOZ_ASSERT(sThreadLocalIndex != kBadThreadLocalIndex);

  nsCOMPtr<nsIObserverService> observerService = services::GetObserverService();
  MOZ_RELEASE_ASSERT(observerService);

  nsCOMPtr<nsIObserver> observer = new ShutdownObserver();

  nsresult rv =
    observerService->AddObserver(observer,
                                 NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID,
                                 false);
  MOZ_RELEASE_ASSERT(NS_SUCCEEDED(rv));
}


void
ChildImpl::Shutdown()
{
  AssertIsOnMainThread();

  if (sShutdownHasStarted) {
    return;
  }

  sShutdownHasStarted = true;

  MOZ_ASSERT(sThreadLocalIndex != kBadThreadLocalIndex);

  DebugOnly<PRStatus> status = PR_SetThreadPrivate(sThreadLocalIndex, nullptr);
  MOZ_ASSERT(status == PR_SUCCESS);
}


PBackgroundChild*
ChildImpl::Alloc(Transport* aTransport, ProcessId aOtherProcess)
{
  AssertIsInChildProcess();
  AssertIsOnMainThread();
  MOZ_ASSERT(aTransport);
  MOZ_ASSERT(sPendingTargets);
  MOZ_ASSERT(!sPendingTargets->IsEmpty());

  nsCOMPtr<nsIEventTarget> eventTarget;
  sPendingTargets->ElementAt(0).swap(eventTarget);

  sPendingTargets->RemoveElementAt(0);

  ProcessHandle processHandle;
  if (!base::OpenProcessHandle(aOtherProcess, &processHandle)) {
    MOZ_CRASH("Failed to open process handle!");
  }

  nsRefPtr<ChildImpl> actor = new ChildImpl();

  ChildImpl* weakActor = actor;

  nsCOMPtr<nsIRunnable> openRunnable =
    new OpenChildProcessActorRunnable(actor.forget(), aTransport,
                                      processHandle);
  if (NS_FAILED(eventTarget->Dispatch(openRunnable, NS_DISPATCH_NORMAL))) {
    MOZ_CRASH("Failed to dispatch OpenActorRunnable!");
  }

  
  
  return weakActor;
}


PBackgroundChild*
ChildImpl::GetForCurrentThread()
{
  MOZ_ASSERT(sThreadLocalIndex != kBadThreadLocalIndex);

  auto threadLocalInfo =
    static_cast<ThreadLocalInfo*>(PR_GetThreadPrivate(sThreadLocalIndex));

  return threadLocalInfo ? threadLocalInfo->mActor : nullptr;
}


bool
ChildImpl::GetOrCreateForCurrentThread(
                                 nsIIPCBackgroundChildCreateCallback* aCallback)
{
  MOZ_ASSERT(aCallback);
  MOZ_ASSERT(sThreadLocalIndex != kBadThreadLocalIndex,
             "BackgroundChild::Startup() was never called!");

  bool created = false;

  auto threadLocalInfo =
    static_cast<ThreadLocalInfo*>(PR_GetThreadPrivate(sThreadLocalIndex));

  if (threadLocalInfo) {
    threadLocalInfo->mCallbacks.AppendElement(aCallback);
  } else {
    nsAutoPtr<ThreadLocalInfo> newInfo(new ThreadLocalInfo(aCallback));

    if (PR_SetThreadPrivate(sThreadLocalIndex, newInfo) != PR_SUCCESS) {
      CRASH_IN_CHILD_PROCESS("PR_SetThreadPrivate failed!");
      return false;
    }

    created = true;
    threadLocalInfo = newInfo.forget();
  }

  if (threadLocalInfo->mActor) {
    nsRefPtr<ChildImpl> actor = threadLocalInfo->mActor;

    nsCOMPtr<nsIRunnable> runnable = new CreateCallbackRunnable(actor.forget());
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToCurrentThread(runnable)));

    return true;
  }

  if (!created) {
    
    
    
    return true;
  }

  if (NS_IsMainThread()) {
    if (NS_WARN_IF(!OpenProtocolOnMainThread(NS_GetCurrentThread()))) {
      return false;
    }

    return true;
  }

  nsCOMPtr<CreateActorRunnable> runnable = new CreateActorRunnable();
  if (NS_FAILED(NS_DispatchToMainThread(runnable, NS_DISPATCH_NORMAL))) {
    CRASH_IN_CHILD_PROCESS("Failed to dispatch to main thread!");
    return false;
  }

  return true;
}

ChildImpl::CreateCallbackRunnable::~CreateCallbackRunnable()
{
  if (mActor) {
    CRASH_IN_CHILD_PROCESS("Leaking actor!");
    mActor.forget();
  }
}


already_AddRefed<nsIIPCBackgroundChildCreateCallback>
ChildImpl::CreateCallbackRunnable::GetNextCallback()
{
  

  auto threadLocalInfo =
    static_cast<ThreadLocalInfo*>(PR_GetThreadPrivate(sThreadLocalIndex));
  MOZ_ASSERT(threadLocalInfo);

  if (threadLocalInfo->mCallbacks.IsEmpty()) {
    return nullptr;
  }

  nsCOMPtr<nsIIPCBackgroundChildCreateCallback> callback;
  threadLocalInfo->mCallbacks[0].swap(callback);

  threadLocalInfo->mCallbacks.RemoveElementAt(0);

  return callback.forget();
}

NS_IMPL_ISUPPORTS_INHERITED0(ChildImpl::CreateCallbackRunnable, nsRunnable)

NS_IMETHODIMP
ChildImpl::CreateCallbackRunnable::Run()
{
  

  nsRefPtr<ChildImpl> actor;
  mActor.swap(actor);

  nsCOMPtr<nsIIPCBackgroundChildCreateCallback> callback = GetNextCallback();
  while (callback) {
    if (actor) {
      callback->ActorCreated(actor);
    } else {
      callback->ActorFailed();
    }

    callback = GetNextCallback();
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED0(ChildImpl::OpenChildProcessActorRunnable,
                             ChildImpl::CreateCallbackRunnable)

NS_IMETHODIMP
ChildImpl::OpenChildProcessActorRunnable::Run()
{
  

  AssertIsInChildProcess();
  MOZ_ASSERT(mActor);
  MOZ_ASSERT(mTransport);

  nsCOMPtr<nsIIPCBackgroundChildCreateCallback> callback = GetNextCallback();
  MOZ_ASSERT(callback,
             "There should be at least one callback when first creating the "
             "actor!");

  nsRefPtr<ChildImpl> strongActor;
  mActor.swap(strongActor);

  if (!strongActor->Open(mTransport.forget(), mProcessHandle,
                         XRE_GetIOMessageLoop(), ChildSide)) {
    CRASH_IN_CHILD_PROCESS("Failed to open ChildImpl!");

    while (callback) {
      callback->ActorFailed();
      callback = GetNextCallback();
    }

    return NS_OK;
  }

  
  auto threadLocalInfo =
    static_cast<ThreadLocalInfo*>(PR_GetThreadPrivate(sThreadLocalIndex));

  MOZ_ASSERT(threadLocalInfo);
  MOZ_ASSERT(!threadLocalInfo->mActor);

  nsRefPtr<ChildImpl>& actor = threadLocalInfo->mActor;
  strongActor.swap(actor);

  actor->SetBoundThread();

  while (callback) {
    callback->ActorCreated(actor);
    callback = GetNextCallback();
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED0(ChildImpl::OpenMainProcessActorRunnable,
                             ChildImpl::CreateCallbackRunnable)

NS_IMETHODIMP
ChildImpl::OpenMainProcessActorRunnable::Run()
{
  

  AssertIsInMainProcess();
  MOZ_ASSERT(mActor);
  MOZ_ASSERT(mParentActor);
  MOZ_ASSERT(mParentMessageLoop);

  nsCOMPtr<nsIIPCBackgroundChildCreateCallback> callback = GetNextCallback();
  MOZ_ASSERT(callback,
             "There should be at least one callback when first creating the "
             "actor!");

  nsRefPtr<ChildImpl> strongChildActor;
  mActor.swap(strongChildActor);

  nsRefPtr<ParentImpl> parentActor;
  mParentActor.swap(parentActor);

  MessageChannel* parentChannel = parentActor->GetIPCChannel();
  MOZ_ASSERT(parentChannel);

  if (!strongChildActor->Open(parentChannel, mParentMessageLoop, ChildSide)) {
    NS_WARNING("Failed to open ChildImpl!");

    parentActor->Destroy();

    while (callback) {
      callback->ActorFailed();
      callback = GetNextCallback();
    }

    return NS_OK;
  }

  
  parentActor.forget();

  auto threadLocalInfo =
    static_cast<ThreadLocalInfo*>(PR_GetThreadPrivate(sThreadLocalIndex));

  MOZ_ASSERT(threadLocalInfo);
  MOZ_ASSERT(!threadLocalInfo->mActor);

  nsRefPtr<ChildImpl>& childActor = threadLocalInfo->mActor;
  strongChildActor.swap(childActor);

  childActor->SetBoundThread();

  while (callback) {
    callback->ActorCreated(childActor);
    callback = GetNextCallback();
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED0(ChildImpl::CreateActorRunnable, nsRunnable)

NS_IMETHODIMP
ChildImpl::CreateActorRunnable::Run()
{
  AssertIsOnMainThread();

  if (!OpenProtocolOnMainThread(mEventTarget)) {
    NS_WARNING("OpenProtocolOnMainThread failed!");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

void
ChildImpl::ParentCreateCallback::Success(
                                      already_AddRefed<ParentImpl> aParentActor,
                                      MessageLoop* aParentMessageLoop)
{
  AssertIsInMainProcess();
  AssertIsOnMainThread();
  MOZ_ASSERT(aParentActor.get());
  MOZ_ASSERT(aParentMessageLoop);
  MOZ_ASSERT(mEventTarget);

  nsRefPtr<ChildImpl> childActor = new ChildImpl();

  nsCOMPtr<nsIEventTarget> target;
  mEventTarget.swap(target);

  nsCOMPtr<nsIRunnable> openRunnable =
    new OpenMainProcessActorRunnable(childActor.forget(), aParentActor,
                                     aParentMessageLoop);
  if (NS_FAILED(target->Dispatch(openRunnable, NS_DISPATCH_NORMAL))) {
    NS_WARNING("Failed to dispatch open runnable!");
  }
}

void
ChildImpl::ParentCreateCallback::Failure()
{
  AssertIsInMainProcess();
  AssertIsOnMainThread();
  MOZ_ASSERT(mEventTarget);

  nsCOMPtr<nsIEventTarget> target;
  mEventTarget.swap(target);

  DispatchFailureCallback(target);
}


bool
ChildImpl::OpenProtocolOnMainThread(nsIEventTarget* aEventTarget)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aEventTarget);

  if (sShutdownHasStarted) {
    MOZ_CRASH("Called BackgroundChild::GetOrCreateForCurrentThread after "
              "shutdown has started!");
  }

  if (IsMainProcess()) {
    nsRefPtr<ParentImpl::CreateCallback> parentCallback =
      new ParentCreateCallback(aEventTarget);

    if (!ParentImpl::CreateActorForSameProcess(parentCallback)) {
      NS_WARNING("BackgroundParent::CreateActor() failed!");
      DispatchFailureCallback(aEventTarget);
      return false;
    }

    return true;
  }

  ContentChild* content = ContentChild::GetSingleton();
  MOZ_ASSERT(content);

  if (!PBackground::Open(content)) {
    MOZ_CRASH("Failed to create top level actor!");
    return false;
  }

  if (!sPendingTargets) {
    sPendingTargets = new nsTArray<nsCOMPtr<nsIEventTarget>>(1);
    ClearOnShutdown(&sPendingTargets);
  }

  sPendingTargets->AppendElement(aEventTarget);

  return true;
}


void
ChildImpl::DispatchFailureCallback(nsIEventTarget* aEventTarget)
{
  MOZ_ASSERT(aEventTarget);

  nsCOMPtr<nsIRunnable> callbackRunnable = new CreateCallbackRunnable();
  if (NS_FAILED(aEventTarget->Dispatch(callbackRunnable, NS_DISPATCH_NORMAL))) {
    NS_WARNING("Failed to dispatch CreateCallbackRunnable!");
  }
}

void
ChildImpl::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnBoundThread();

  BackgroundChildImpl::ActorDestroy(aWhy);
}

NS_IMPL_ISUPPORTS1(ChildImpl::ShutdownObserver, nsIObserver)

NS_IMETHODIMP
ChildImpl::ShutdownObserver::Observe(nsISupports* aSubject,
                                     const char* aTopic,
                                     const char16_t* aData)
{
  AssertIsInMainProcess();
  AssertIsOnMainThread();
  MOZ_ASSERT(!strcmp(aTopic, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID));

  ChildImpl::Shutdown();

  return NS_OK;
}
