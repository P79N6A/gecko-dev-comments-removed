











































#include "nsRepeatService.h"
#include "nsIServiceManager.h"

nsRepeatService* nsRepeatService::gInstance = nsnull;

nsRepeatService::nsRepeatService()
: mCallback(nsnull), mCallbackData(nsnull)
{
}

nsRepeatService::~nsRepeatService()
{
  NS_ASSERTION(!mCallback && !mCallbackData, "Callback was not removed before shutdown");
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

void nsRepeatService::Start(Callback aCallback, void* aCallbackData,
                            PRUint32 aInitialDelay)
{
  NS_PRECONDITION(aCallback != nsnull, "null ptr");

  mCallback = aCallback;
  mCallbackData = aCallbackData;
  nsresult rv;
  mRepeatTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);

  if (NS_SUCCEEDED(rv))  {
    mRepeatTimer->InitWithCallback(this, aInitialDelay, nsITimer::TYPE_ONE_SHOT);
  }
}

void nsRepeatService::Stop(Callback aCallback, void* aCallbackData)
{
  if (mCallback != aCallback || mCallbackData != aCallbackData)
    return;

  
  if (mRepeatTimer) {
     mRepeatTimer->Cancel();
     mRepeatTimer = nsnull;
  }
  mCallback = nsnull;
  mCallbackData = nsnull;
}

NS_IMETHODIMP nsRepeatService::Notify(nsITimer *timer)
{
  
  if (mCallback)
    mCallback(mCallbackData);

  
  if (mRepeatTimer) {
    mRepeatTimer->InitWithCallback(this, REPEAT_DELAY, nsITimer::TYPE_ONE_SHOT);
  }
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsRepeatService, nsITimerCallback)
