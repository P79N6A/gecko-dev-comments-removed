




#include "WakeLock.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/Hal.h"
#include "mozilla/HalWakeLock.h"
#include "nsDOMClassInfoID.h"
#include "nsError.h"
#include "nsIDOMWindow.h"
#include "nsIDOMEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsPIDOMWindow.h"
#include "PowerManager.h"

DOMCI_DATA(MozWakeLock, mozilla::dom::power::WakeLock)

using namespace mozilla::hal;

namespace mozilla {
namespace dom {
namespace power {

NS_INTERFACE_MAP_BEGIN(WakeLock)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozWakeLock)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMozWakeLock)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozWakeLock)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(WakeLock)
NS_IMPL_RELEASE(WakeLock)

WakeLock::WakeLock()
  : mLocked(false)
  , mHidden(true)
  , mContentParentID(CONTENT_PROCESS_ID_UNKNOWN)
{
}

WakeLock::~WakeLock()
{
  DoUnlock();
  DetachEventListener();
}

nsresult
WakeLock::Init(const nsAString &aTopic, nsIDOMWindow *aWindow)
{
  
  MOZ_ASSERT(mTopic.IsEmpty());

  if (aTopic.IsEmpty()) {
    return NS_ERROR_INVALID_ARG;
  }

  mTopic.Assign(aTopic);

  mWindow = do_GetWeakReference(aWindow);
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);

  



  if (window) {
    nsCOMPtr<nsIDOMDocument> domDoc = window->GetExtantDocument();
    NS_ENSURE_STATE(domDoc);
    domDoc->GetHidden(&mHidden);
  }

  AttachEventListener();
  DoLock();

  return NS_OK;
}

nsresult
WakeLock::Init(const nsAString& aTopic, ContentParent* aContentParent)
{
  
  MOZ_ASSERT(mTopic.IsEmpty());
  MOZ_ASSERT(aContentParent);

  if (aTopic.IsEmpty()) {
    return NS_ERROR_INVALID_ARG;
  }

  mTopic.Assign(aTopic);
  mContentParentID = aContentParent->ChildID();
  mHidden = false;

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (obs) {
    obs->AddObserver(this, "ipc:content-shutdown",  true);
  }

  DoLock();
  return NS_OK;
}

NS_IMETHODIMP
WakeLock::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* data)
{
  
  
  
  
  
  
  

  MOZ_ASSERT(!strcmp(aTopic, "ipc:content-shutdown"));

  nsCOMPtr<nsIPropertyBag2> props = do_QueryInterface(aSubject);
  if (!props) {
    NS_WARNING("ipc:content-shutdown message without property bag as subject");
    return NS_OK;
  }

  uint64_t childID = 0;
  nsresult rv = props->GetPropertyAsUint64(NS_LITERAL_STRING("childID"),
                                           &childID);
  if (NS_SUCCEEDED(rv)) {
    if (childID == mContentParentID) {
      mLocked = false;
    }
  } else {
    NS_WARNING("ipc:content-shutdown message without childID property");
  }
  return NS_OK;
}

void
WakeLock::DoLock()
{
  if (!mLocked) {
    
    mLocked = true;

    hal::ModifyWakeLock(mTopic,
                        hal::WAKE_LOCK_ADD_ONE,
                        mHidden ? hal::WAKE_LOCK_ADD_ONE : hal::WAKE_LOCK_NO_CHANGE,
                        mContentParentID);
  }
}

void
WakeLock::DoUnlock()
{
  if (mLocked) {
    
    mLocked = false;

    hal::ModifyWakeLock(mTopic,
                        hal::WAKE_LOCK_REMOVE_ONE,
                        mHidden ? hal::WAKE_LOCK_REMOVE_ONE : hal::WAKE_LOCK_NO_CHANGE,
                        mContentParentID);
  }
}

void
WakeLock::AttachEventListener()
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);
  
  if (window) {
    nsCOMPtr<nsIDOMDocument> domDoc = window->GetExtantDocument();
    if (domDoc) {
      nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(domDoc);
      target->AddSystemEventListener(NS_LITERAL_STRING("visibilitychange"),
                                     this,
                                      true,
                                      false);

      target = do_QueryInterface(window);
      target->AddSystemEventListener(NS_LITERAL_STRING("pagehide"),
                                     this,
                                      true,
                                      false);
      target->AddSystemEventListener(NS_LITERAL_STRING("pageshow"),
                                     this,
                                      true,
                                      false);
    }
  }
}

void
WakeLock::DetachEventListener()
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);

  if (window) {
    nsCOMPtr<nsIDOMDocument> domDoc = window->GetExtantDocument();
    if (domDoc) {
      nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(domDoc);
      target->RemoveSystemEventListener(NS_LITERAL_STRING("visibilitychange"),
                                        this,
                                         true);
      target = do_QueryInterface(window);
      target->RemoveSystemEventListener(NS_LITERAL_STRING("pagehide"),
                                        this,
                                         true);
      target->RemoveSystemEventListener(NS_LITERAL_STRING("pageshow"),
                                        this,
                                         true);
    }
  }
}

NS_IMETHODIMP
WakeLock::Unlock()
{
  


  if (!mLocked) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  DoUnlock();
  DetachEventListener();

  return NS_OK;
}

NS_IMETHODIMP
WakeLock::GetTopic(nsAString &aTopic)
{
  aTopic.Assign(mTopic);
  return NS_OK;
}

NS_IMETHODIMP
WakeLock::HandleEvent(nsIDOMEvent *aEvent)
{
  nsAutoString type;
  aEvent->GetType(type);

  if (type.EqualsLiteral("visibilitychange")) {
    nsCOMPtr<nsIDOMEventTarget> target;
    aEvent->GetTarget(getter_AddRefs(target));
    nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(target);
    NS_ENSURE_STATE(domDoc);
    bool oldHidden = mHidden;
    domDoc->GetHidden(&mHidden);

    if (mLocked && oldHidden != mHidden) {
      hal::ModifyWakeLock(mTopic,
                          hal::WAKE_LOCK_NO_CHANGE,
                          mHidden ? hal::WAKE_LOCK_ADD_ONE : hal::WAKE_LOCK_REMOVE_ONE,
                          mContentParentID);
    }

    return NS_OK;
  }

  if (type.EqualsLiteral("pagehide")) {
    DoUnlock();
    return NS_OK;
  }

  if (type.EqualsLiteral("pageshow")) {
    DoLock();
    return NS_OK;
  }

  return NS_OK;
}

} 
} 
} 
