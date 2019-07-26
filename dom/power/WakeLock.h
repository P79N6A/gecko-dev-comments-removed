




#ifndef mozilla_dom_power_WakeLock_h
#define mozilla_dom_power_WakeLock_h

#include "nsCOMPtr.h"
#include "nsIDOMEventListener.h"
#include "nsIObserver.h"
#include "nsString.h"
#include "nsWeakReference.h"
#include "nsWrapperCache.h"
#include "mozilla/ErrorResult.h"

class nsIDOMWindow;

namespace mozilla {
namespace dom {

class ContentParent;

class WakeLock MOZ_FINAL
  : public nsIDOMEventListener
  , public nsWrapperCache
  , public nsIObserver
  , public nsSupportsWeakReference
{
public:
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSIOBSERVER

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(WakeLock, nsIDOMEventListener)

  
  
  
  

  WakeLock();

  
  
  
  nsresult Init(const nsAString &aTopic, nsIDOMWindow* aWindow);

  
  
  
  nsresult Init(const nsAString &aTopic, ContentParent* aContentParent);

  

  nsISupports* GetParentObject() const;

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void GetTopic(nsAString& aTopic);

  void Unlock(ErrorResult& aRv);

private:
  virtual ~WakeLock();

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

#endif 
