




#ifndef mozilla_dom_workers_listenermanager_h__
#define mozilla_dom_workers_listenermanager_h__

#include "mozilla/dom/workers/Workers.h"

#include "prclist.h"

BEGIN_WORKERS_NAMESPACE

class EventTarget;



class EventListenerManager
{
  PRCList mCollectionHead;

public:
  EventListenerManager()
  {
    PR_INIT_CLIST(&mCollectionHead);
  }

#ifdef DEBUG
  ~EventListenerManager();
#endif

  void
  _Trace(JSTracer* aTrc) const
  {
    if (!PR_CLIST_IS_EMPTY(&mCollectionHead)) {
      TraceInternal(aTrc);
    }
  }

  void
  _Finalize(JSContext* aCx)
  {
    if (!PR_CLIST_IS_EMPTY(&mCollectionHead)) {
      FinalizeInternal(aCx);
    }
  }

  enum Phase
  {
    All = 0,
    Capturing,
    Onfoo,
    Bubbling
  };

  void
  AddEventListener(JSContext* aCx, const jsid& aType, JSObject* aListener,
                   bool aCapturing, bool aWantsUntrusted, nsresult& aRv)
  {
    Add(aCx, aType, aListener, aCapturing ? Capturing : Bubbling,
        aWantsUntrusted, aRv);
  }

  void
  RemoveEventListener(JSContext* aCx, const jsid& aType, JSObject* aListener,
                      bool aCapturing)
  {
    if (PR_CLIST_IS_EMPTY(&mCollectionHead)) {
      return;
    }
    Remove(aCx, aType, aListener, aCapturing ? Capturing : Bubbling, true);
  }

  bool
  DispatchEvent(JSContext* aCx, const EventTarget& aTarget, JSObject* aEvent,
                nsresult& aRv) const;

  JSObject*
  GetEventListener(const jsid& aType) const;

  void
  SetEventListener(JSContext* aCx, const jsid& aType, JSObject* aListener,
                   nsresult& aRv)
  {
    JSObject* existing = GetEventListener(aType);
    if (existing) {
      Remove(aCx, aType, existing, Onfoo, false);
    }

    if (aListener) {
      Add(aCx, aType, aListener, Onfoo, false, aRv);
    }
  }

  bool
  HasListeners() const
  {
    return !PR_CLIST_IS_EMPTY(&mCollectionHead);
  }

  bool
  HasListenersForType(JSContext* aCx, const jsid& aType) const
  {
    return HasListeners() && HasListenersForTypeInternal(aCx, aType);
  }

private:
  void
  TraceInternal(JSTracer* aTrc) const;

  void
  FinalizeInternal(JSContext* aCx);

  void
  Add(JSContext* aCx, const jsid& aType, JSObject* aListener, Phase aPhase,
      bool aWantsUntrusted, nsresult& aRv);

  void
  Remove(JSContext* aCx, const jsid& aType, JSObject* aListener, Phase aPhase,
         bool aClearEmpty);

  bool
  HasListenersForTypeInternal(JSContext* aCx, const jsid& aType) const;
};

END_WORKERS_NAMESPACE

#endif 
