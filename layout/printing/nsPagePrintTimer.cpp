




#include "nsPagePrintTimer.h"
#include "nsIContentViewer.h"
#include "nsIServiceManager.h"
#include "nsPrintEngine.h"

NS_IMPL_ISUPPORTS_INHERITED1(nsPagePrintTimer, nsRunnable, nsITimerCallback)

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

nsresult
nsPagePrintTimer::StartWatchDogTimer()
{
  nsresult result;
  if (mWatchDogTimer) {
    mWatchDogTimer->Cancel();
  }
  mWatchDogTimer = do_CreateInstance("@mozilla.org/timer;1", &result);
  if (NS_FAILED(result)) {
    NS_WARNING("unable to start the timer");
  } else {
    
    
    mWatchDogTimer->InitWithCallback(this, WATCH_DOG_INTERVAL,
                                     nsITimer::TYPE_ONE_SHOT);
  }
  return result;
}

void
nsPagePrintTimer::StopWatchDogTimer()
{
  if (mWatchDogTimer) {
    mWatchDogTimer->Cancel();
    mWatchDogTimer = nullptr;
  }
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
      mDone = true;
    }
  }

  
  
  
  Stop(); 
  if (initNewTimer) {
    ++mFiringCount;
    nsresult result = StartTimer(inRange);
    if (NS_FAILED(result)) {
      mDone = true;     
      mPrintEngine->SetIsPrinting(false);
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsPagePrintTimer::Notify(nsITimer *timer)
{
  
  
  if (mDone) {
    return NS_OK;
  }

  
  
  
  
  if (timer && timer == mWatchDogTimer) {
    mWatchDogCount++;
    if (mWatchDogCount > WATCH_DOG_MAX_COUNT) {
      Fail();
      return NS_OK;
    }
  } else if(!timer) {
    
    mWatchDogCount = 0;
  }

  if (mDocViewerPrint) {
    bool donePrePrint = mPrintEngine->PrePrintPage();

    if (donePrePrint) {
      StopWatchDogTimer();
      NS_DispatchToMainThread(this);
    } else {
      
      
      StartWatchDogTimer();
    }

  }
  return NS_OK;
}

nsresult 
nsPagePrintTimer::Start(nsPrintObject* aPO)
{
  mPrintObj = aPO;
  mWatchDogCount = 0;
  mDone = false;
  return StartTimer(false);
}


void  
nsPagePrintTimer::Stop()
{
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nullptr;
  }
  StopWatchDogTimer();
}

void
nsPagePrintTimer::Fail()
{
  mDone = true;
  Stop();
  if (mPrintEngine) {
    mPrintEngine->CleanupOnFailure(NS_OK, false);
  }
}
