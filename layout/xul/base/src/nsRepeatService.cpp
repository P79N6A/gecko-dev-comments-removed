











































#include "nsRepeatService.h"
#include "nsIServiceManager.h"

#if defined(XP_MAC) || defined(XP_MACOSX)
#define INITAL_REPEAT_DELAY 250
#define REPEAT_DELAY        25
#else
#define INITAL_REPEAT_DELAY 250
#define REPEAT_DELAY        50
#endif

nsRepeatService* nsRepeatService::gInstance = nsnull;

nsRepeatService::nsRepeatService()
: mFirstCall(PR_FALSE)
{
}

nsRepeatService::~nsRepeatService()
{
  Stop();
}

nsRepeatService* 
nsRepeatService::GetInstance()
{
  if (!gInstance) {
    gInstance = new nsRepeatService();
    NS_IF_ADDREF(gInstance);
  }
  return gInstance;
}

 void
nsRepeatService::Shutdown()
{
  NS_IF_RELEASE(gInstance);
}

void nsRepeatService::Start(nsITimerCallback* aCallback)
{
  NS_PRECONDITION(aCallback != nsnull, "null ptr");
  if (! aCallback)
    return;

  mCallback = aCallback;
  nsresult rv;
  mRepeatTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);

  if (NS_OK == rv)  {
    mRepeatTimer->InitWithCallback(this, INITAL_REPEAT_DELAY, nsITimer::TYPE_ONE_SHOT);
  }

}

void nsRepeatService::Stop()
{
  
  if (mRepeatTimer) {
     mRepeatTimer->Cancel();
     mRepeatTimer = nsnull;
     mCallback = nsnull;
  }
}

NS_IMETHODIMP nsRepeatService::Notify(nsITimer *timer)
{
   
  if (mRepeatTimer) {
     mRepeatTimer->Cancel();
  }

  
  if (mCallback)
    mCallback->Notify(timer);

  
  if (mRepeatTimer) {
     mRepeatTimer = do_CreateInstance("@mozilla.org/timer;1");
     mRepeatTimer->InitWithCallback(this, REPEAT_DELAY, nsITimer::TYPE_ONE_SHOT);
  }
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsRepeatService, nsITimerCallback)

