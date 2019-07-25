




#ifndef mozilla_dom_power_WakeLock_h
#define mozilla_dom_power_WakeLock_h

#include "nsCOMPtr.h"
#include "nsIDOMWakeLock.h"
#include "nsIDOMEventListener.h"
#include "nsString.h"
#include "nsWeakReference.h"

class nsIDOMWindow;

namespace mozilla {
namespace dom {
namespace power {

class WakeLock
  : public nsIDOMMozWakeLock
  , public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZWAKELOCK
  NS_DECL_NSIDOMEVENTLISTENER

  WakeLock();
  virtual ~WakeLock();

  nsresult Init(const nsAString &aTopic, nsIDOMWindow *aWindow);

private:
  void     DoUnlock();
  void     DoLock();
  void     AttachEventListener();
  void     DetachEventListener();

  bool      mLocked;
  bool      mHidden;
  nsString  mTopic;

  
  nsWeakPtr mWindow;
};

} 
} 
} 

#endif 
