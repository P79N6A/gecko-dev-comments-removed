





































#ifndef mozilla_dom_workers_eventtarget_h__
#define mozilla_dom_workers_eventtarget_h__

#include "jspubtd.h"

#include "ListenerManager.h"

BEGIN_WORKERS_NAMESPACE

namespace events {



class EventTarget : public PrivatizableBase
{
  ListenerManager mListenerManager;

protected:
  EventTarget();
  ~EventTarget();

  void
  TraceInstance(JSTracer* aTrc)
  {
    mListenerManager.Trace(aTrc);
  }

  void
  FinalizeInstance(JSContext* aCx)
  {
    mListenerManager.Finalize(aCx);
  }

  bool
  GetEventListenerOnEventTarget(JSContext* aCx, const char* aType, jsval* aVp);

  bool
  SetEventListenerOnEventTarget(JSContext* aCx, const char* aType, jsval* aVp);

public:
  static EventTarget*
  FromJSObject(JSContext* aCx, JSObject* aObj);

  static JSBool
  AddEventListener(JSContext* aCx, uintN aArgc, jsval* aVp);

  static JSBool
  RemoveEventListener(JSContext* aCx, uintN aArgc, jsval* aVp);

  static JSBool
  DispatchEvent(JSContext* aCx, uintN aArgc, jsval* aVp);

  bool
  HasListeners()
  {
    return mListenerManager.HasListeners();
  }

  bool
  HasListenersForType(JSContext* aCx, JSString* aType)
  {
    return mListenerManager.HasListenersForType(aCx, aType);
  }
};

JSObject*
InitEventTargetClass(JSContext* aCx, JSObject* aGlobal, bool aMainRuntime);

} 

END_WORKERS_NAMESPACE

#endif 
