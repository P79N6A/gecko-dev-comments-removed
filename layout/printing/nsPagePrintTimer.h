



































#ifndef nsPagePrintTimer_h___
#define nsPagePrintTimer_h___


#include "nsITimer.h"

#include "nsIDocumentViewerPrint.h"
#include "nsPrintObject.h"

class nsPrintEngine;




class nsPagePrintTimer : public nsITimerCallback
{
public:

  NS_DECL_ISUPPORTS

  nsPagePrintTimer();
  ~nsPagePrintTimer();

  NS_DECL_NSITIMERCALLBACK

  void Init(nsPrintEngine*          aPrintEngine,
            nsIDocumentViewerPrint* aDocViewerPrint,
            PRUint32                aDelay);

  nsresult Start(nsPrintObject* aPO);

  void Stop();

private:
  nsresult StartTimer(bool aUseDelay);

  nsPrintEngine*             mPrintEngine;
  nsCOMPtr<nsIDocumentViewerPrint> mDocViewerPrint;
  nsCOMPtr<nsITimer>         mTimer;
  PRUint32                   mDelay;
  PRUint32                   mFiringCount;
  nsPrintObject *            mPrintObj;
};


nsresult
NS_NewPagePrintTimer(nsPagePrintTimer **aResult);

#endif 
