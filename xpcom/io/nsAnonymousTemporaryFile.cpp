





#include "nsAnonymousTemporaryFile.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsXULAppAPI.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsAppDirectoryServiceDefs.h"

#ifdef XP_WIN
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "nsIIdleService.h"
#include "nsISimpleEnumerator.h"
#include "nsIFile.h"
#include "nsAutoPtr.h"
#include "nsITimer.h"
#include "nsCRT.h"

using namespace mozilla;
#endif






















static nsresult
GetTempDir(nsIFile** aTempDir)
{
  if (NS_WARN_IF(!aTempDir)) {
    return NS_ERROR_INVALID_ARG;
  }
  nsCOMPtr<nsIFile> tmpFile;
  nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tmpFile));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

#ifdef XP_WIN
  
  
  
  rv = tmpFile->AppendNative(nsDependentCString("mozilla-temp-files"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  rv = tmpFile->Create(nsIFile::DIRECTORY_TYPE, 0700);
  if (rv != NS_ERROR_FILE_ALREADY_EXISTS && NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
#endif

  tmpFile.forget(aTempDir);

  return NS_OK;
}

nsresult
NS_OpenAnonymousTemporaryFile(PRFileDesc** aOutFileDesc)
{
  if (NS_WARN_IF(!aOutFileDesc)) {
    return NS_ERROR_INVALID_ARG;
  }
  nsresult rv;

  nsCOMPtr<nsIFile> tmpFile;
  rv = GetTempDir(getter_AddRefs(tmpFile));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  
  
  
  
  nsAutoCString name("mozilla-temp-");
  name.AppendInt(rand());

  rv = tmpFile->AppendNative(name);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = tmpFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0700);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = tmpFile->OpenNSPRFileDesc(PR_RDWR | nsIFile::DELETE_ON_CLOSE,
                                 PR_IRWXU, aOutFileDesc);

  return rv;
}

#ifdef XP_WIN






#define TEMP_FILE_IDLE_TIME_S 30



#define SCHEDULE_TIMEOUT_MS 3 * 60 * 1000

#define XPCOM_SHUTDOWN_TOPIC "xpcom-shutdown"












class nsAnonTempFileRemover MOZ_FINAL : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS

  nsAnonTempFileRemover()
  {
    MOZ_COUNT_CTOR(nsAnonTempFileRemover);
  }

  ~nsAnonTempFileRemover()
  {
    MOZ_COUNT_DTOR(nsAnonTempFileRemover);
  }

  nsresult Init()
  {
    
    
    
    
    
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
    if (NS_WARN_IF(!mTimer)) {
      return NS_ERROR_FAILURE;
    }
    nsresult rv = mTimer->Init(this,
                               SCHEDULE_TIMEOUT_MS,
                               nsITimer::TYPE_ONE_SHOT);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    
    
    nsCOMPtr<nsIObserverService> obsSrv = services::GetObserverService();
    if (NS_WARN_IF(!obsSrv)) {
      return NS_ERROR_FAILURE;
    }
    return obsSrv->AddObserver(this, XPCOM_SHUTDOWN_TOPIC, false);
  }

  void Cleanup()
  {
    
    if (mTimer) {
      mTimer->Cancel();
      mTimer = nullptr;
    }
    
    nsCOMPtr<nsIIdleService> idleSvc =
      do_GetService("@mozilla.org/widget/idleservice;1");
    if (idleSvc) {
      idleSvc->RemoveIdleObserver(this, TEMP_FILE_IDLE_TIME_S);
    }
    
    nsCOMPtr<nsIObserverService> obsSrv = services::GetObserverService();
    if (obsSrv) {
      obsSrv->RemoveObserver(this, XPCOM_SHUTDOWN_TOPIC);
    }
  }

  NS_IMETHODIMP Observe(nsISupports* aSubject,
                        const char* aTopic,
                        const char16_t* aData)
  {
    if (nsCRT::strcmp(aTopic, NS_TIMER_CALLBACK_TOPIC) == 0 &&
        NS_FAILED(RegisterIdleObserver())) {
      Cleanup();
    } else if (nsCRT::strcmp(aTopic, OBSERVER_TOPIC_IDLE) == 0) {
      
      
      
      RemoveAnonTempFileFiles();
      Cleanup();
    } else if (nsCRT::strcmp(aTopic, XPCOM_SHUTDOWN_TOPIC) == 0) {
      Cleanup();
    }
    return NS_OK;
  }

  nsresult RegisterIdleObserver()
  {
    
    
    
    nsCOMPtr<nsIIdleService> idleSvc =
      do_GetService("@mozilla.org/widget/idleservice;1");
    if (!idleSvc) {
      return NS_ERROR_FAILURE;
    }
    return idleSvc->AddIdleObserver(this, TEMP_FILE_IDLE_TIME_S);
  }

  void RemoveAnonTempFileFiles()
  {
    nsCOMPtr<nsIFile> tmpDir;
    nsresult rv = GetTempDir(getter_AddRefs(tmpDir));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }

    
    tmpDir->Remove(true);
  }

private:
  nsCOMPtr<nsITimer> mTimer;
};

NS_IMPL_ISUPPORTS(nsAnonTempFileRemover, nsIObserver)

nsresult
CreateAnonTempFileRemover()
{
  
  
  
  
  
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    return NS_OK;
  }
  nsRefPtr<nsAnonTempFileRemover> tempRemover = new nsAnonTempFileRemover();
  return tempRemover->Init();
}

#endif

