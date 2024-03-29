





#include "WakeLock.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/Event.h" 
#include "mozilla/dom/MozWakeLockBinding.h"
#include "mozilla/Hal.h"
#include "mozilla/HalWakeLock.h"
#include "nsError.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "nsIDOMEvent.h"
#include "nsPIDOMWindow.h"
#include "nsIPropertyBag2.h"

using namespace mozilla::hal;

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WakeLock)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WakeLock)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(WakeLock)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WakeLock)

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

JSObject*
WakeLock::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return MozWakeLockBinding::Wrap(aCx, this, aGivenProto);
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
    nsCOMPtr<nsIDocument> doc = window->GetExtantDoc();
    NS_ENSURE_STATE(doc);
    mHidden = doc->Hidden();
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
WakeLock::Observe(nsISupports* aSubject, const char* aTopic, const char16_t* data)
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
    nsCOMPtr<nsIDocument> doc = window->GetExtantDoc();
    if (doc) {
      doc->AddSystemEventListener(NS_LITERAL_STRING("visibilitychange"),
                                  this,
                                   true,
                                   false);

      nsCOMPtr<EventTarget> target = do_QueryInterface(window);
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
    nsCOMPtr<nsIDocument> doc = window->GetExtantDoc();
    if (doc) {
      doc->RemoveSystemEventListener(NS_LITERAL_STRING("visibilitychange"),
                                     this,
                                      true);
      nsCOMPtr<EventTarget> target = do_QueryInterface(window);
      target->RemoveSystemEventListener(NS_LITERAL_STRING("pagehide"),
                                        this,
                                         true);
      target->RemoveSystemEventListener(NS_LITERAL_STRING("pageshow"),
                                        this,
                                         true);
    }
  }
}

void
WakeLock::Unlock(ErrorResult& aRv)
{
  


  if (!mLocked) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  DoUnlock();
  DetachEventListener();
}

void
WakeLock::GetTopic(nsAString &aTopic)
{
  aTopic.Assign(mTopic);
}

NS_IMETHODIMP
WakeLock::HandleEvent(nsIDOMEvent *aEvent)
{
  nsAutoString type;
  aEvent->GetType(type);

  if (type.EqualsLiteral("visibilitychange")) {
    nsCOMPtr<nsIDocument> doc =
      do_QueryInterface(aEvent->InternalDOMEvent()->GetTarget());
    NS_ENSURE_STATE(doc);

    bool oldHidden = mHidden;
    mHidden = doc->Hidden();

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

nsISupports*
WakeLock::GetParentObject() const
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(mWindow);
  return window;
}

} 
} 
