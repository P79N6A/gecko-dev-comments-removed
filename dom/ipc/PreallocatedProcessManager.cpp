





#include "mozilla/PreallocatedProcessManager.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ScriptSettings.h"
#include "nsIPropertyBag2.h"
#include "ProcessPriorityManager.h"
#include "nsServiceManagerUtils.h"

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif




#define DEFAULT_ALLOCATE_DELAY 1000
#define NUWA_FORK_WAIT_DURATION_MS 2000 // 2 seconds.

using namespace mozilla;
using namespace mozilla::hal;
using namespace mozilla::dom;

namespace {





class PreallocatedProcessManagerImpl MOZ_FINAL
  : public nsIObserver
{
public:
  static PreallocatedProcessManagerImpl* Singleton();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  void AllocateAfterDelay();
  void AllocateOnIdle();
  void AllocateNow();
  already_AddRefed<ContentParent> Take();

#ifdef MOZ_NUWA_PROCESS
public:
  void ScheduleDelayedNuwaFork();
  void DelayedNuwaFork();
  void PublishSpareProcess(ContentParent* aContent);
  void MaybeForgetSpare(ContentParent* aContent);
  bool IsNuwaReady();
  void OnNuwaReady();
  bool PreallocatedProcessReady();
  already_AddRefed<ContentParent> GetSpareProcess();

private:
  void NuwaFork();

  
  CancelableTask* mPreallocateAppProcessTask;

  
  
  nsAutoTArray<nsRefPtr<ContentParent>, 4> mSpareProcesses;

  
  bool mIsNuwaReady;
#endif

private:
  static mozilla::StaticRefPtr<PreallocatedProcessManagerImpl> sSingleton;

  PreallocatedProcessManagerImpl();
  ~PreallocatedProcessManagerImpl() {}
  DISALLOW_EVIL_CONSTRUCTORS(PreallocatedProcessManagerImpl);

  void Init();

  void RereadPrefs();
  void Enable();
  void Disable();

  void ObserveProcessShutdown(nsISupports* aSubject);

  bool mEnabled;
  bool mShutdown;
  nsRefPtr<ContentParent> mPreallocatedAppProcess;
};

 StaticRefPtr<PreallocatedProcessManagerImpl>
PreallocatedProcessManagerImpl::sSingleton;

 PreallocatedProcessManagerImpl*
PreallocatedProcessManagerImpl::Singleton()
{
  if (!sSingleton) {
    sSingleton = new PreallocatedProcessManagerImpl();
    sSingleton->Init();
    ClearOnShutdown(&sSingleton);
  }

  return sSingleton;
}

NS_IMPL_ISUPPORTS(PreallocatedProcessManagerImpl, nsIObserver)

PreallocatedProcessManagerImpl::PreallocatedProcessManagerImpl()
  : mEnabled(false)
#ifdef MOZ_NUWA_PROCESS
  , mPreallocateAppProcessTask(nullptr)
  , mIsNuwaReady(false)
#endif
  , mShutdown(false)
{}

void
PreallocatedProcessManagerImpl::Init()
{
  Preferences::AddStrongObserver(this, "dom.ipc.processPrelaunch.enabled");
  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    os->AddObserver(this, "ipc:content-shutdown",
                     false);
    os->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                     false);
  }
  RereadPrefs();
}

NS_IMETHODIMP
PreallocatedProcessManagerImpl::Observe(nsISupports* aSubject,
                                        const char* aTopic,
                                        const char16_t* aData)
{
  if (!strcmp("ipc:content-shutdown", aTopic)) {
    ObserveProcessShutdown(aSubject);
  } else if (!strcmp("nsPref:changed", aTopic)) {
    
    RereadPrefs();
  } else if (!strcmp(NS_XPCOM_SHUTDOWN_OBSERVER_ID, aTopic)) {
    mShutdown = true;
  } else {
    MOZ_ASSERT(false);
  }

  return NS_OK;
}

void
PreallocatedProcessManagerImpl::RereadPrefs()
{
  if (Preferences::GetBool("dom.ipc.processPrelaunch.enabled")) {
    Enable();
  } else {
    Disable();
  }
}

already_AddRefed<ContentParent>
PreallocatedProcessManagerImpl::Take()
{
  return mPreallocatedAppProcess.forget();
}

void
PreallocatedProcessManagerImpl::Enable()
{
  if (mEnabled) {
    return;
  }

  mEnabled = true;
#ifdef MOZ_NUWA_PROCESS
  ScheduleDelayedNuwaFork();
#else
  AllocateAfterDelay();
#endif
}

