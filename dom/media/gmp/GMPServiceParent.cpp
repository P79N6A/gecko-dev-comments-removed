




#include "GMPService.h"
#include "prio.h"
#include "prlog.h"
#include "GMPParent.h"
#include "GMPVideoDecoderParent.h"
#include "nsIObserverService.h"
#include "GeckoChildProcessHost.h"
#include "mozilla/Preferences.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/SyncRunnable.h"
#include "nsXPCOMPrivate.h"
#include "mozilla/Services.h"
#include "nsNativeCharsetUtils.h"
#include "nsIConsoleService.h"
#include "mozilla/unused.h"
#include "GMPDecryptorParent.h"
#include "GMPAudioDecoderParent.h"
#include "nsComponentManagerUtils.h"
#include "mozilla/Preferences.h"
#include "runnable_utils.h"
#include "VideoUtils.h"
#if defined(XP_LINUX) && defined(MOZ_GMP_SANDBOX)
#include "mozilla/SandboxInfo.h"
#endif
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsHashKeys.h"
#include "nsIFile.h"
#include "nsISimpleEnumerator.h"

namespace mozilla {

#ifdef LOG
#undef LOG
#endif

#ifdef PR_LOGGING
#define LOGD(msg) PR_LOG(GetGMPLog(), PR_LOG_DEBUG, msg)
#define LOG(level, msg) PR_LOG(GetGMPLog(), (level), msg)
#else
#define LOGD(msg)
#define LOG(leve1, msg)
#endif

#ifdef __CLASS__
#undef __CLASS__
#endif
#define __CLASS__ "GMPService"

namespace gmp {

static const uint32_t NodeIdSaltLength = 32;

already_AddRefed<GeckoMediaPluginServiceParent>
GeckoMediaPluginServiceParent::GetSingleton()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  nsRefPtr<GeckoMediaPluginService> service(
    GeckoMediaPluginServiceParent::GetGeckoMediaPluginService());
#ifdef DEBUG
  if (service) {
    nsCOMPtr<mozIGeckoMediaPluginChromeService> chromeService;
    CallQueryInterface(service.get(), getter_AddRefs(chromeService));
    MOZ_ASSERT(chromeService);
  }
#endif
  return service.forget().downcast<GeckoMediaPluginServiceParent>();
}

NS_IMPL_ISUPPORTS_INHERITED(GeckoMediaPluginServiceParent,
                            GeckoMediaPluginService,
                            mozIGeckoMediaPluginChromeService)

static int32_t sMaxAsyncShutdownWaitMs = 0;
static bool sHaveSetTimeoutPrefCache = false;

GeckoMediaPluginServiceParent::GeckoMediaPluginServiceParent()
  : mShuttingDown(false)
  , mScannedPluginOnDisk(false)
  , mWaitingForPluginsAsyncShutdown(false)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!sHaveSetTimeoutPrefCache) {
    sHaveSetTimeoutPrefCache = true;
    Preferences::AddIntVarCache(&sMaxAsyncShutdownWaitMs,
                                "media.gmp.async-shutdown-timeout",
                                GMP_DEFAULT_ASYNC_SHUTDONW_TIMEOUT);
  }
}

GeckoMediaPluginServiceParent::~GeckoMediaPluginServiceParent()
{
  MOZ_ASSERT(mPlugins.IsEmpty());
  MOZ_ASSERT(mAsyncShutdownPlugins.IsEmpty());
}

int32_t
GeckoMediaPluginServiceParent::AsyncShutdownTimeoutMs()
{
  MOZ_ASSERT(sHaveSetTimeoutPrefCache);
  return sMaxAsyncShutdownWaitMs;
}

nsresult
GeckoMediaPluginServiceParent::Init()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIObserverService> obsService = mozilla::services::GetObserverService();
  MOZ_ASSERT(obsService);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(obsService->AddObserver(this, "profile-change-teardown", false)));
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(obsService->AddObserver(this, "last-pb-context-exited", false)));
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(obsService->AddObserver(this, "browser:purge-session-history", false)));

#ifdef DEBUG
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(obsService->AddObserver(this, "mediakeys-request", false)));
#endif

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    prefs->AddObserver("media.gmp.plugin.crash", this, false);
  }

  nsresult rv = InitStorage();
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  nsCOMPtr<nsIThread> thread;
  return GetThread(getter_AddRefs(thread));
}


nsresult
GeckoMediaPluginServiceParent::InitStorage()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    return NS_OK;
  }

  
  
#ifdef MOZ_WIDGET_GONK
  nsresult rv = NS_NewLocalFile(NS_LITERAL_STRING("/data/b2g/mozilla"), false, getter_AddRefs(mStorageBaseDir));
