



































#include "nsAccelerometer.h"

#include "nsAutoPtr.h"
#include "nsIDOMEvent.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsIServiceManager.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMDeviceOrientationEvent.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"

class nsAcceleration : public nsIAcceleration
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCELERATION

  nsAcceleration(double alpha, double beta, double gamma);

private:
  ~nsAcceleration();

protected:
  
  double mAlpha, mBeta, mGamma;
};


NS_IMPL_ISUPPORTS1(nsAcceleration, nsIAcceleration)

nsAcceleration::nsAcceleration(double alpha, double beta, double gamma)
:mAlpha(alpha), mBeta(beta), mGamma(gamma)
{
}

nsAcceleration::~nsAcceleration()
{
}

NS_IMETHODIMP nsAcceleration::GetAlpha(double *aAlpha)
{
  NS_ENSURE_ARG_POINTER(aAlpha);
  *aAlpha = mAlpha;
  return NS_OK;
}

NS_IMETHODIMP nsAcceleration::GetBeta(double *aBeta)
{
  NS_ENSURE_ARG_POINTER(aBeta);
  *aBeta = mBeta;
  return NS_OK;
}

NS_IMETHODIMP nsAcceleration::GetGamma(double *aGamma)
{
  NS_ENSURE_ARG_POINTER(aGamma);
  *aGamma = mGamma;
  return NS_OK;
}

NS_IMPL_ISUPPORTS2(nsAccelerometer, nsIAccelerometer, nsIAccelerometerUpdate)

nsAccelerometer::nsAccelerometer()
: mLastAlpha(-200), 
  mLastBeta(-200),
  mLastGamma(-200),
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
nsAccelerometer::AccelerationChanged(double alpha, double beta, double gamma)
{
  if (!mEnabled)
    return NS_ERROR_NOT_INITIALIZED;

  if (alpha > 360)
    alpha = 360;
  if (alpha < 0)
    alpha = 0;

  if (beta > 180)
    beta = 180;
  if (beta < -180)
    beta = -180;

  if (gamma > 90)
    gamma = 90;
  if (gamma < -90)
    gamma = -90;

  if (!mNewListener) {
    if (PR_ABS(mLastAlpha - alpha) < 1 &&
        PR_ABS(mLastBeta - beta) < 1 &&
        PR_ABS(mLastGamma - gamma) < 1)
      return NS_OK;
  }

  mLastAlpha = alpha;
  mLastBeta = beta;
  mLastGamma = gamma;
  mNewListener = PR_FALSE;

  for (PRUint32 i = mListeners.Count(); i > 0 ; ) {
    --i;
    nsRefPtr<nsIAcceleration> a = new nsAcceleration(alpha, beta, gamma);
    mListeners[i]->OnAccelerationChange(a);
  }

  for (PRUint32 i = mWindowListeners.Count(); i > 0 ; ) {
    --i;

    nsCOMPtr<nsIDOMDocument> domdoc;
    mWindowListeners[i]->GetDocument(getter_AddRefs(domdoc));

    nsCOMPtr<nsIDOMEvent> event;

    PRBool defaultActionEnabled = PR_TRUE;

    if (domdoc) {
      domdoc->CreateEvent(NS_LITERAL_STRING("DeviceOrientationEvent"), getter_AddRefs(event));

      nsCOMPtr<nsIDOMDeviceOrientationEvent> oe = do_QueryInterface(event);

      if (event) {
        oe->InitDeviceOrientationEvent(NS_LITERAL_STRING("deviceorientation"),
                                       PR_TRUE,
                                       PR_FALSE,
                                       alpha,
                                       beta,
                                       gamma,
                                       PR_TRUE);

        nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
        if (privateEvent)
          privateEvent->SetTrusted(PR_TRUE);
        
        nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mWindowListeners[i]);
        target->DispatchEvent(event, &defaultActionEnabled);
      }
    }
  }
  return NS_OK;
}
