

































#include "gfxCrashReporterUtils.h"

#if defined(MOZ_CRASHREPORTER)
#define MOZ_GFXFEATUREREPORTER 1
#endif

#ifdef MOZ_GFXFEATUREREPORTER
#include "nsExceptionHandler.h"
#include "nsString.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsAutoPtr.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Services.h"

namespace mozilla {

static nsTArray<nsCString> *gFeaturesAlreadyReported = nsnull;

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
      gFeaturesAlreadyReported = nsnull;
    }
  }
  return NS_OK;
}


void
ScopedGfxFeatureReporter::WriteAppNote(char statusChar)
{
  
  
  
  
  
  
  if (!gFeaturesAlreadyReported) {
    nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
    if (!observerService)
      return;
    nsRefPtr<ObserverToDestroyFeaturesAlreadyReported> observer = new ObserverToDestroyFeaturesAlreadyReported;
    nsresult rv = observerService->AddObserver(observer, "xpcom-shutdown", PR_FALSE);
    if (NS_FAILED(rv)) {
      observer = nsnull;
      return;
    }
    gFeaturesAlreadyReported = new nsTArray<nsCString>;
  }

  nsCAutoString featureString;
  featureString.AppendPrintf("%s%c%c",
                             mFeature,
                             statusChar,
                             statusChar == '?' ? ' ' : '\n');

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
