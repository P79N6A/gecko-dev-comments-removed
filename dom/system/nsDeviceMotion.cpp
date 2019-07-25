



































#include "nsDeviceMotion.h"

#include "nsAutoPtr.h"
#include "nsIDOMEvent.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsIServiceManager.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMDeviceOrientationEvent.h"
#include "nsIDOMDeviceMotionEvent.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"
#include "nsDOMDeviceMotionEvent.h"

static const nsTArray<nsIDOMWindow*>::index_type NoIndex =
    nsTArray<nsIDOMWindow*>::NoIndex;

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
: mStarted(false),
  mUpdateInterval(50), 
  mEnabled(true)
{
  nsCOMPtr<nsIPrefBranch> prefSrv = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefSrv) {
    PRInt32 value;
    nsresult rv = prefSrv->GetIntPref("device.motion.update.interval", &value);
    if (NS_SUCCEEDED(rv))
      mUpdateInterval = value;

    bool bvalue;
    rv = prefSrv->GetBoolPref("device.motion.enabled", &bvalue);
    if (NS_SUCCEEDED(rv) && bvalue == false)
      mEnabled = false;
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
  
  
  if (self->mListeners.Count() == 0 && self->mWindowListeners.Length() == 0) {
    self->Shutdown();
    self->mStarted = false;
  }
}

NS_IMETHODIMP nsDeviceMotion::AddListener(nsIDeviceMotionListener *aListener)
{
  if (mListeners.IndexOf(aListener) != -1)
    return NS_OK; 

  if (mStarted == false) {
    mStarted = true;
    Startup();
  }

  mListeners.AppendObject(aListener);
  return NS_OK;
}

NS_IMETHODIMP nsDeviceMotion::RemoveListener(nsIDeviceMotionListener *aListener)
{
  if (mListeners.IndexOf(aListener) == -1)
    return NS_OK; 

  mListeners.RemoveObject(aListener);
  StartDisconnectTimer();
  return NS_OK;
}

NS_IMETHODIMP nsDeviceMotion::AddWindowListener(nsIDOMWindow *aWindow)
{
  if (mWindowListeners.IndexOf(aWindow) != NoIndex)
      return NS_OK;

  if (mStarted == false) {
    mStarted = true;
    Startup();
  }

  mWindowListeners.AppendElement(aWindow);
  return NS_OK;
}

NS_IMETHODIMP nsDeviceMotion::RemoveWindowListener(nsIDOMWindow *aWindow)
{
  if (mWindowListeners.IndexOf(aWindow) == NoIndex)
    return NS_OK;

  mWindowListeners.RemoveElement(aWindow);
  StartDisconnectTimer();
  return NS_OK;
}

NS_IMETHODIMP
nsDeviceMotion::DeviceMotionChanged(PRUint32 type, double x, double y, double z)
{
  if (!mEnabled)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMArray<nsIDeviceMotionListener> listeners = mListeners;
  for (PRUint32 i = listeners.Count(); i > 0 ; ) {
    --i;
    nsRefPtr<nsDeviceMotionData> a = new nsDeviceMotionData(type, x, y, z);
    listeners[i]->OnMotionChange(a);
  }

  nsCOMArray<nsIDOMWindow> windowListeners;
  for (PRUint32 i = 0; i < mWindowListeners.Length(); i++) {
    windowListeners.AppendObject(mWindowListeners[i]);
  }

  for (PRUint32 i = windowListeners.Count(); i > 0 ; ) {
    --i;

    
    
    nsCOMPtr<nsPIDOMWindow> pwindow = do_QueryInterface(windowListeners[i]);
    if (!pwindow ||
        !pwindow->GetOuterWindow() ||
        pwindow->GetOuterWindow()->IsBackground())
      continue;

    nsCOMPtr<nsIDOMDocument> domdoc;
    windowListeners[i]->GetDocument(getter_AddRefs(domdoc));

    if (domdoc) {
      nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(windowListeners[i]);
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
  bool defaultActionEnabled = true;
  domdoc->CreateEvent(NS_LITERAL_STRING("DeviceOrientationEvent"), getter_AddRefs(event));

  nsCOMPtr<nsIDOMDeviceOrientationEvent> oe = do_QueryInterface(event);

  if (!oe) {
    return;
  }

  oe->InitDeviceOrientationEvent(NS_LITERAL_STRING("deviceorientation"),
                                 true,
                                 false,
                                 alpha,
                                 beta,
                                 gamma,
                                 true);

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
  if (privateEvent)
    privateEvent->SetTrusted(true);
  
  target->DispatchEvent(event, &defaultActionEnabled);
}


void
nsDeviceMotion::FireDOMMotionEvent(nsIDOMDocument *domdoc,
                                   nsIDOMEventTarget *target,
                                   double x,
                                   double y,
                                   double z) {
  nsCOMPtr<nsIDOMEvent> event;
  bool defaultActionEnabled = true;
  domdoc->CreateEvent(NS_LITERAL_STRING("DeviceMotionEvent"), getter_AddRefs(event));

  nsCOMPtr<nsIDOMDeviceMotionEvent> me = do_QueryInterface(event);

  if (!me) {
    return;
}

  
  nsRefPtr<nsDOMDeviceAcceleration> acceleration = new nsDOMDeviceAcceleration(x, y, z);

  me->InitDeviceMotionEvent(NS_LITERAL_STRING("devicemotion"),
                            true,
                            false,
                            nsnull,
                            acceleration,
                            nsnull,
                            0);

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
  if (privateEvent)
    privateEvent->SetTrusted(true);
  
  target->DispatchEvent(event, &defaultActionEnabled);
}

