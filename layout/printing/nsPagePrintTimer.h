



#ifndef nsPagePrintTimer_h___
#define nsPagePrintTimer_h___


#include "nsITimer.h"

#include "nsIDocumentViewerPrint.h"
#include "nsPrintObject.h"
#include "mozilla/Attributes.h"

class nsPrintEngine;




class nsPagePrintTimer MOZ_FINAL : public nsITimerCallback
{
public:

  NS_DECL_ISUPPORTS

  nsPagePrintTimer();
  ~nsPagePrintTimer();

  NS_DECL_NSITIMERCALLBACK

  void Init(nsPrintEngine*          aPrintEngine,
            nsIDocumentViewerPrint* aDocViewerPrint,
            uint32_t                aDelay);

  nsresult Start(nsPrintObject* aPO);

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
