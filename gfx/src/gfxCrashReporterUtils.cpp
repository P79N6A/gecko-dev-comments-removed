




#include "gfxCrashReporterUtils.h"

#if defined(MOZ_CRASHREPORTER)
#define MOZ_GFXFEATUREREPORTER 1
#endif

#ifdef MOZ_GFXFEATUREREPORTER
#include "nsExceptionHandler.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsAutoPtr.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Services.h"
#include "nsThreadUtils.h"

namespace mozilla {

static nsTArray<nsCString> *gFeaturesAlreadyReported = nullptr;

class ObserverToDestroyFeaturesAlreadyReported : public nsIObserver
{

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  ObserverToDestroyFeaturesAlreadyReported() {}
  virtual ~ObserverToDestroyFeaturesAlreadyReported() {}
};

NS_IMPL_ISUPPORTS1(ObserverToDestroyFeaturesAlreadyReported,
                   nsIObserver)

NS_IMETHODIMP
ObserverToDestroyFeaturesAlreadyReported::Observe(nsISupports* aSubject,
                                                  const char* aTopic,
                                                  const PRUnichar* aData)
{
  if (!strcmp(aTopic, "xpcom-shutdown")) {
    if (gFeaturesAlreadyReported) {
      delete gFeaturesAlreadyReported;
      gFeaturesAlreadyReported = nullptr;
    }
  }
  return NS_OK;
}

class ScopedGfxFeatureReporter::AppNoteWritingRunnable : public nsRunnable {
public:
  AppNoteWritingRunnable(char aStatusChar, const char *aFeature) :
    mStatusChar(aStatusChar), mFeature(aFeature) {}
  NS_IMETHOD Run() { 
    
    
    
    
    
    
    if (!gFeaturesAlreadyReported) {
      nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
      if (!observerService)
        return NS_OK;
      nsRefPtr<ObserverToDestroyFeaturesAlreadyReported> observer = new ObserverToDestroyFeaturesAlreadyReported;
      nsresult rv = observerService->AddObserver(observer, "xpcom-shutdown", false);
      if (NS_FAILED(rv)) {
        observer = nullptr;
        return NS_OK;
      }
      gFeaturesAlreadyReported = new nsTArray<nsCString>;
    }

    nsAutoCString featureString;
    featureString.AppendPrintf("%s%c ",
                               mFeature,
                               mStatusChar);

    if (!gFeaturesAlreadyReported->Contains(featureString)) {
      gFeaturesAlreadyReported->AppendElement(featureString);
      CrashReporter::AppendAppNotesToCrashReport(featureString);
    }
    return NS_OK;
  }
private:
  char mStatusChar;
  const char *mFeature;
};

void
ScopedGfxFeatureReporter::WriteAppNote(char statusChar)
{
  nsCOMPtr<nsIRunnable> r = new AppNoteWritingRunnable(statusChar, mFeature);
  NS_DispatchToMainThread(r.get(), NS_DISPATCH_NORMAL);
}

} 

#else

namespace mozilla {
void ScopedGfxFeatureReporter::WriteAppNote(char) {}
}

#endif
