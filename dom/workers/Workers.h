





































#ifndef mozilla_dom_workers_workers_h__
#define mozilla_dom_workers_workers_h__

#include "jspubtd.h"

#define BEGIN_WORKERS_NAMESPACE \
  namespace mozilla { namespace dom { namespace workers {
#define END_WORKERS_NAMESPACE \
  } /* namespace workers */ } /* namespace dom */ } /* namespace mozilla */
#define USING_WORKERS_NAMESPACE \
  using namespace mozilla::dom::workers;

class nsPIDOMWindow;

BEGIN_WORKERS_NAMESPACE

struct PrivatizableBase
{ };

#ifdef DEBUG
void
AssertIsOnMainThread();
#else
inline void
AssertIsOnMainThread()
{ }
#endif


JSBool
ResolveWorkerClasses(JSContext* aCx, JSObject* aObj, jsid aId, uintN aFlags,
                     JSObject** aObjp);

void
CancelWorkersForWindow(JSContext* aCx, nsPIDOMWindow* aWindow);

void
SuspendWorkersForWindow(JSContext* aCx, nsPIDOMWindow* aWindow);

void
ResumeWorkersForWindow(JSContext* aCx, nsPIDOMWindow* aWindow);

END_WORKERS_NAMESPACE

#endif 