#else
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(mStorageBaseDir));
#endif

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = mStorageBaseDir->AppendNative(NS_LITERAL_CSTRING("gmp"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = mStorageBaseDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
  if (NS_WARN_IF(NS_FAILED(rv) && rv != NS_ERROR_FILE_ALREADY_EXISTS)) {
    return rv;
  }

  return GeckoMediaPluginService::Init();
}

NS_IMETHODIMP
GeckoMediaPluginServiceParent::Observe(nsISupports* aSubject,
                                       const char* aTopic,
                                       const char16_t* aSomeData)
{
  LOGD(("%s::%s topic='%s' data='%s'", __CLASS__, __FUNCTION__,
       aTopic, NS_ConvertUTF16toUTF8(aSomeData).get()));
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsCOMPtr<nsIPrefBranch> branch( do_QueryInterface(aSubject) );
    if (branch) {
      bool crashNow = false;
      if (NS_LITERAL_STRING("media.gmp.plugin.crash").Equals(aSomeData)) {
        branch->GetBoolPref("media.gmp.plugin.crash",  &crashNow);
      }
      if (crashNow) {
        nsCOMPtr<nsIThread> gmpThread;
        {
          MutexAutoLock lock(mMutex);
          gmpThread = mGMPThread;
        }
        if (gmpThread) {
          gmpThread->Dispatch(WrapRunnable(this,
                                           &GeckoMediaPluginServiceParent::CrashPlugins),
                              NS_DISPATCH_NORMAL);
        }
      }
    }
  } else if (!strcmp("profile-change-teardown", aTopic)) {

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    mWaitingForPluginsAsyncShutdown = true;

    nsCOMPtr<nsIThread> gmpThread;
    {
      MutexAutoLock lock(mMutex);
      MOZ_ASSERT(!mShuttingDown);
      mShuttingDown = true;
      gmpThread = mGMPThread;
    }

    if (gmpThread) {
      gmpThread->Dispatch(
        NS_NewRunnableMethod(this,
                             &GeckoMediaPluginServiceParent::UnloadPlugins),
        NS_DISPATCH_NORMAL);
    } else {
      MOZ_ASSERT(mPlugins.IsEmpty());
    }

    
    while (mWaitingForPluginsAsyncShutdown) {
      NS_ProcessNextEvent(NS_GetCurrentThread(), true);
    }

  } else if (!strcmp(NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID, aTopic)) {
    MOZ_ASSERT(mShuttingDown);
    ShutdownGMPThread();
  } else if (!strcmp("last-pb-context-exited", aTopic)) {
    
    
    
    
    
    mTempNodeIds.Clear();
  } else if (!strcmp("browser:purge-session-history", aTopic)) {
    
    if (!aSomeData || nsDependentString(aSomeData).IsEmpty()) {
      return GMPDispatch(NS_NewRunnableMethod(
          this, &GeckoMediaPluginServiceParent::ClearStorage));
    }

    
    nsresult rv;
    PRTime t = nsDependentString(aSomeData).ToInteger64(&rv, 10);
    if (NS_FAILED(rv)) {
      return rv;
    }
    return GMPDispatch(NS_NewRunnableMethodWithArg<PRTime>(
        this, &GeckoMediaPluginServiceParent::ClearRecentHistoryOnGMPThread,
        t));
  }

  return NS_OK;
}

bool
GeckoMediaPluginServiceParent::GetContentParentFrom(const nsACString& aNodeId,
                                                    const nsCString& aAPI,
                                                    const nsTArray<nsCString>& aTags,
                                                    UniquePtr<GetGMPContentParentCallback>&& aCallback)
{
  nsRefPtr<GMPParent> gmp = SelectPluginForAPI(aNodeId, aAPI, aTags);

#ifdef PR_LOGGING
  nsCString api = aTags[0];
  LOGD(("%s: %p returning %p for api %s", __FUNCTION__, (void *)this, (void *)gmp, api.get()));
#endif

  if (!gmp) {
    return false;
  }

  return gmp->GetGMPContentParent(Move(aCallback));
}

void
GeckoMediaPluginServiceParent::InitializePlugins()
{
  mGMPThread->Dispatch(
    NS_NewRunnableMethod(this, &GeckoMediaPluginServiceParent::LoadFromEnvironment),
    NS_DISPATCH_NORMAL);
}

void
GeckoMediaPluginServiceParent::AsyncShutdownNeeded(GMPParent* aParent)
{
  LOGD(("%s::%s %p", __CLASS__, __FUNCTION__, aParent));
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);

  MOZ_ASSERT(!mAsyncShutdownPlugins.Contains(aParent));
  mAsyncShutdownPlugins.AppendElement(aParent);
}

void
GeckoMediaPluginServiceParent::AsyncShutdownComplete(GMPParent* aParent)
{
  LOGD(("%s::%s %p", __CLASS__, __FUNCTION__, aParent));
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);

  mAsyncShutdownPlugins.RemoveElement(aParent);
  if (mAsyncShutdownPlugins.IsEmpty() && mShuttingDownOnGMPThread) {
    
    
    nsCOMPtr<nsIRunnable> task(NS_NewRunnableMethod(
      this, &GeckoMediaPluginServiceParent::SetAsyncShutdownComplete));
    NS_DispatchToMainThread(task);
  }
}

void
GeckoMediaPluginServiceParent::SetAsyncShutdownComplete()
{
  MOZ_ASSERT(NS_IsMainThread());
  mWaitingForPluginsAsyncShutdown = false;
}

void
GeckoMediaPluginServiceParent::UnloadPlugins()
{
  LOGD(("%s::%s async_shutdown=%d", __CLASS__, __FUNCTION__,
        mAsyncShutdownPlugins.Length()));
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);

  MOZ_ASSERT(!mShuttingDownOnGMPThread);
  mShuttingDownOnGMPThread = true;

  {
    MutexAutoLock lock(mMutex);
    
    
    for (size_t i = 0; i < mPlugins.Length(); i++) {
      mPlugins[i]->CloseActive(true);
    }
    mPlugins.Clear();
  }

  if (mAsyncShutdownPlugins.IsEmpty()) {
    nsCOMPtr<nsIRunnable> task(NS_NewRunnableMethod(
      this, &GeckoMediaPluginServiceParent::SetAsyncShutdownComplete));
    NS_DispatchToMainThread(task);
  }
}

void
GeckoMediaPluginServiceParent::CrashPlugins()
{
  LOGD(("%s::%s", __CLASS__, __FUNCTION__));
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);

  MutexAutoLock lock(mMutex);
  for (size_t i = 0; i < mPlugins.Length(); i++) {
    mPlugins[i]->Crash();
  }
}

