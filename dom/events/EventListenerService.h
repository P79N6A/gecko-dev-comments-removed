




#ifndef mozilla_EventListenerService_h_
#define mozilla_EventListenerService_h_

#include "jsapi.h"
#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMEventListener.h"
#include "nsIEventListenerService.h"
#include "nsString.h"
#include "nsTObserverArray.h"
#include "nsDataHashtable.h"

class nsIMutableArray;

namespace mozilla {
namespace dom {
class EventTarget;
};

template<typename T>
class Maybe;

class EventListenerInfo final : public nsIEventListenerInfo
{
public:
  EventListenerInfo(const nsAString& aType,
                    already_AddRefed<nsIDOMEventListener> aListener,
                    bool aCapturing,
                    bool aAllowsUntrusted,
                    bool aInSystemEventGroup)
    : mType(aType)
    , mListener(aListener)
    , mCapturing(aCapturing)
    , mAllowsUntrusted(aAllowsUntrusted)
    , mInSystemEventGroup(aInSystemEventGroup)
  {
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(EventListenerInfo)
  NS_DECL_NSIEVENTLISTENERINFO

protected:
  virtual ~EventListenerInfo() {}

  bool GetJSVal(JSContext* aCx,
                Maybe<JSAutoCompartment>& aAc,
                JS::MutableHandle<JS::Value> aJSVal);

  nsString mType;
  
  nsRefPtr<nsIDOMEventListener> mListener;
  bool mCapturing;
  bool mAllowsUntrusted;
  bool mInSystemEventGroup;
};

class EventListenerService final : public nsIEventListenerService
{
  ~EventListenerService();
public:
  EventListenerService();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEVENTLISTENERSERVICE

  static void NotifyAboutMainThreadListenerChange(dom::EventTarget* aTarget)
  {
    if (sInstance) {
      sInstance->NotifyAboutMainThreadListenerChangeInternal(aTarget);
    }
  }

  void NotifyPendingChanges();
private:
  void NotifyAboutMainThreadListenerChangeInternal(dom::EventTarget* aTarget);
  nsTObserverArray<nsCOMPtr<nsIListenerChangeListener>> mChangeListeners;
  nsCOMPtr<nsIMutableArray> mPendingListenerChanges;
  nsDataHashtable<nsISupportsHashKey, bool> mPendingListenerChangesSet;

  static EventListenerService* sInstance;
};

} 

#endif 
