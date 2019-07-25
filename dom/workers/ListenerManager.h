





































#ifndef mozilla_dom_workers_listenermanager_h__
#define mozilla_dom_workers_listenermanager_h__

#include "Workers.h"

#include "jsapi.h"
#include "prclist.h"

BEGIN_WORKERS_NAMESPACE

namespace events {



class ListenerManager
{
public:
  enum Phase
  {
    All = 0,
    Capturing,
    Onfoo,
    Bubbling
  };

private:
  PRCList mCollectionHead;

public:
  ListenerManager()
  {
    PR_INIT_CLIST(&mCollectionHead);
  }

#ifdef DEBUG
  ~ListenerManager();
#endif

  void
  Trace(JSTracer* aTrc)
  {
    if (!PR_CLIST_IS_EMPTY(&mCollectionHead)) {
      TraceInternal(aTrc);
    }
  }

  void
  Finalize(JSContext* aCx)
  {
    if (!PR_CLIST_IS_EMPTY(&mCollectionHead)) {
      FinalizeInternal(aCx);
    }
  }

  bool
  AddEventListener(JSContext* aCx, JSString* aType, jsval aListener,
                   bool aCapturing, bool aWantsUntrusted)
  {
    return Add(aCx, aType, aListener, aCapturing ? Capturing : Bubbling,
               aWantsUntrusted);
  }

  bool
  SetEventListener(JSContext* aCx, JSString* aType, jsval aListener);

  bool
  RemoveEventListener(JSContext* aCx, JSString* aType, jsval aListener,
                      bool aCapturing)
  {
    if (PR_CLIST_IS_EMPTY(&mCollectionHead)) {
      return true;
    }
    return Remove(aCx, aType, aListener, aCapturing ? Capturing : Bubbling,
                  true);
  }

  bool
  GetEventListener(JSContext* aCx, JSString* aType, jsval* aListener);

  bool
  DispatchEvent(JSContext* aCx, JSObject* aTarget, JSObject* aEvent,
                bool* aPreventDefaultCalled);

  bool
  HasListeners()
  {
    return !PR_CLIST_IS_EMPTY(&mCollectionHead);
  }

  bool
  HasListenersForType(JSContext* aCx, JSString* aType)
  {
    return HasListeners() && HasListenersForTypeInternal(aCx, aType);
  }

  bool
  HasListenersForTypeInternal(JSContext* aCx, JSString* aType);

private:
  void
  TraceInternal(JSTracer* aTrc);

  void
  FinalizeInternal(JSContext* aCx);

  bool
  Add(JSContext* aCx, JSString* aType, jsval aListener, Phase aPhase,
      bool aWantsUntrusted);

  bool
  Remove(JSContext* aCx, JSString* aType, jsval aListener, Phase aPhase,
         bool aClearEmpty);
};

} 

END_WORKERS_NAMESPACE

#endif