void
GeckoMediaPluginServiceParent::LoadFromEnvironment()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);

  const char* env = PR_GetEnv("MOZ_GMP_PATH");
  if (!env || !*env) {
    return;
  }

  nsString allpaths;
  if (NS_WARN_IF(NS_FAILED(NS_CopyNativeToUnicode(nsDependentCString(env), allpaths)))) {
    return;
  }

  uint32_t pos = 0;
  while (pos < allpaths.Length()) {
    
    
    int32_t next = allpaths.FindChar(XPCOM_ENV_PATH_SEPARATOR[0], pos);
    if (next == -1) {
      AddOnGMPThread(nsDependentSubstring(allpaths, pos));
      break;
    } else {
      AddOnGMPThread(nsDependentSubstring(allpaths, pos, next - pos));
      pos = next + 1;
    }
  }

  mScannedPluginOnDisk = true;
}

NS_IMETHODIMP
GeckoMediaPluginServiceParent::PathRunnable::Run()
{
  if (mOperation == ADD) {
    mService->AddOnGMPThread(mPath);
  } else {
    mService->RemoveOnGMPThread(mPath,
                                mOperation == REMOVE_AND_DELETE_FROM_DISK,
                                mDefer);
  }
  return NS_OK;
}

NS_IMETHODIMP
GeckoMediaPluginServiceParent::AddPluginDirectory(const nsAString& aDirectory)
{
  MOZ_ASSERT(NS_IsMainThread());
  return GMPDispatch(new PathRunnable(this, aDirectory,
                                      PathRunnable::EOperation::ADD));
}

NS_IMETHODIMP
GeckoMediaPluginServiceParent::RemovePluginDirectory(const nsAString& aDirectory)
{
  MOZ_ASSERT(NS_IsMainThread());
  return GMPDispatch(new PathRunnable(this, aDirectory,
                                      PathRunnable::EOperation::REMOVE));
}

NS_IMETHODIMP
GeckoMediaPluginServiceParent::RemoveAndDeletePluginDirectory(
  const nsAString& aDirectory, const bool aDefer)
{
  MOZ_ASSERT(NS_IsMainThread());
  return GMPDispatch(
    new PathRunnable(this, aDirectory,
                     PathRunnable::EOperation::REMOVE_AND_DELETE_FROM_DISK,
                     aDefer));
}

class DummyRunnable : public nsRunnable {
public:
  NS_IMETHOD Run() { return NS_OK; }
};

NS_IMETHODIMP
GeckoMediaPluginServiceParent::GetPluginVersionForAPI(const nsACString& aAPI,
                                                      nsTArray<nsCString>* aTags,
                                                      bool* aHasPlugin,
                                                      nsACString& aOutVersion)
{
  NS_ENSURE_ARG(aTags && aTags->Length() > 0);
  NS_ENSURE_ARG(aHasPlugin);
  NS_ENSURE_ARG(aOutVersion.IsEmpty());

  nsresult rv = EnsurePluginsOnDiskScanned();
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to load GMPs from disk.");
    return rv;
  }

  {
    MutexAutoLock lock(mMutex);
    nsCString api(aAPI);
    size_t index = 0;

    
    double maxParsedVersion = -1.;

    *aHasPlugin = false;
    while (GMPParent* gmp = FindPluginForAPIFrom(index, api, *aTags, &index)) {
      *aHasPlugin = true;
      double parsedVersion = atof(gmp->GetVersion().get());
      if (maxParsedVersion < 0 || parsedVersion > maxParsedVersion) {
        maxParsedVersion = parsedVersion;
        aOutVersion = gmp->GetVersion();
      }
      index++;
    }
  }

  return NS_OK;
}

nsresult
GeckoMediaPluginServiceParent::EnsurePluginsOnDiskScanned()
{
  const char* env = nullptr;
  if (!mScannedPluginOnDisk && (env = PR_GetEnv("MOZ_GMP_PATH")) && *env) {
    
    
    
    
    
    
    nsresult rv = GMPDispatch(new DummyRunnable(), NS_DISPATCH_SYNC);
    NS_ENSURE_SUCCESS(rv, rv);
    MOZ_ASSERT(mScannedPluginOnDisk, "Should have scanned MOZ_GMP_PATH by now");
  }

  return NS_OK;
}

GMPParent*
GeckoMediaPluginServiceParent::FindPluginForAPIFrom(size_t aSearchStartIndex,
                                                    const nsCString& aAPI,
                                                    const nsTArray<nsCString>& aTags,
                                                    size_t* aOutPluginIndex)
{
  mMutex.AssertCurrentThreadOwns();
  for (size_t i = aSearchStartIndex; i < mPlugins.Length(); i++) {
    GMPParent* gmp = mPlugins[i];
    bool supportsAllTags = true;
    for (size_t t = 0; t < aTags.Length(); t++) {
      const nsCString& tag = aTags.ElementAt(t);
      if (!gmp->SupportsAPI(aAPI, tag)) {
        supportsAllTags = false;
        break;
      }
    }
    if (!supportsAllTags) {
      continue;
    }
    if (aOutPluginIndex) {
      *aOutPluginIndex = i;
    }
    return gmp;
  }
  return nullptr;
}

