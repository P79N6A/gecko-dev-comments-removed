




#ifndef mozilla_EventListenerService_h_
#define mozilla_EventListenerService_h_

#include "jsapi.h"
#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMEventListener.h"
#include "nsIEventListenerService.h"
#include "nsString.h"

namespace mozilla {

template<typename T>
class Maybe;

class EventListenerInfo MOZ_FINAL : public nsIEventListenerInfo
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

  virtual ~EventListenerInfo() {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(EventListenerInfo)
  NS_DECL_NSIEVENTLISTENERINFO

protected:
  bool GetJSVal(JSContext* aCx,
                Maybe<JSAutoCompartment>& aAc,
                JS::MutableHandle<JS::Value> aJSVal);

  nsString mType;
  
  nsRefPtr<nsIDOMEventListener> mListener;
  bool mCapturing;
  bool mAllowsUntrusted;
  bool mInSystemEventGroup;
};

class EventListenerService MOZ_FINAL : public nsIEventListenerService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEVENTLISTENERSERVICE
};

} 

#endif