void
PreallocatedProcessManagerImpl::AllocateAfterDelay()
{
  if (!mEnabled || mPreallocatedAppProcess) {
    return;
  }

  MessageLoop::current()->PostDelayedTask(
    FROM_HERE,
    NewRunnableMethod(this, &PreallocatedProcessManagerImpl::AllocateOnIdle),
    Preferences::GetUint("dom.ipc.processPrelaunch.delayMs",
                         DEFAULT_ALLOCATE_DELAY));
}

void
PreallocatedProcessManagerImpl::AllocateOnIdle()
{
  if (!mEnabled || mPreallocatedAppProcess) {
    return;
  }

  MessageLoop::current()->PostIdleTask(
    FROM_HERE,
    NewRunnableMethod(this, &PreallocatedProcessManagerImpl::AllocateNow));
}

void
PreallocatedProcessManagerImpl::AllocateNow()
{
  if (!mEnabled || mPreallocatedAppProcess) {
    return;
  }

  mPreallocatedAppProcess = ContentParent::PreallocateAppProcess();
}

#ifdef MOZ_NUWA_PROCESS

void
PreallocatedProcessManagerImpl::ScheduleDelayedNuwaFork()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mPreallocateAppProcessTask) {
    
    return;
  }

  mPreallocateAppProcessTask = NewRunnableMethod(
    this, &PreallocatedProcessManagerImpl::DelayedNuwaFork);
  MessageLoop::current()->PostDelayedTask(
    FROM_HERE, mPreallocateAppProcessTask,
    Preferences::GetUint("dom.ipc.processPrelaunch.delayMs",
                         DEFAULT_ALLOCATE_DELAY));
}

void
PreallocatedProcessManagerImpl::DelayedNuwaFork()
{
  MOZ_ASSERT(NS_IsMainThread());

  mPreallocateAppProcessTask = nullptr;

  if (!mIsNuwaReady) {
    if (!mPreallocatedAppProcess && !mShutdown && mEnabled) {
      mPreallocatedAppProcess = ContentParent::RunNuwaProcess();
    }
    
  } else if (mSpareProcesses.IsEmpty()) {
    NuwaFork();
  }
}




already_AddRefed<ContentParent>
PreallocatedProcessManagerImpl::GetSpareProcess()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mSpareProcesses.IsEmpty()) {
    return nullptr;
  }

  nsRefPtr<ContentParent> process = mSpareProcesses.LastElement();
  mSpareProcesses.RemoveElementAt(mSpareProcesses.Length() - 1);

  if (mSpareProcesses.IsEmpty() && mIsNuwaReady) {
    NS_ASSERTION(mPreallocatedAppProcess != nullptr,
                 "Nuwa process is not present!");
    ScheduleDelayedNuwaFork();
  }

  return process.forget();
}




void
PreallocatedProcessManagerImpl::PublishSpareProcess(ContentParent* aContent)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (Preferences::GetBool("dom.ipc.processPriorityManager.testMode")) {
    AutoJSContext cx;
    nsCOMPtr<nsIMessageBroadcaster> ppmm =
      do_GetService("@mozilla.org/parentprocessmessagemanager;1");
    mozilla::unused << ppmm->BroadcastAsyncMessage(
      NS_LITERAL_STRING("TEST-ONLY:nuwa-add-new-process"),
      JS::NullHandleValue, JS::NullHandleValue, cx, 1);
  }

  mSpareProcesses.AppendElement(aContent);
}

void
PreallocatedProcessManagerImpl::MaybeForgetSpare(ContentParent* aContent)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mSpareProcesses.RemoveElement(aContent)) {
    return;
  }

  if (aContent == mPreallocatedAppProcess) {
    mPreallocatedAppProcess = nullptr;
    mIsNuwaReady = false;
    while (mSpareProcesses.Length() > 0) {
      nsRefPtr<ContentParent> process = mSpareProcesses[mSpareProcesses.Length() - 1];
      process->Close();
      mSpareProcesses.RemoveElementAt(mSpareProcesses.Length() - 1);
    }
    ScheduleDelayedNuwaFork();
  }
}

bool
PreallocatedProcessManagerImpl::IsNuwaReady()
{
  return mIsNuwaReady;
}

