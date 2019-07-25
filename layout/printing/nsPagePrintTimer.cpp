




































#include "nsPagePrintTimer.h"
#include "nsIContentViewer.h"
#include "nsIServiceManager.h"
#include "nsPrintEngine.h"

NS_IMPL_ISUPPORTS1(nsPagePrintTimer, nsITimerCallback)

nsPagePrintTimer::nsPagePrintTimer() :
  mPrintEngine(nsnull),
  mDelay(0),
  mFiringCount(0),
  mPrintObj(nsnull)
{
}

nsPagePrintTimer::~nsPagePrintTimer()
{
  
  
  
  
  nsCOMPtr<nsIContentViewer> cv(do_QueryInterface(mDocViewerPrint));
  if (cv) {
    cv->Destroy();
  }
}

nsresult 
nsPagePrintTimer::StartTimer(bool aUseDelay)
{
  nsresult result;
  mTimer = do_CreateInstance("@mozilla.org/timer;1", &result);
  if (NS_FAILED(result)) {
    NS_WARNING("unable to start the timer");
  } else {
    PRUint32 delay = 0;
    if (aUseDelay) {
      if (mFiringCount < 10) {
        
        delay = mDelay + ((10 - mFiringCount) * 100);
      } else {
        delay = mDelay;
      }
    }
    mTimer->InitWithCallback(this, delay, nsITimer::TYPE_ONE_SHOT);
  }
  return result;
}




NS_IMETHODIMP
nsPagePrintTimer::Notify(nsITimer *timer)
{
  if (mDocViewerPrint) {
    bool initNewTimer = true;
    
    
    bool inRange;
    
    
    bool donePrinting = mPrintEngine->PrintPage(mPrintObj, inRange);
    if (donePrinting) {
      
      if (mPrintEngine->DonePrintingPages(mPrintObj, NS_OK)) {
        initNewTimer = PR_FALSE;
      }
    }

    
    
    
    Stop(); 
    if (initNewTimer) {
      ++mFiringCount;
      nsresult result = StartTimer(inRange);
      if (NS_FAILED(result)) {
        donePrinting = PR_TRUE;     
        mPrintEngine->SetIsPrinting(PR_FALSE);
      }
    }
  }
  return NS_OK;
}

void 
nsPagePrintTimer::Init(nsPrintEngine*          aPrintEngine,
                       nsIDocumentViewerPrint* aDocViewerPrint,
                       PRUint32                aDelay)
{
  mPrintEngine     = aPrintEngine;
  mDocViewerPrint  = aDocViewerPrint;
  mDelay           = aDelay;

  mDocViewerPrint->IncrementDestroyRefCount();
}

nsresult 
nsPagePrintTimer::Start(nsPrintObject* aPO)
{
  mPrintObj = aPO;
  return StartTimer(PR_FALSE);
}


void  
nsPagePrintTimer::Stop()
{
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nsnull;
  }
}

nsresult NS_NewPagePrintTimer(nsPagePrintTimer **aResult)
{

  NS_PRECONDITION(aResult, "null param");

  nsPagePrintTimer* result = new nsPagePrintTimer;

  if (!result) {
    *aResult = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(result);
  *aResult = result;

  return NS_OK;
}

