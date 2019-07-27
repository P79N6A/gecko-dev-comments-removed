





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
#include "nsGkAtoms.h"

class nsIMutableArray;

namespace mozilla {
namespace dom {
class EventTarget;
} 

template<typename T>
class Maybe;

class EventListenerChange final : public nsIEventListenerChange
{
public:
  explicit EventListenerChange(dom::EventTarget* aTarget);

  void AddChangedListenerName(nsIAtom* aEventName);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIEVENTLISTENERCHANGE

protected:
  virtual ~EventListenerChange();
  nsCOMPtr<dom::EventTarget> mTarget;
  nsCOMPtr<nsIMutableArray> mChangedListenerNames;

};

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

  static void NotifyAboutMainThreadListenerChange(dom::EventTarget* aTarget,
                                                  nsIAtom* aName)
  {
    if (sInstance) {
      sInstance->NotifyAboutMainThreadListenerChangeInternal(aTarget, aName);
    }
  }

  void NotifyPendingChanges();
private:
  void NotifyAboutMainThreadListenerChangeInternal(dom::EventTarget* aTarget,
                                                   nsIAtom* aName);
  nsTObserverArray<nsCOMPtr<nsIListenerChangeListener>> mChangeListeners;
  nsCOMPtr<nsIMutableArray> mPendingListenerChanges;
  nsDataHashtable<nsISupportsHashKey, nsRefPtr<EventListenerChange>> mPendingListenerChangesSet;

  static EventListenerService* sInstance;
};

} 

#endif 
