





#ifndef mozilla_dom_workers_runtimeservice_h__
#define mozilla_dom_workers_runtimeservice_h__

#include "Workers.h"

#include "nsIObserver.h"

#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsAutoPtr.h"
#include "nsClassHashtable.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsHashKeys.h"
#include "nsString.h"
#include "nsTArray.h"

class nsIThread;
class nsITimer;
class nsPIDOMWindow;

BEGIN_WORKERS_NAMESPACE

class SharedWorker;
class WorkerPrivate;

class RuntimeService MOZ_FINAL : public nsIObserver
{
  struct SharedWorkerInfo
  {
    WorkerPrivate* mWorkerPrivate;
    nsCString mScriptSpec;
    nsString mName;

    SharedWorkerInfo(WorkerPrivate* aWorkerPrivate,
                     const nsACString& aScriptSpec,
                     const nsAString& aName)
    : mWorkerPrivate(aWorkerPrivate), mScriptSpec(aScriptSpec), mName(aName)
    { }
  };

  struct WorkerDomainInfo
  {
    nsCString mDomain;
    nsTArray<WorkerPrivate*> mActiveWorkers;
    nsTArray<WorkerPrivate*> mQueuedWorkers;
    nsClassHashtable<nsCStringHashKey, SharedWorkerInfo> mSharedWorkerInfos;
    uint32_t mChildWorkerCount;

    WorkerDomainInfo()
    : mActiveWorkers(1), mChildWorkerCount(0)
    { }

    uint32_t
    ActiveWorkerCount() const
    {
      return mActiveWorkers.Length() + mChildWorkerCount;
    }
  };

  struct IdleThreadInfo
  {
    nsCOMPtr<nsIThread> mThread;
    mozilla::TimeStamp mExpirationTime;
  };

  struct MatchSharedWorkerInfo
  {
    WorkerPrivate* mWorkerPrivate;
    SharedWorkerInfo* mSharedWorkerInfo;

    MatchSharedWorkerInfo(WorkerPrivate* aWorkerPrivate)
    : mWorkerPrivate(aWorkerPrivate), mSharedWorkerInfo(nullptr)
    { }
  };

  mozilla::Mutex mMutex;

  
  nsClassHashtable<nsCStringHashKey, WorkerDomainInfo> mDomainMap;

  
  nsTArray<IdleThreadInfo> mIdleThreadArray;

  
  nsClassHashtable<nsPtrHashKey<nsPIDOMWindow>,
                   nsTArray<WorkerPrivate*> > mWindowMap;

  
  nsCOMPtr<nsITimer> mIdleThreadTimer;

  nsCString mDetectorName;
  nsCString mSystemCharset;

  static JSSettings sDefaultJSSettings;

public:
  struct NavigatorStrings
  {
    nsString mAppName;
    nsString mAppVersion;
    nsString mPlatform;
    nsString mUserAgent;
  };

private:
  NavigatorStrings mNavigatorStrings;

  
  bool mObserved;
  bool mShuttingDown;
  bool mNavigatorStringsLoaded;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static RuntimeService*
  GetOrCreateService();

  static RuntimeService*
  GetService();

  bool
  RegisterWorker(JSContext* aCx, WorkerPrivate* aWorkerPrivate);

  void
  UnregisterWorker(JSContext* aCx, WorkerPrivate* aWorkerPrivate);

  void
  CancelWorkersForWindow(nsPIDOMWindow* aWindow);

  void
  SuspendWorkersForWindow(nsPIDOMWindow* aWindow);

  void
  ResumeWorkersForWindow(nsPIDOMWindow* aWindow);

  nsresult
  CreateSharedWorker(const GlobalObject& aGlobal,
                     const nsAString& aScriptURL,
                     const nsAString& aName,
                     SharedWorker** aSharedWorker);

  void
  ForgetSharedWorker(WorkerPrivate* aWorkerPrivate);

  const nsACString&
  GetDetectorName() const
  {
    return mDetectorName;
  }

  const nsACString&
  GetSystemCharset() const
  {
    return mSystemCharset;
  }

  const NavigatorStrings&
  GetNavigatorStrings() const
  {
    return mNavigatorStrings;
  }

  void
  NoteIdleThread(nsIThread* aThread);

  static void
  GetDefaultJSSettings(JSSettings& aSettings)
  {
    AssertIsOnMainThread();
    aSettings = sDefaultJSSettings;
  }

  static void
  SetDefaultJSContextOptions(uint32_t aContentOptions, uint32_t aChromeOptions)
  {
    AssertIsOnMainThread();
    sDefaultJSSettings.content.options = aContentOptions;
    sDefaultJSSettings.chrome.options = aChromeOptions;
  }

  void
  UpdateAllWorkerJSContextOptions();

  static void
  SetDefaultJSGCSettings(JSGCParamKey aKey, uint32_t aValue)
  {
    AssertIsOnMainThread();
    sDefaultJSSettings.ApplyGCSetting(aKey, aValue);
  }

  void
  UpdateAllWorkerMemoryParameter(JSGCParamKey aKey, uint32_t aValue);

  static uint32_t
  GetContentCloseHandlerTimeoutSeconds()
  {
    return sDefaultJSSettings.content.maxScriptRuntime;
  }

  static uint32_t
  GetChromeCloseHandlerTimeoutSeconds()
  {
    return sDefaultJSSettings.chrome.maxScriptRuntime;
  }

#ifdef JS_GC_ZEAL
  static void
  SetDefaultGCZeal(uint8_t aGCZeal, uint32_t aFrequency)
  {
    AssertIsOnMainThread();
    sDefaultJSSettings.gcZeal = aGCZeal;
    sDefaultJSSettings.gcZealFrequency = aFrequency;
  }

  void
  UpdateAllWorkerGCZeal();
#endif

  static void
  SetDefaultJITHardening(bool aJITHardening)
  {
    AssertIsOnMainThread();
    sDefaultJSSettings.jitHardening = aJITHardening;
  }

  void
  UpdateAllWorkerJITHardening(bool aJITHardening);

  void
  GarbageCollectAllWorkers(bool aShrinking);

private:
  RuntimeService();
  ~RuntimeService();

  nsresult
  Init();

  void
  Shutdown();

  void
  Cleanup();

  static PLDHashOperator
  AddAllTopLevelWorkersToArray(const nsACString& aKey,
                               WorkerDomainInfo* aData,
                               void* aUserArg);

  static PLDHashOperator
  RemoveSharedWorkerFromWindowMap(nsPIDOMWindow* aKey,
                                  nsAutoPtr<nsTArray<WorkerPrivate*> >& aData,
                                  void* aUserArg);

  static PLDHashOperator
  FindSharedWorkerInfo(const nsACString& aKey,
                       SharedWorkerInfo* aData,
                       void* aUserArg);

  void
  GetWorkersForWindow(nsPIDOMWindow* aWindow,
                      nsTArray<WorkerPrivate*>& aWorkers);

  bool
  ScheduleWorker(JSContext* aCx, WorkerPrivate* aWorkerPrivate);

  static void
  ShutdownIdleThreads(nsITimer* aTimer, void* aClosure);
};

END_WORKERS_NAMESPACE

#endif 
