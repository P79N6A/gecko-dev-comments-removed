





#include "nsAnonymousTemporaryFile.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsXULAppAPI.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsAppDirectoryServiceDefs.h"

#ifdef XP_WIN
#include "nsIObserver.h"
#include "nsIIdleService.h"
#include "nsISimpleEnumerator.h"
#include "nsIFile.h"
#include "nsAutoPtr.h"
#include "nsITimer.h"
#endif












#ifdef XP_WIN
#define ANON_TEMP_DIR NS_APP_USER_PROFILE_LOCAL_50_DIR
#else
#define ANON_TEMP_DIR NS_OS_TEMP_DIR
#endif

nsresult
NS_OpenAnonymousTemporaryFile(PRFileDesc** aOutFileDesc)
{
  NS_ENSURE_ARG(aOutFileDesc);
  nsresult rv;

  nsCOMPtr<nsIFile> tmpFile;
  rv = NS_GetSpecialDirectory(ANON_TEMP_DIR, getter_AddRefs(tmpFile));
  NS_ENSURE_SUCCESS(rv,rv);

#ifdef XP_WIN
  
  
  
  rv = tmpFile->AppendNative(nsDependentCString("mozilla-temp-files"));
  NS_ENSURE_SUCCESS(rv,rv);    
  rv = tmpFile->Create(nsIFile::DIRECTORY_TYPE, 0700);
  NS_ENSURE_TRUE(rv == NS_ERROR_FILE_ALREADY_EXISTS || NS_SUCCEEDED(rv), rv);
#endif

  
  
  
  
  
  
  nsAutoString name(NS_LITERAL_STRING("mozilla-temp-"));
  name.AppendInt(rand());

  rv = tmpFile->Append(name);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = tmpFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0700);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = tmpFile->OpenNSPRFileDesc(PR_RDWR | nsIFile::DELETE_ON_CLOSE,
                                 PR_IRWXU, aOutFileDesc);

  return rv;    
}

#ifdef XP_WIN






#define TEMP_FILE_IDLE_TIME 30



#define SCHEDULE_TIMEOUT 3 * 60 * 1000








class nsAnonTempFileRemover : public nsIObserver {
public:
  NS_DECL_ISUPPORTS

  nsAnonTempFileRemover() {
    MOZ_COUNT_CTOR(nsAnonTempFileRemover);
  }

  ~nsAnonTempFileRemover() {
    MOZ_COUNT_DTOR(nsAnonTempFileRemover);
  }

  NS_IMETHODIMP Observe(nsISupports *subject,
                        const char *topic,
                        const PRUnichar *data)
  {
    if (strcmp(topic, "idle") == 0) {
      
      
      
      RemoveAnonTempFileFiles();
    }
    return NS_OK;
  }

  nsresult RegisterIdleObserver() {
    
    
    
    nsCOMPtr<nsIIdleService> idleSvc =
      do_GetService("@mozilla.org/widget/idleservice;1");
    if (!idleSvc)
      return NS_ERROR_FAILURE;
    return idleSvc->AddIdleObserver(this, TEMP_FILE_IDLE_TIME);
  }

  void RemoveAnonTempFileFiles() {
    nsCOMPtr<nsIIdleService> idleSvc =
      do_GetService("@mozilla.org/widget/idleservice;1");
    if (idleSvc)
      idleSvc->RemoveIdleObserver(this, TEMP_FILE_IDLE_TIME);

    nsCOMPtr<nsIFile> tmpDir;
    nsresult rv = NS_GetSpecialDirectory(ANON_TEMP_DIR,
                                         getter_AddRefs(tmpDir));
    if (NS_FAILED(rv))
      return;

    rv = tmpDir->AppendNative(nsDependentCString("mozilla-temp-files"));
    if (NS_FAILED(rv))
      return;

    
    tmpDir->Remove(true);
  }
};

NS_IMPL_ISUPPORTS1(nsAnonTempFileRemover, nsIObserver)




static nsITimer* sAnonTempFileTimer = nullptr;

void CreateAnonTempFileRemover(nsITimer* aTimer, void* aClosure) {
  
  
  
  
  nsRefPtr<nsAnonTempFileRemover> t = new nsAnonTempFileRemover();
  t->RegisterIdleObserver();
  NS_IF_RELEASE(sAnonTempFileTimer);
  sAnonTempFileTimer = nullptr;
}

nsresult ScheduleAnonTempFileRemover() {
  NS_ASSERTION(sAnonTempFileTimer == nullptr, "Don't init timer twice!");
  
  
  
  
  
  nsCOMPtr<nsITimer> t = do_CreateInstance(NS_TIMER_CONTRACTID);
  if (NS_SUCCEEDED(t->InitWithFuncCallback(CreateAnonTempFileRemover,
                                           nullptr,
                                           SCHEDULE_TIMEOUT,
                                           nsITimer::TYPE_ONE_SHOT))) {
    t.forget(&sAnonTempFileTimer);
  }
  return NS_OK;
}

#endif

