



































#include "nsDeviceMotion.h"

#include "nsAutoPtr.h"
#include "nsIDOMEvent.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsIServiceManager.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMDeviceOrientationEvent.h"
#include "nsIDOMDeviceMotionEvent.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"
#include "nsDOMDeviceMotionEvent.h"

class nsDeviceMotionData : public nsIDeviceMotionData
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDEVICEMOTIONDATA

  nsDeviceMotionData(unsigned long type, double x, double y, double z);

private:
  ~nsDeviceMotionData();

protected:
  unsigned long mType;
  double mX, mY, mZ;
};

nsDeviceMotionData::nsDeviceMotionData(unsigned long type, double x, double y, double z)
  : mType(type), mX(x), mY(y), mZ(z)
{
}

NS_INTERFACE_MAP_BEGIN(nsDeviceMotionData)
NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDeviceMotionData)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDeviceMotionData)
NS_IMPL_RELEASE(nsDeviceMotionData)

nsDeviceMotionData::~nsDeviceMotionData()
{
}

NS_IMETHODIMP nsDeviceMotionData::GetType(PRUint32 *aType)
{
  NS_ENSURE_ARG_POINTER(aType);
  *aType = mType;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceMotionData::GetX(double *aX)
{
  NS_ENSURE_ARG_POINTER(aX);
  *aX = mX;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceMotionData::GetY(double *aY)
{
  NS_ENSURE_ARG_POINTER(aY);
  *aY = mY;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceMotionData::GetZ(double *aZ)
{
  NS_ENSURE_ARG_POINTER(aZ);
  *aZ = mZ;
  return NS_OK;
}

NS_IMPL_ISUPPORTS2(nsDeviceMotion, nsIDeviceMotion, nsIDeviceMotionUpdate)

nsDeviceMotion::nsDeviceMotion()
: mStarted(PR_FALSE),
  mUpdateInterval(50), 
  mEnabled(PR_TRUE)
{
  nsCOMPtr<nsIPrefBranch> prefSrv = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefSrv) {
    PRInt32 value;
    nsresult rv = prefSrv->GetIntPref("device.motion.update.interval", &value);
    if (NS_SUCCEEDED(rv))
      mUpdateInterval = value;

    PRBool bvalue;
    rv = prefSrv->GetBoolPref("device.motion.enabled", &bvalue);
    if (NS_SUCCEEDED(rv) && bvalue == PR_FALSE)
      mEnabled = PR_FALSE;
  }
}

nsDeviceMotion::~nsDeviceMotion()
{
  if (mTimeoutTimer)
    mTimeoutTimer->Cancel();
}

void
nsDeviceMotion::StartDisconnectTimer()
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
nsDeviceMotion::TimeoutHandler(nsITimer *aTimer, void *aClosure)
{
  
  
  
  nsDeviceMotion *self = reinterpret_cast<nsDeviceMotion *>(aClosure);
  if (!self) {
    NS_ERROR("no self");
    return;
  }
  
  
  if (self->mListeners.Count() == 0 && self->mWindowListeners.Count() == 0) {
    self->Shutdown();
    self->mStarted = PR_FALSE;
  }
}

NS_IMETHODIMP nsDeviceMotion::AddListener(nsIDeviceMotionListener *aListener)
{
  if (mListeners.IndexOf(aListener) >= 0)
    return NS_OK; 

  if (mStarted == PR_FALSE) {
    mStarted = PR_TRUE;
    Startup();
  }

  mListeners.AppendObject(aListener);
  return NS_OK;
}

NS_IMETHODIMP nsDeviceMotion::RemoveListener(nsIDeviceMotionListener *aListener)
{
  if (mListeners.IndexOf(aListener) < 0)
    return NS_OK; 

  mListeners.RemoveObject(aListener);
  StartDisconnectTimer();
  return NS_OK;
}

NS_IMETHODIMP nsDeviceMotion::AddWindowListener(nsIDOMWindow *aWindow)
{
  if (mWindowListeners.IndexOf(aWindow) >= 0)
    return NS_OK; 

  if (mStarted == PR_FALSE) {
    mStarted = PR_TRUE;
    Startup();
  }

  mWindowListeners.AppendObject(aWindow);
  return NS_OK;
}

NS_IMETHODIMP nsDeviceMotion::RemoveWindowListener(nsIDOMWindow *aWindow)
{
  if (mWindowListeners.IndexOf(aWindow) < 0)
    return NS_OK; 

  mWindowListeners.RemoveObject(aWindow);
  StartDisconnectTimer();

  return NS_OK;
}

NS_IMETHODIMP
nsDeviceMotion::DeviceMotionChanged(PRUint32 type, double x, double y, double z)
{
  if (!mEnabled)
    return NS_ERROR_NOT_INITIALIZED;

  for (PRUint32 i = mListeners.Count(); i > 0 ; ) {
    --i;
    nsRefPtr<nsDeviceMotionData> a = new nsDeviceMotionData(type, x, y, z);
    mListeners[i]->OnMotionChange(a);
  }

  for (PRUint32 i = mWindowListeners.Count(); i > 0 ; ) {
    --i;

    nsCOMPtr<nsIDOMDocument> domdoc;
    mWindowListeners[i]->GetDocument(getter_AddRefs(domdoc));

    if (domdoc) {
      nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mWindowListeners[i]);
      if (type == nsIDeviceMotionData::TYPE_ACCELERATION)
        FireDOMMotionEvent(domdoc, target, x, y, z);
      else if (type == nsIDeviceMotionData::TYPE_ORIENTATION)
        FireDOMOrientationEvent(domdoc, target, x, y, z);
    }
  }
  return NS_OK;
}

void
nsDeviceMotion::FireDOMOrientationEvent(nsIDOMDocument *domdoc,
                                         nsIDOMEventTarget *target,
                                         double alpha,
                                         double beta,
                                         double gamma)
{
  nsCOMPtr<nsIDOMEvent> event;
  PRBool defaultActionEnabled = PR_TRUE;
  domdoc->CreateEvent(NS_LITERAL_STRING("DeviceOrientationEvent"), getter_AddRefs(event));

  nsCOMPtr<nsIDOMDeviceOrientationEvent> oe = do_QueryInterface(event);

  if (!oe) {
    return;
  }

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
  
  target->DispatchEvent(event, &defaultActionEnabled);
}


void
nsDeviceMotion::FireDOMMotionEvent(nsIDOMDocument *domdoc,
                                   nsIDOMEventTarget *target,
                                   double x,
                                   double y,
                                   double z) {
  nsCOMPtr<nsIDOMEvent> event;
  PRBool defaultActionEnabled = PR_TRUE;
  domdoc->CreateEvent(NS_LITERAL_STRING("DeviceMotionEvent"), getter_AddRefs(event));

  nsCOMPtr<nsIDOMDeviceMotionEvent> me = do_QueryInterface(event);

  if (!me) {
    return;
}

  
  nsRefPtr<nsDOMDeviceAcceleration> acceleration = new nsDOMDeviceAcceleration(x, y, z);

  me->InitDeviceMotionEvent(NS_LITERAL_STRING("devicemotion"),
                            PR_TRUE,
                            PR_FALSE,
                            nsnull,
                            acceleration,
                            nsnull,
                            0);

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
  if (privateEvent)
    privateEvent->SetTrusted(PR_TRUE);
  
  target->DispatchEvent(event, &defaultActionEnabled);
}

