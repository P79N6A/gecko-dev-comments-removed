




#ifndef mozilla_dom_workers_events_h__
#define mozilla_dom_workers_events_h__

#include "Workers.h"

#include "js/StructuredClone.h"

BEGIN_WORKERS_NAMESPACE

namespace events {

bool
InitClasses(JSContext* aCx, JS::Handle<JSObject*> aGlobal, bool aMainRuntime);

JSObject*
CreateGenericEvent(JSContext* aCx, JS::Handle<JSString*> aType, bool aBubbles,
                   bool aCancelable, bool aMainRuntime);

JSObject*
CreateMessageEvent(JSContext* aCx, JSAutoStructuredCloneBuffer& aData,
                   nsTArray<nsCOMPtr<nsISupports> >& aClonedObjects,
                   bool aMainRuntime);

JSObject*
CreateErrorEvent(JSContext* aCx, JS::Handle<JSString*> aMessage,
                 JS::Handle<JSString*> aFilename,
                 uint32_t aLineNumber, bool aMainRuntime);

JSObject*
CreateProgressEvent(JSContext* aCx, JS::Handle<JSString*> aType,
                    bool aLengthComputable, double aLoaded, double aTotal);

bool
IsSupportedEventClass(JSObject* aEvent);

void
SetEventTarget(JSObject* aEvent, JSObject* aTarget);

bool
EventWasCanceled(JSObject* aEvent);

bool
EventImmediatePropagationStopped(JSObject* aEvent);

bool
DispatchEventToTarget(JSContext* aCx, JS::Handle<JSObject*> aTarget,
                      JS::Handle<JSObject*> aEvent, bool* aPreventDefaultCalled);

} 

END_WORKERS_NAMESPACE

#endif 