GMPParent*
GeckoMediaPluginServiceParent::SelectPluginForAPI(const nsACString& aNodeId,
                                                  const nsCString& aAPI,
                                                  const nsTArray<nsCString>& aTags)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread,
             "Can't clone GMP plugins on non-GMP threads.");

  GMPParent* gmpToClone = nullptr;
  {
    MutexAutoLock lock(mMutex);
    size_t index = 0;
    GMPParent* gmp = nullptr;
    while ((gmp = FindPluginForAPIFrom(index, aAPI, aTags, &index))) {
      if (aNodeId.IsEmpty()) {
        if (gmp->CanBeSharedCrossNodeIds()) {
          return gmp;
        }
      } else if (gmp->CanBeUsedFrom(aNodeId)) {
        MOZ_ASSERT(!aNodeId.IsEmpty());
        gmp->SetNodeId(aNodeId);
        return gmp;
      }

      if (!gmpToClone ||
          (gmpToClone->IsMarkedForDeletion() && !gmp->IsMarkedForDeletion())) {
        
        
        
        
        
        
        
        gmpToClone = gmp;
      }
      
      index++;
    }
  }

  
  
  if (gmpToClone) {
    GMPParent* clone = ClonePlugin(gmpToClone);
    if (!aNodeId.IsEmpty()) {
      clone->SetNodeId(aNodeId);
    }
    return clone;
  }

  if (aAPI.EqualsLiteral(GMP_API_DECRYPTOR)) {
    
    return SelectPluginForAPI(aNodeId,
                              NS_LITERAL_CSTRING(GMP_API_DECRYPTOR_COMPAT),
                              aTags);
  }

  return nullptr;
}

class CreateGMPParentTask : public nsRunnable {
public:
  NS_IMETHOD Run() {
    MOZ_ASSERT(NS_IsMainThread());
#if defined(XP_LINUX) && defined(MOZ_GMP_SANDBOX)
    if (!SandboxInfo::Get().CanSandboxMedia()) {
      if (!Preferences::GetBool("media.gmp.insecure.allow")) {
        NS_WARNING("Denying media plugin load due to lack of sandboxing.");
        return NS_ERROR_NOT_AVAILABLE;
      }
      NS_WARNING("Loading media plugin despite lack of sandboxing.");
    }
#endif
    mParent = new GMPParent();
    return NS_OK;
  }
  already_AddRefed<GMPParent> GetParent() {
    return mParent.forget();
  }
private:
  nsRefPtr<GMPParent> mParent;
};

GMPParent*
GeckoMediaPluginServiceParent::ClonePlugin(const GMPParent* aOriginal)
{
  MOZ_ASSERT(aOriginal);

  
  
  nsRefPtr<CreateGMPParentTask> task(new CreateGMPParentTask());
  if (!NS_IsMainThread()) {
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    MOZ_ASSERT(mainThread);
    mozilla::SyncRunnable::DispatchToThread(mainThread, task);
  }

  nsRefPtr<GMPParent> gmp = task->GetParent();
  nsresult rv = gmp->CloneFrom(aOriginal);

  if (NS_FAILED(rv)) {
    NS_WARNING("Can't Create GMPParent");
    return nullptr;
  }

  MutexAutoLock lock(mMutex);
  mPlugins.AppendElement(gmp);

  return gmp.get();
}

class NotifyObserversTask final : public nsRunnable {
public:
  explicit NotifyObserversTask(const char* aTopic, nsString aData = EmptyString())
    : mTopic(aTopic)
    , mData(aData)
  {}
  NS_IMETHOD Run() override {
    MOZ_ASSERT(NS_IsMainThread());
    nsCOMPtr<nsIObserverService> obsService = mozilla::services::GetObserverService();
    MOZ_ASSERT(obsService);
    if (obsService) {
      obsService->NotifyObservers(nullptr, mTopic, mData.get());
    }
    return NS_OK;
  }
private:
  ~NotifyObserversTask() {}
  const char* mTopic;
  const nsString mData;
};

void
GeckoMediaPluginServiceParent::AddOnGMPThread(const nsAString& aDirectory)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);
  LOGD(("%s::%s: %s", __CLASS__, __FUNCTION__, NS_LossyConvertUTF16toASCII(aDirectory).get()));

  nsCOMPtr<nsIFile> directory;
  nsresult rv = NS_NewLocalFile(aDirectory, false, getter_AddRefs(directory));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  
  
  nsRefPtr<CreateGMPParentTask> task(new CreateGMPParentTask());
  nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
  MOZ_ASSERT(mainThread);
  mozilla::SyncRunnable::DispatchToThread(mainThread, task);
  nsRefPtr<GMPParent> gmp = task->GetParent();
  rv = gmp ? gmp->Init(this, directory) : NS_ERROR_NOT_AVAILABLE;
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't Create GMPParent");
    return;
  }

  {
    MutexAutoLock lock(mMutex);
    mPlugins.AppendElement(gmp);
  }

  NS_DispatchToMainThread(new NotifyObserversTask("gmp-path-added"), NS_DISPATCH_NORMAL);
}

