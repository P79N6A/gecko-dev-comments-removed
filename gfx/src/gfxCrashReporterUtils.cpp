




#include "gfxCrashReporterUtils.h"

#if defined(MOZ_CRASHREPORTER)
#define MOZ_GFXFEATUREREPORTER 1
#endif

#ifdef MOZ_GFXFEATUREREPORTER
#include "gfxCrashReporterUtils.h"
#include <string.h>                     
#include "mozilla/Assertions.h"         
#include "mozilla/Services.h"           
#include "mozilla/StaticMutex.h"
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsError.h"                    
#include "nsExceptionHandler.h"         
#include "nsID.h"
#include "nsIEventTarget.h"             
#include "nsIObserver.h"                
#include "nsIObserverService.h"         
#include "nsIRunnable.h"                
#include "nsISupports.h"
#include "nsString.h"               
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              
#include "nscore.h"                     

namespace mozilla {

static nsTArray<nsCString> *gFeaturesAlreadyReported = nullptr;
static StaticMutex gFeaturesAlreadyReportedMutex;

class ObserverToDestroyFeaturesAlreadyReported final : public nsIObserver
{

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  ObserverToDestroyFeaturesAlreadyReported() {}
private:
  virtual ~ObserverToDestroyFeaturesAlreadyReported() {}
};

NS_IMPL_ISUPPORTS(ObserverToDestroyFeaturesAlreadyReported,
                  nsIObserver)

NS_IMETHODIMP
ObserverToDestroyFeaturesAlreadyReported::Observe(nsISupports* aSubject,
                                                  const char* aTopic,
                                                  const char16_t* aData)
{
  if (!strcmp(aTopic, "xpcom-shutdown")) {
    StaticMutexAutoLock al(gFeaturesAlreadyReportedMutex);
    if (gFeaturesAlreadyReported) {
      delete gFeaturesAlreadyReported;
      gFeaturesAlreadyReported = nullptr;
    }
  }
  return NS_OK;
}

class RegisterObserverRunnable : public nsRunnable {
public:
  NS_IMETHOD Run() {
    
    
    
    
    
    
    nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
    if (!observerService)
      return NS_OK;
    nsRefPtr<ObserverToDestroyFeaturesAlreadyReported> observer = new ObserverToDestroyFeaturesAlreadyReported;
    nsresult rv = observerService->AddObserver(observer, "xpcom-shutdown", false);
    if (NS_FAILED(rv)) {
      observer = nullptr;
      return NS_OK;
    }
    return NS_OK;
  }
};

void
ScopedGfxFeatureReporter::WriteAppNote(char statusChar)
{
  StaticMutexAutoLock al(gFeaturesAlreadyReportedMutex);

  if (!gFeaturesAlreadyReported) {
    gFeaturesAlreadyReported = new nsTArray<nsCString>;
    nsCOMPtr<nsIRunnable> r = new RegisterObserverRunnable();
    NS_DispatchToMainThread(r);
  }

  nsAutoCString featureString;
  featureString.AppendPrintf("%s%c ",
                             mFeature,
                             mStatusChar);

  if (!gFeaturesAlreadyReported->Contains(featureString)) {
    gFeaturesAlreadyReported->AppendElement(featureString);
    CrashReporter::AppendAppNotesToCrashReport(featureString);
  }
}

} 

#else

namespace mozilla {
void ScopedGfxFeatureReporter::WriteAppNote(char) {}
}

#endif
