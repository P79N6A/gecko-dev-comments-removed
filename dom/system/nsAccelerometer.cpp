



































#include "nsAccelerometer.h"

#include "nsAutoPtr.h"
#include "nsIDOMEvent.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsIServiceManager.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMOrientationEvent.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"

class nsAcceleration : public nsIAcceleration
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCELERATION

  nsAcceleration(double x, double y, double z);

private:
  ~nsAcceleration();

protected:
  
  double mX, mY, mZ;
};


NS_IMPL_ISUPPORTS1(nsAcceleration, nsIAcceleration)

nsAcceleration::nsAcceleration(double x, double y, double z)
:mX(x), mY(y), mZ(z)
{
}

nsAcceleration::~nsAcceleration()
{
}

NS_IMETHODIMP nsAcceleration::GetX(double *aX)
{
  NS_ENSURE_ARG_POINTER(aX);
  *aX = mX;
  return NS_OK;
}

NS_IMETHODIMP nsAcceleration::GetY(double *aY)
{
  NS_ENSURE_ARG_POINTER(aY);
  *aY = mY;
  return NS_OK;
}

NS_IMETHODIMP nsAcceleration::GetZ(double *aZ)
{
  NS_ENSURE_ARG_POINTER(aZ);
  *aZ = mZ;
  return NS_OK;
}

NS_IMPL_ISUPPORTS2(nsAccelerometer, nsIAccelerometer, nsIAccelerometerUpdate)

nsAccelerometer::nsAccelerometer()
: mLastX(10), 
  mLastY(10),
  mLastZ(10),
  mStarted(PR_FALSE),
  mNewListener(PR_FALSE),
  mUpdateInterval(50), 
  mEnabled(PR_TRUE)
{
  nsCOMPtr<nsIPrefBranch> prefSrv = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefSrv) {
    PRInt32 value;
    nsresult rv = prefSrv->GetIntPref("accelerometer.update.interval", &value);
    if (NS_SUCCEEDED(rv))
      mUpdateInterval = value;

    PRBool bvalue;
    rv = prefSrv->GetBoolPref("accelerometer.enabled", &bvalue);
    if (NS_SUCCEEDED(rv) && bvalue == PR_FALSE)
      mEnabled = PR_FALSE;
  }
}

nsAccelerometer::~nsAccelerometer()
{
  if (mTimeoutTimer)
    mTimeoutTimer->Cancel();
}

void
nsAccelerometer::StartDisconnectTimer()
{
  if (mTimeoutTimer)
    mTimeoutTimer->Cancel();

  mTimeoutTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (mTimeoutTimer)
    mTimeoutTimer->InitWithFuncCallback(TimeoutHandler,
                                        this,
                                        2000, 
                                        nsITimer::TYPE_ONE_SHOT);  
}

void
nsAccelerometer::TimeoutHandler(nsITimer *aTimer, void *aClosure)
{
  
  
  
  nsAccelerometer *self = reinterpret_cast<nsAccelerometer *>(aClosure);
  if (!self) {
    NS_ERROR("no self");
    return;
  }
  
  
  if (self->mListeners.Count() == 0 && self->mWindowListeners.Count() == 0) {
    self->Shutdown();
    self->mStarted = PR_FALSE;
  }
}

NS_IMETHODIMP nsAccelerometer::AddListener(nsIAccelerationListener *aListener)
{
  if (mListeners.IndexOf(aListener) >= 0)
    return NS_OK; 

  if (mStarted == PR_FALSE) {
    mStarted = PR_TRUE;
    mNewListener = PR_TRUE;
    Startup();
  }

  mListeners.AppendObject(aListener);
  return NS_OK;
}

NS_IMETHODIMP nsAccelerometer::RemoveListener(nsIAccelerationListener *aListener)
{
  if (mListeners.IndexOf(aListener) < 0)
    return NS_OK; 

  mListeners.RemoveObject(aListener);
  StartDisconnectTimer();
  return NS_OK;
}

NS_IMETHODIMP nsAccelerometer::AddWindowListener(nsIDOMWindow *aWindow)
{
  if (mWindowListeners.IndexOf(aWindow) >= 0)
    return NS_OK; 

  if (mStarted == PR_FALSE) {
    mStarted = PR_TRUE;
    mNewListener = PR_TRUE;
    Startup();
  }

  mWindowListeners.AppendObject(aWindow);
  return NS_OK;
}

NS_IMETHODIMP nsAccelerometer::RemoveWindowListener(nsIDOMWindow *aWindow)
{
  if (mWindowListeners.IndexOf(aWindow) < 0)
    return NS_OK; 

  mWindowListeners.RemoveObject(aWindow);
  StartDisconnectTimer();

  return NS_OK;
}

NS_IMETHODIMP
nsAccelerometer::AccelerationChanged(double x, double y, double z)
{
  if (!mEnabled)
    return NS_ERROR_NOT_INITIALIZED;

  if (x > 1)
    x = 1;
  if (y > 1)
    y = 1;
  if (z > 1)
    z = 1;
  if (x < -1)
    x = -1;
  if (y < -1)
    y = -1;
  if (z < -1)
    z = -1;

  if (!mNewListener) {
    if (PR_ABS(mLastX - x) < .01 &&
        PR_ABS(mLastY - y) < .01 &&
        PR_ABS(mLastZ - z) < .01)
      return NS_OK;
  }

  mLastX = x;
  mLastY = y;
  mLastZ = z;
  mNewListener = PR_FALSE;

  for (PRUint32 i = mListeners.Count(); i > 0 ; ) {
    --i;
    nsRefPtr<nsIAcceleration> a = new nsAcceleration(x, y, z);
    mListeners[i]->OnAccelerationChange(a);
  }

  for (PRUint32 i = mWindowListeners.Count(); i > 0 ; ) {
    --i;

    nsCOMPtr<nsIDOMDocument> domdoc;
    mWindowListeners[i]->GetDocument(getter_AddRefs(domdoc));

    nsCOMPtr<nsIDOMDocumentEvent> docevent(do_QueryInterface(domdoc));
    nsCOMPtr<nsIDOMEvent> event;

    PRBool defaultActionEnabled = PR_TRUE;

    if (docevent) {
      docevent->CreateEvent(NS_LITERAL_STRING("orientation"), getter_AddRefs(event));

      nsCOMPtr<nsIDOMOrientationEvent> oe = do_QueryInterface(event);

      if (event) {
        oe->InitOrientationEvent(NS_LITERAL_STRING("MozOrientation"),
                                 PR_TRUE,
                                 PR_FALSE,
                                 x,
                                 y,
                                 z);

        nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
        if (privateEvent)
          privateEvent->SetTrusted(PR_TRUE);
        
        nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mWindowListeners[i]));
        target->DispatchEvent(event, &defaultActionEnabled);
      }
    }
  }
  return NS_OK;
}
