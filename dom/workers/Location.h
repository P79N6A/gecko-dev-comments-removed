





































#ifndef mozilla_dom_workers_location_h__
#define mozilla_dom_workers_location_h__

#include "Workers.h"

#include "jspubtd.h"

BEGIN_WORKERS_NAMESPACE

namespace location {

bool
InitClass(JSContext* aCx, JSObject* aGlobal);

JSObject*
Create(JSContext* aCx, JSString* aHref, JSString* aProtocol, JSString* aHost,
       JSString* aHostname, JSString* aPort, JSString* aPathname,
       JSString* aSearch, JSString* aHash);

} 

END_WORKERS_NAMESPACE

#endif 
