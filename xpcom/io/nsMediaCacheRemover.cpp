



#ifndef XP_WIN
#error nsMediaCacheRemover only needed on Windows.
#endif

#include "nsIObserver.h"
#include "nsIIdleService.h"
#include "nsISimpleEnumerator.h"
#include "nsIFile.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsXULAppAPI.h"
#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsITimer.h"



#define TEMP_FILE_IDLE_TIME 30



#define SCHEDULE_TIMEOUT 3 * 60 * 1000








class nsMediaCacheRemover : public nsIObserver {
public:
  NS_DECL_ISUPPORTS

  nsMediaCacheRemover() {
    MOZ_COUNT_CTOR(nsMediaCacheRemover);
  }

  ~nsMediaCacheRemover() {
    MOZ_COUNT_DTOR(nsMediaCacheRemover);
  }

  NS_IMETHODIMP Observe(nsISupports *subject,
                        const char *topic,
                        const PRUnichar *data)
  {
    if (strcmp(topic, "idle") == 0) {
      
      
      
      RemoveMediaCacheFiles();
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

  void RemoveMediaCacheFiles() {
    nsCOMPtr<nsIIdleService> idleSvc =
      do_GetService("@mozilla.org/widget/idleservice;1");
    if (idleSvc)
      idleSvc->RemoveIdleObserver(this, TEMP_FILE_IDLE_TIME);

    nsCOMPtr<nsIFile> tmpDir;
    nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR,
                                         getter_AddRefs(tmpDir));
    if (NS_FAILED(rv))
      return;

    NS_ABORT_IF_FALSE(XRE_GetProcessType() == GeckoProcessType_Default,
                      "Need to update media cache file location");

    rv = tmpDir->AppendNative(nsDependentCString("mozilla-media-cache"));
    if (NS_FAILED(rv))
      return;

    
    tmpDir->Remove(true);
  }
};

NS_IMPL_ISUPPORTS1(nsMediaCacheRemover, nsIObserver)

void CreateMediaCacheRemover(nsITimer* aTimer, void* aClosure) {
  
  
  
  
  nsRefPtr<nsMediaCacheRemover> t = new nsMediaCacheRemover();
  t->RegisterIdleObserver();
}

nsresult ScheduleMediaCacheRemover() {
  
  
  
  
  
  nsCOMPtr<nsITimer> t = do_CreateInstance(NS_TIMER_CONTRACTID);
  nsresult res = t->InitWithFuncCallback(CreateMediaCacheRemover,
                                         0,
                                         SCHEDULE_TIMEOUT,
                                         nsITimer::TYPE_ONE_SHOT);
  return res;
}
