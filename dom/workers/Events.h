





































#ifndef mozilla_dom_workers_events_h__
#define mozilla_dom_workers_events_h__

#include "Workers.h"

#include "jspubtd.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"

class JSAutoStructuredCloneBuffer;

BEGIN_WORKERS_NAMESPACE

namespace events {

bool
InitClasses(JSContext* aCx, JSObject* aGlobal, bool aMainRuntime);

JSObject*
CreateGenericEvent(JSContext* aCx, JSString* aType, bool aBubbles,
                   bool aCancelable, bool aMainRuntime);

JSObject*
CreateMessageEvent(JSContext* aCx, JSAutoStructuredCloneBuffer& aData,
                   nsTArray<nsCOMPtr<nsISupports> >& aClonedObjects,
                   bool aMainRuntime);

JSObject*
CreateErrorEvent(JSContext* aCx, JSString* aMessage, JSString* aFilename,
                 uint32 aLineNumber, bool aMainRuntime);

JSObject*
CreateProgressEvent(JSContext* aCx, JSString* aType, bool aLengthComputable,
                    jsdouble aLoaded, jsdouble aTotal);

bool
IsSupportedEventClass(JSContext* aCx, JSObject* aEvent);

bool
SetEventTarget(JSContext* aCx, JSObject* aEvent, JSObject* aTarget);

bool
EventWasCanceled(JSContext* aCx, JSObject* aEvent);

bool
EventImmediatePropagationStopped(JSContext* aCx, JSObject* aEvent);

bool
DispatchEventToTarget(JSContext* aCx, JSObject* aTarget, JSObject* aEvent,
                      bool* aPreventDefaultCalled);

} 

END_WORKERS_NAMESPACE

#endif 