void
GeckoMediaPluginServiceParent::RemoveOnGMPThread(const nsAString& aDirectory,
                                                 const bool aDeleteFromDisk,
                                                 const bool aCanDefer)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);
  LOGD(("%s::%s: %s", __CLASS__, __FUNCTION__, NS_LossyConvertUTF16toASCII(aDirectory).get()));

  nsCOMPtr<nsIFile> directory;
  nsresult rv = NS_NewLocalFile(aDirectory, false, getter_AddRefs(directory));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  
  
  nsTArray<nsRefPtr<GMPParent>> deadPlugins;

  bool inUse = false;
  MutexAutoLock lock(mMutex);
  for (size_t i = mPlugins.Length() - 1; i < mPlugins.Length(); i--) {
    nsCOMPtr<nsIFile> pluginpath = mPlugins[i]->GetDirectory();
    bool equals;
    if (NS_FAILED(directory->Equals(pluginpath, &equals)) || !equals) {
      continue;
    }

    nsRefPtr<GMPParent> gmp = mPlugins[i];
    if (aDeleteFromDisk && gmp->State() != GMPStateNotLoaded) {
      
      
      inUse = true;
      gmp->MarkForDeletion();

      if (!mPluginsWaitingForDeletion.Contains(aDirectory)) {
        mPluginsWaitingForDeletion.AppendElement(aDirectory);
      }
    }

    if (gmp->State() == GMPStateNotLoaded || !aCanDefer) {
      
      deadPlugins.AppendElement(gmp);
      mPlugins.RemoveElementAt(i);
    }
  }

  {
    MutexAutoUnlock unlock(mMutex);
    for (auto& gmp : deadPlugins) {
      gmp->AbortAsyncShutdown();
      gmp->CloseActive(true);
    }
  }

  if (aDeleteFromDisk && !inUse) {
    if (NS_SUCCEEDED(directory->Remove(true))) {
      mPluginsWaitingForDeletion.RemoveElement(aDirectory);
      NS_DispatchToMainThread(new NotifyObserversTask("gmp-directory-deleted",
                                                      nsString(aDirectory)),
                              NS_DISPATCH_NORMAL);
    }
  }
}


static void Dummy(nsRefPtr<GMPParent>& aOnDeathsDoor)
{
  
  
}

void
GeckoMediaPluginServiceParent::PluginTerminated(const nsRefPtr<GMPParent>& aPlugin)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);

  if (aPlugin->IsMarkedForDeletion()) {
    nsCString path8;
    nsRefPtr<nsIFile> dir = aPlugin->GetDirectory();
    nsresult rv = dir->GetNativePath(path8);
    NS_ENSURE_SUCCESS_VOID(rv);

    nsString path = NS_ConvertUTF8toUTF16(path8);
    if (mPluginsWaitingForDeletion.Contains(path)) {
      RemoveOnGMPThread(path, true , true );
    }
  }
}

void
GeckoMediaPluginServiceParent::ReAddOnGMPThread(const nsRefPtr<GMPParent>& aOld)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);
  LOGD(("%s::%s: %p", __CLASS__, __FUNCTION__, (void*) aOld));

  nsRefPtr<GMPParent> gmp;
  if (!mShuttingDownOnGMPThread) {
    
    gmp = ClonePlugin(aOld);
  }
  
  
  
  MutexAutoLock lock(mMutex);
  mPlugins.RemoveElement(aOld);

  
  
  NS_DispatchToCurrentThread(WrapRunnableNM(&Dummy, aOld));
}

NS_IMETHODIMP
GeckoMediaPluginServiceParent::GetStorageDir(nsIFile** aOutFile)
{
  if (NS_WARN_IF(!mStorageBaseDir)) {
    return NS_ERROR_FAILURE;
  }
  return mStorageBaseDir->Clone(aOutFile);
}

static nsresult
WriteToFile(nsIFile* aPath,
            const nsCString& aFileName,
            const nsCString& aData)
{
  nsCOMPtr<nsIFile> path;
  nsresult rv = aPath->Clone(getter_AddRefs(path));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = path->AppendNative(aFileName);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  PRFileDesc* f = nullptr;
  rv = path->OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE, PR_IRWXU, &f);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  int32_t len = PR_Write(f, aData.get(), aData.Length());
  PR_Close(f);
  if (NS_WARN_IF(len < 0 || (size_t)len != aData.Length())) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

static nsresult
ReadFromFile(nsIFile* aPath,
             const nsACString& aFileName,
             nsACString& aOutData,
             int32_t aMaxLength)
{
  nsCOMPtr<nsIFile> path;
  nsresult rv = aPath->Clone(getter_AddRefs(path));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = path->AppendNative(aFileName);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  PRFileDesc* f = nullptr;
  rv = path->OpenNSPRFileDesc(PR_RDONLY | PR_CREATE_FILE, PR_IRWXU, &f);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  auto size = PR_Seek(f, 0, PR_SEEK_END);
  PR_Seek(f, 0, PR_SEEK_SET);

  if (size > aMaxLength) {
    return NS_ERROR_FAILURE;
  }
  aOutData.SetLength(size);

  auto len = PR_Read(f, aOutData.BeginWriting(), size);
  PR_Close(f);
  if (NS_WARN_IF(len != size)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
ReadSalt(nsIFile* aPath, nsACString& aOutData)
{
  return ReadFromFile(aPath, NS_LITERAL_CSTRING("salt"),
                      aOutData, NodeIdSaltLength);

}

NS_IMETHODIMP
GeckoMediaPluginServiceParent::IsPersistentStorageAllowed(const nsACString& aNodeId,
                                                          bool* aOutAllowed)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);
  NS_ENSURE_ARG(aOutAllowed);
  *aOutAllowed = mPersistentStorageAllowed.Get(aNodeId);
  return NS_OK;
}

