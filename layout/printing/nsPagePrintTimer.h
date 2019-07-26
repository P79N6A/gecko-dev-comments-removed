



#ifndef nsPagePrintTimer_h___
#define nsPagePrintTimer_h___


#include "nsITimer.h"

#include "nsIDocumentViewerPrint.h"
#include "nsPrintObject.h"
#include "mozilla/Attributes.h"
#include "nsThreadUtils.h"

class nsPrintEngine;




class nsPagePrintTimer MOZ_FINAL : public nsITimerCallback,
                                   public nsRunnable
{
public:

  NS_DECL_ISUPPORTS

  nsPagePrintTimer(nsPrintEngine* aPrintEngine,
                   nsIDocumentViewerPrint* aDocViewerPrint,
                   uint32_t aDelay)
    : mPrintEngine(aPrintEngine)
    , mDocViewerPrint(aDocViewerPrint)
    , mDelay(aDelay)
    , mFiringCount(0)
    , mPrintObj(nullptr)
  {
    mDocViewerPrint->IncrementDestroyRefCount();
  }
  ~nsPagePrintTimer();

  NS_DECL_NSITIMERCALLBACK

  nsresult Start(nsPrintObject* aPO);

  NS_IMETHOD Run();

  void Stop();

private:
  nsresult StartTimer(bool aUseDelay);

  nsPrintEngine*             mPrintEngine;
  nsCOMPtr<nsIDocumentViewerPrint> mDocViewerPrint;
  nsCOMPtr<nsITimer>         mTimer;
  uint32_t                   mDelay;
  uint32_t                   mFiringCount;
  nsPrintObject *            mPrintObj;
};


nsresult
NS_NewPagePrintTimer(nsPagePrintTimer **aResult);

#endif 
