



































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

NS_IMPL_ISUPPORTS1(nsAccelerometer, nsIAccelerometer)

nsAccelerometer::nsAccelerometer()
: mStarted(PR_FALSE)
{
}

nsAccelerometer::~nsAccelerometer()
{
  if (mTimer)
    mTimer->Cancel();
}

void
nsAccelerometer::startDisconnectTimer()
{
  mTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (mTimer)
    mTimer->InitWithFuncCallback(TimeoutHandler,
                                 this,
                                 2000, 
                                 nsITimer::TYPE_REPEATING_SLACK);  
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
  if (mStarted == PR_FALSE) {
    startDisconnectTimer();
    Startup();
    mStarted = PR_TRUE;
  }

  mListeners.AppendObject(aListener);
  return NS_OK;
}

NS_IMETHODIMP nsAccelerometer::RemoveListener(nsIAccelerationListener *aListener)
{
  mListeners.RemoveObject(aListener);
  return NS_OK;
}

NS_IMETHODIMP nsAccelerometer::AddWindowListener(nsIDOMWindow *aWindow)
{
  if (mStarted == PR_FALSE) {
    startDisconnectTimer();
    Startup(); 
   mStarted = PR_TRUE;
  }

  mWindowListeners.AppendObject(aWindow);
  return NS_OK;
}

NS_IMETHODIMP nsAccelerometer::RemoveWindowListener(nsIDOMWindow *aWindow)
{
  mWindowListeners.RemoveObject(aWindow);
  return NS_OK;
}

void 
nsAccelerometer::AccelerationChanged(double x, double y, double z)
{
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
}
