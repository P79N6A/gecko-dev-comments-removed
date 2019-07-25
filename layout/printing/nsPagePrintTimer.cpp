




#include "nsPagePrintTimer.h"
#include "nsIContentViewer.h"
#include "nsIServiceManager.h"
#include "nsPrintEngine.h"

NS_IMPL_ISUPPORTS1(nsPagePrintTimer, nsITimerCallback)

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
    uint32_t delay = 0;
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
nsPagePrintTimer::Run() 
{
  bool initNewTimer = true;
  
  
  bool inRange;
  bool donePrinting;

  
  
  donePrinting = mPrintEngine->PrintPage(mPrintObj, inRange);
  if (donePrinting) {
    
    if (mPrintEngine->DonePrintingPages(mPrintObj, NS_OK)) {
      initNewTimer = false;
    }
  }

  
  
  
  Stop(); 
  if (initNewTimer) {
    ++mFiringCount;
    nsresult result = StartTimer(inRange);
    if (NS_FAILED(result)) {
      donePrinting = true;     
      mPrintEngine->SetIsPrinting(false);
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsPagePrintTimer::Notify(nsITimer *timer)
{
  if (mDocViewerPrint) {
    bool donePrePrint = mPrintEngine->PrePrintPage();

    if (donePrePrint) {
      NS_DispatchToMainThread(this);
    }

  }
  return NS_OK;
}

nsresult 
nsPagePrintTimer::Start(nsPrintObject* aPO)
{
  mPrintObj = aPO;
  return StartTimer(false);
}


void  
nsPagePrintTimer::Stop()
{
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nullptr;
  }
}
