




#ifndef mozilla_dom_workers_events_h__
#define mozilla_dom_workers_events_h__

#include "Workers.h"

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
                 uint32_t aLineNumber, bool aMainRuntime);

JSObject*
CreateProgressEvent(JSContext* aCx, JSString* aType, bool aLengthComputable,
                    double aLoaded, double aTotal);

bool
IsSupportedEventClass(JSObject* aEvent);

void
SetEventTarget(JSObject* aEvent, JSObject* aTarget);

bool
EventWasCanceled(JSObject* aEvent);

bool
EventImmediatePropagationStopped(JSObject* aEvent);

bool
DispatchEventToTarget(JSContext* aCx, JSObject* aTarget, JSObject* aEvent,
                      bool* aPreventDefaultCalled);

} 

END_WORKERS_NAMESPACE

#endif 