void
PreallocatedProcessManagerImpl::OnNuwaReady()
{
  NS_ASSERTION(!mIsNuwaReady, "Multiple Nuwa processes created!");
  ProcessPriorityManager::SetProcessPriority(mPreallocatedAppProcess,
                                             hal::PROCESS_PRIORITY_MASTER);
  mIsNuwaReady = true;
  if (Preferences::GetBool("dom.ipc.processPriorityManager.testMode")) {
    AutoJSContext cx;
    nsCOMPtr<nsIMessageBroadcaster> ppmm =
      do_GetService("@mozilla.org/parentprocessmessagemanager;1");
    mozilla::unused << ppmm->BroadcastAsyncMessage(
      NS_LITERAL_STRING("TEST-ONLY:nuwa-ready"),
      JS::NullHandleValue, JS::NullHandleValue, cx, 1);
  }
  NuwaFork();
}

bool
PreallocatedProcessManagerImpl::PreallocatedProcessReady()
{
  return !mSpareProcesses.IsEmpty();
}


void
PreallocatedProcessManagerImpl::NuwaFork()
{
  mozilla::unused << mPreallocatedAppProcess->SendNuwaFork();
}
#endif

void
PreallocatedProcessManagerImpl::Disable()
{
  if (!mEnabled) {
    return;
  }

  mEnabled = false;

#ifdef MOZ_NUWA_PROCESS
  
  if (mPreallocateAppProcessTask) {
    mPreallocateAppProcessTask->Cancel();
    mPreallocateAppProcessTask = nullptr;
  }
#endif

  if (mPreallocatedAppProcess) {
#ifdef MOZ_NUWA_PROCESS
    while (mSpareProcesses.Length() > 0){
      nsRefPtr<ContentParent> process = mSpareProcesses[0];
      process->Close();
      mSpareProcesses.RemoveElementAt(0);
    }
    mIsNuwaReady = false;
#endif
    mPreallocatedAppProcess->Close();
    mPreallocatedAppProcess = nullptr;
  }
}

void
PreallocatedProcessManagerImpl::ObserveProcessShutdown(nsISupports* aSubject)
{
  if (!mPreallocatedAppProcess) {
    return;
  }

  nsCOMPtr<nsIPropertyBag2> props = do_QueryInterface(aSubject);
  NS_ENSURE_TRUE_VOID(props);

  uint64_t childID = CONTENT_PROCESS_ID_UNKNOWN;
  props->GetPropertyAsUint64(NS_LITERAL_STRING("childID"), &childID);
  NS_ENSURE_TRUE_VOID(childID != CONTENT_PROCESS_ID_UNKNOWN);

  if (childID == mPreallocatedAppProcess->ChildID()) {
    mPreallocatedAppProcess = nullptr;
  }
}

inline PreallocatedProcessManagerImpl* GetPPMImpl()
{
  return PreallocatedProcessManagerImpl::Singleton();
}

} 

namespace mozilla {

 void
PreallocatedProcessManager::AllocateAfterDelay()
{
#ifdef MOZ_NUWA_PROCESS
  GetPPMImpl()->ScheduleDelayedNuwaFork();
#else
  GetPPMImpl()->AllocateAfterDelay();
#endif
}

 void
PreallocatedProcessManager::AllocateOnIdle()
{
  GetPPMImpl()->AllocateOnIdle();
}

 void
PreallocatedProcessManager::AllocateNow()
{
  GetPPMImpl()->AllocateNow();
}

 already_AddRefed<ContentParent>
PreallocatedProcessManager::Take()
{
#ifdef MOZ_NUWA_PROCESS
  return GetPPMImpl()->GetSpareProcess();
#else
  return GetPPMImpl()->Take();
#endif
}

#ifdef MOZ_NUWA_PROCESS
 void
PreallocatedProcessManager::PublishSpareProcess(ContentParent* aContent)
{
  GetPPMImpl()->PublishSpareProcess(aContent);
}

 void
PreallocatedProcessManager::MaybeForgetSpare(ContentParent* aContent)
{
  GetPPMImpl()->MaybeForgetSpare(aContent);
}

 void
PreallocatedProcessManager::OnNuwaReady()
{
  GetPPMImpl()->OnNuwaReady();
}

 bool
PreallocatedProcessManager::IsNuwaReady()
{
  return GetPPMImpl()->IsNuwaReady();
}

 bool
PreallocatedProcessManager::PreallocatedProcessReady()
{
  return GetPPMImpl()->PreallocatedProcessReady();
}

#endif

} 