nsresult
GeckoMediaPluginServiceParent::GetNodeId(const nsAString& aOrigin,
                                         const nsAString& aTopLevelOrigin,
                                         bool aInPrivateBrowsing,
                                         nsACString& aOutId)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);
  LOGD(("%s::%s: (%s, %s), %s", __CLASS__, __FUNCTION__,
       NS_ConvertUTF16toUTF8(aOrigin).get(),
       NS_ConvertUTF16toUTF8(aTopLevelOrigin).get(),
       (aInPrivateBrowsing ? "PrivateBrowsing" : "NonPrivateBrowsing")));

  nsresult rv;

  if (aOrigin.EqualsLiteral("null") ||
      aOrigin.IsEmpty() ||
      aTopLevelOrigin.EqualsLiteral("null") ||
      aTopLevelOrigin.IsEmpty()) {
    
    
    
    nsAutoCString salt;
    rv = GenerateRandomPathName(salt, NodeIdSaltLength);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
    aOutId = salt;
    mPersistentStorageAllowed.Put(salt, false);
    return NS_OK;
  }

  const uint32_t hash = AddToHash(HashString(aOrigin),
                                  HashString(aTopLevelOrigin));

  if (aInPrivateBrowsing) {
    
    
    
    nsCString* salt = nullptr;
    if (!(salt = mTempNodeIds.Get(hash))) {
      
      nsAutoCString newSalt;
      rv = GenerateRandomPathName(newSalt, NodeIdSaltLength);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
      salt = new nsCString(newSalt);
      mTempNodeIds.Put(hash, salt);
      mPersistentStorageAllowed.Put(*salt, false);
    }
    aOutId = *salt;
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIFile> path; 
  rv = GetStorageDir(getter_AddRefs(path));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = path->AppendNative(NS_LITERAL_CSTRING("id"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  rv = path->Create(nsIFile::DIRECTORY_TYPE, 0700);
  if (rv != NS_ERROR_FILE_ALREADY_EXISTS && NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsAutoCString hashStr;
  hashStr.AppendInt((int64_t)hash);

  
  rv = path->AppendNative(hashStr);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = path->Create(nsIFile::DIRECTORY_TYPE, 0700);
  if (rv != NS_ERROR_FILE_ALREADY_EXISTS && NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIFile> saltFile;
  rv = path->Clone(getter_AddRefs(saltFile));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = saltFile->AppendNative(NS_LITERAL_CSTRING("salt"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsAutoCString salt;
  bool exists = false;
  rv = saltFile->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  if (!exists) {
    
    
    nsresult rv = GenerateRandomPathName(salt, NodeIdSaltLength);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
    MOZ_ASSERT(salt.Length() == NodeIdSaltLength);

    
    rv = WriteToFile(path, NS_LITERAL_CSTRING("salt"), salt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    
    rv = WriteToFile(path,
                     NS_LITERAL_CSTRING("origin"),
                     NS_ConvertUTF16toUTF8(aOrigin));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    
    rv = WriteToFile(path,
                     NS_LITERAL_CSTRING("topLevelOrigin"),
                     NS_ConvertUTF16toUTF8(aTopLevelOrigin));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

  } else {
    rv = ReadSalt(path, salt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  aOutId = salt;
  mPersistentStorageAllowed.Put(salt, true);

  return NS_OK;
}

NS_IMETHODIMP
GeckoMediaPluginServiceParent::GetNodeId(const nsAString& aOrigin,
                                         const nsAString& aTopLevelOrigin,
                                         bool aInPrivateBrowsing,
                                         UniquePtr<GetNodeIdCallback>&& aCallback)
{
  nsCString nodeId;
  nsresult rv = GetNodeId(aOrigin, aTopLevelOrigin, aInPrivateBrowsing, nodeId);
  aCallback->Done(rv, nodeId);
  return rv;
}

static bool
ExtractHostName(const nsACString& aOrigin, nsACString& aOutData)
{
  nsCString str;
  str.Assign(aOrigin);
  int begin = str.Find("://");
  
  if (begin == -1) {
    return false;
  }

  int end = str.RFind(":");
  
  if (end != begin) {
    str.SetLength(end);
  }

  nsDependentCSubstring host(str, begin + 3);
  aOutData.Assign(host);
  return true;
}

bool
MatchOrigin(nsIFile* aPath, const nsACString& aSite)
{
  
  static const uint32_t MaxDomainLength = 253;

  nsresult rv;
  nsCString str;
  rv = ReadFromFile(aPath, NS_LITERAL_CSTRING("origin"), str, MaxDomainLength);
  if (NS_SUCCEEDED(rv) && ExtractHostName(str, str) && str.Equals(aSite)) {
    return true;
  }
  rv = ReadFromFile(aPath, NS_LITERAL_CSTRING("topLevelOrigin"), str, MaxDomainLength);
  if (NS_SUCCEEDED(rv) && ExtractHostName(str, str) && str.Equals(aSite)) {
    return true;
  }
  return false;
}

template<typename T> static void
KillPlugins(const nsTArray<nsRefPtr<GMPParent>>& aPlugins,
            Mutex& aMutex, T&& aFilter)
{
  
  
  
  
  
  
  nsTArray<nsRefPtr<GMPParent>> pluginsToKill;
  {
    MutexAutoLock lock(aMutex);
    for (size_t i = 0; i < aPlugins.Length(); i++) {
      nsRefPtr<GMPParent> parent(aPlugins[i]);
      if (aFilter(parent)) {
        pluginsToKill.AppendElement(parent);
      }
    }
  }

  for (size_t i = 0; i < pluginsToKill.Length(); i++) {
    pluginsToKill[i]->CloseActive(false);
    
    
    pluginsToKill[i]->AbortAsyncShutdown();
  }
}

static nsresult
DeleteDir(nsIFile* aPath)
{
  bool exists = false;
  nsresult rv = aPath->Exists(&exists);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (exists) {
    return aPath->Remove(true);
  }
  return NS_OK;
}

struct NodeFilter {
  explicit NodeFilter(const nsTArray<nsCString>& nodeIDs) : mNodeIDs(nodeIDs) {}
  bool operator()(GMPParent* aParent) {
    return mNodeIDs.Contains(aParent->GetNodeId());
  }
private:
  const nsTArray<nsCString>& mNodeIDs;
};

void
GeckoMediaPluginServiceParent::ClearNodeIdAndPlugin(DirectoryFilter& aFilter)
{
  nsresult rv;
  nsCOMPtr<nsIFile> path;

  
  rv = GetStorageDir(getter_AddRefs(path));
  if (NS_FAILED(rv)) {
    return;
  }

  
  rv = path->AppendNative(NS_LITERAL_CSTRING("id"));
  if (NS_FAILED(rv)) {
    return;
  }

  
  nsCOMPtr<nsISimpleEnumerator> iter;
  rv = path->GetDirectoryEntries(getter_AddRefs(iter));
  if (NS_FAILED(rv)) {
    return;
  }

  bool hasMore = false;
  nsTArray<nsCString> nodeIDsToClear;
  while (NS_SUCCEEDED(iter->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = iter->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv)) {
      continue;
    }

    
    nsCOMPtr<nsIFile> dirEntry(do_QueryInterface(supports, &rv));
    if (NS_FAILED(rv)) {
      continue;
    }

    
    bool isDirectory = false;
    rv = dirEntry->IsDirectory(&isDirectory);
    if (NS_FAILED(rv) || !isDirectory) {
      continue;
    }

    if (!aFilter(dirEntry)) {
      continue;
    }

    nsAutoCString salt;
    if (NS_SUCCEEDED(ReadSalt(dirEntry, salt))) {
      
      nodeIDsToClear.AppendElement(salt);
      
      mPersistentStorageAllowed.Remove(salt);
    }
    
    if (NS_FAILED(dirEntry->Remove(true))) {
      NS_WARNING("Failed to delete the directory for the origin pair");
    }
  }

  
  KillPlugins(mPlugins, mMutex, NodeFilter(nodeIDsToClear));

  
  rv = GetStorageDir(getter_AddRefs(path));
  if (NS_FAILED(rv)) {
    return;
  }

  rv = path->AppendNative(NS_LITERAL_CSTRING("storage"));
  if (NS_FAILED(rv)) {
    return;
  }

  for (size_t i = 0; i < nodeIDsToClear.Length(); i++) {
    nsCOMPtr<nsIFile> dirEntry;
    rv = path->Clone(getter_AddRefs(dirEntry));
    if (NS_FAILED(rv)) {
      continue;
    }

    rv = dirEntry->AppendNative(nodeIDsToClear[i]);
    if (NS_FAILED(rv)) {
      continue;
    }

    if (NS_FAILED(DeleteDir(dirEntry))) {
      NS_WARNING("Failed to delete GMP storage directory for the node");
    }
  }
}

void
GeckoMediaPluginServiceParent::ForgetThisSiteOnGMPThread(const nsACString& aSite)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);
  LOGD(("%s::%s: origin=%s", __CLASS__, __FUNCTION__, aSite.Data()));

  struct OriginFilter : public DirectoryFilter {
    explicit OriginFilter(const nsACString& aSite) : mSite(aSite) {}
    virtual bool operator()(nsIFile* aPath) {
      return MatchOrigin(aPath, mSite);
    }
  private:
    const nsACString& mSite;
  } filter(aSite);

  ClearNodeIdAndPlugin(filter);
}

void
GeckoMediaPluginServiceParent::ClearRecentHistoryOnGMPThread(PRTime aSince)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);
  LOGD(("%s::%s: since=%lld", __CLASS__, __FUNCTION__, (int64_t)aSince));

  nsCOMPtr<nsIFile> storagePath;
  nsCOMPtr<nsIFile> temp;
  if (NS_SUCCEEDED(GetStorageDir(getter_AddRefs(temp))) &&
      NS_SUCCEEDED(temp->AppendNative(NS_LITERAL_CSTRING("storage")))) {
    storagePath = temp.forget();
  }

  struct MTimeFilter : public DirectoryFilter {
    explicit MTimeFilter(PRTime aSince, already_AddRefed<nsIFile> aPath)
      : mSince(aSince), mStoragePath(aPath) {}

    
    bool IsModifiedAfter(nsIFile* aPath) {
      PRTime lastModified;
      nsresult rv = aPath->GetLastModifiedTime(&lastModified);
      if (NS_SUCCEEDED(rv) && lastModified >= mSince) {
        return true;
      }
      
      nsCOMPtr<nsISimpleEnumerator> iter;
      rv = aPath->GetDirectoryEntries(getter_AddRefs(iter));
      if (NS_FAILED(rv)) {
        return false;
      }

      bool hasMore = false;
      while (NS_SUCCEEDED(iter->HasMoreElements(&hasMore)) && hasMore) {
        nsCOMPtr<nsISupports> supports;
        rv = iter->GetNext(getter_AddRefs(supports));
        if (NS_FAILED(rv)) {
          continue;
        }

        nsCOMPtr<nsIFile> path(do_QueryInterface(supports, &rv));
        if (NS_FAILED(rv)) {
          continue;
        }

        if (IsModifiedAfter(path)) {
          return true;
        }
      }
      return false;
    }

    
    virtual bool operator()(nsIFile* aPath) {
      if (IsModifiedAfter(aPath)) {
        return true;
      }

      nsAutoCString salt;
      nsresult rv = ReadSalt(aPath, salt);
      if (NS_FAILED(rv)) {
        return false;
      }

      
      if (!mStoragePath) {
        return false;
      }
      
      nsCOMPtr<nsIFile> path;
      rv = mStoragePath->Clone(getter_AddRefs(path));
      if (NS_FAILED(rv)) {
        return false;
      }

      rv = path->AppendNative(salt);
      return NS_SUCCEEDED(rv) && IsModifiedAfter(path);
    }
  private:
    const PRTime mSince;
    const nsCOMPtr<nsIFile> mStoragePath;
  } filter(aSince, storagePath.forget());

  ClearNodeIdAndPlugin(filter);

  NS_DispatchToMainThread(new NotifyObserversTask("gmp-clear-storage-complete"), NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP
GeckoMediaPluginServiceParent::ForgetThisSite(const nsAString& aSite)
{
  MOZ_ASSERT(NS_IsMainThread());
  return GMPDispatch(NS_NewRunnableMethodWithArg<nsCString>(
      this, &GeckoMediaPluginServiceParent::ForgetThisSiteOnGMPThread,
      NS_ConvertUTF16toUTF8(aSite)));
}

static bool IsNodeIdValid(GMPParent* aParent) {
  return !aParent->GetNodeId().IsEmpty();
}

void
GeckoMediaPluginServiceParent::ClearStorage()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mGMPThread);
  LOGD(("%s::%s", __CLASS__, __FUNCTION__));

  
  KillPlugins(mPlugins, mMutex, &IsNodeIdValid);

  nsCOMPtr<nsIFile> path; 
  nsresult rv = GetStorageDir(getter_AddRefs(path));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  if (NS_FAILED(DeleteDir(path))) {
    NS_WARNING("Failed to delete GMP storage directory");
  }
  NS_DispatchToMainThread(new NotifyObserversTask("gmp-clear-storage-complete"), NS_DISPATCH_NORMAL);
}

GMPServiceParent::~GMPServiceParent()
{
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new DeleteTask<Transport>(GetTransport()));
}

bool
GMPServiceParent::RecvLoadGMP(const nsCString& aNodeId,
                              const nsCString& aAPI,
                              nsTArray<nsCString>&& aTags,
                              nsTArray<ProcessId>&& aAlreadyBridgedTo,
                              ProcessId* aId,
                              nsCString* aDisplayName,
                              nsCString* aPluginId)
{
  nsRefPtr<GMPParent> gmp = mService->SelectPluginForAPI(aNodeId, aAPI, aTags);

#ifdef PR_LOGGING
  nsCString api = aTags[0];
  LOGD(("%s: %p returning %p for api %s", __FUNCTION__, (void *)this, (void *)gmp, api.get()));
#endif

  if (!gmp || !gmp->EnsureProcessLoaded(aId)) {
    return false;
  }

  *aDisplayName = gmp->GetDisplayName();
  *aPluginId = gmp->GetPluginId();

  return aAlreadyBridgedTo.Contains(*aId) || gmp->Bridge(this);
}

bool
GMPServiceParent::RecvGetGMPNodeId(const nsString& aOrigin,
                                   const nsString& aTopLevelOrigin,
                                   const bool& aInPrivateBrowsing,
                                   nsCString* aID)
{
  nsresult rv = mService->GetNodeId(aOrigin, aTopLevelOrigin,
                                    aInPrivateBrowsing, *aID);
  return NS_SUCCEEDED(rv);
}


bool
GMPServiceParent::RecvGetGMPPluginVersionForAPI(const nsCString& aAPI,
                                                nsTArray<nsCString>&& aTags,
                                                bool* aHasPlugin,
                                                nsCString* aVersion)
{
  nsRefPtr<GeckoMediaPluginServiceParent> service =
    GeckoMediaPluginServiceParent::GetSingleton();
  return service &&
         NS_SUCCEEDED(service->GetPluginVersionForAPI(aAPI, &aTags, aHasPlugin,
                                                      *aVersion));
}

class DeleteGMPServiceParent : public nsRunnable
{
public:
  explicit DeleteGMPServiceParent(GMPServiceParent* aToDelete)
    : mToDelete(aToDelete)
  {
  }

  NS_IMETHODIMP Run()
  {
    return NS_OK;
  }

private:
  nsAutoPtr<GMPServiceParent> mToDelete;
};

void
GMPServiceParent::ActorDestroy(ActorDestroyReason aWhy)
{
  NS_DispatchToCurrentThread(new DeleteGMPServiceParent(this));
}

class OpenPGMPServiceParent : public nsRunnable
{
public:
  OpenPGMPServiceParent(GMPServiceParent* aGMPServiceParent,
                        mozilla::ipc::Transport* aTransport,
                        base::ProcessId aOtherPid,
                        bool* aResult)
    : mGMPServiceParent(aGMPServiceParent),
      mTransport(aTransport),
      mOtherPid(aOtherPid),
      mResult(aResult)
  {
  }

  NS_IMETHOD Run()
  {
    *mResult = mGMPServiceParent->Open(mTransport, mOtherPid,
                                       XRE_GetIOMessageLoop(), ipc::ParentSide);
    return NS_OK;
  }

private:
  GMPServiceParent* mGMPServiceParent;
  mozilla::ipc::Transport* mTransport;
  base::ProcessId mOtherPid;
  bool* mResult;
};


PGMPServiceParent*
GMPServiceParent::Create(Transport* aTransport, ProcessId aOtherPid)
{
  nsRefPtr<GeckoMediaPluginServiceParent> gmp =
    GeckoMediaPluginServiceParent::GetSingleton();

  nsAutoPtr<GMPServiceParent> serviceParent(new GMPServiceParent(gmp));

  nsCOMPtr<nsIThread> gmpThread;
  nsresult rv = gmp->GetThread(getter_AddRefs(gmpThread));
  NS_ENSURE_SUCCESS(rv, nullptr);

  bool ok;
  rv = gmpThread->Dispatch(new OpenPGMPServiceParent(serviceParent,
                                                     aTransport,
                                                     aOtherPid, &ok),
                           NS_DISPATCH_SYNC);
  if (NS_FAILED(rv) || !ok) {
    return nullptr;
  }

  return serviceParent.forget();
}

} 
} 
