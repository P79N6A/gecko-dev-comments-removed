




#ifndef mozilla_dom_power_WakeLock_h
#define mozilla_dom_power_WakeLock_h

#include "nsCOMPtr.h"
#include "nsIDOMWakeLock.h"
#include "nsIDOMEventListener.h"
#include "nsIObserver.h"
#include "nsString.h"
#include "nsWeakReference.h"

class nsIDOMWindow;

namespace mozilla {
namespace dom {

class ContentParent;

namespace power {

class WakeLock
  : public nsIDOMMozWakeLock
  , public nsIDOMEventListener
  , public nsIObserver
  , public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZWAKELOCK
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSIOBSERVER

  WakeLock();
  virtual ~WakeLock();

  
  
  
  nsresult Init(const nsAString &aTopic, nsIDOMWindow* aWindow);

  
  
  
  nsresult Init(const nsAString &aTopic, ContentParent* aContentParent);

private:
  void     DoUnlock();
  void     DoLock();
  void     AttachEventListener();
  void     DetachEventListener();

  bool      mLocked;
  bool      mHidden;

  
  
  
  uint64_t  mContentParentID;
  nsString  mTopic;

  
  nsWeakPtr mWindow;
};

} 
} 
} 

#endif 
