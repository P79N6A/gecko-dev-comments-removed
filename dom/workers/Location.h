




#ifndef mozilla_dom_workers_location_h__
#define mozilla_dom_workers_location_h__

#include "Workers.h"

#include "jspubtd.h"

BEGIN_WORKERS_NAMESPACE

namespace location {

bool
InitClass(JSContext* aCx, JSObject* aGlobal);

JSObject*
Create(JSContext* aCx, JS::Handle<JSString*> aHref, JS::Handle<JSString*> aProtocol,
       JS::Handle<JSString*> aHost, JS::Handle<JSString*> aHostname,
       JS::Handle<JSString*> aPort, JS::Handle<JSString*> aPathname,
       JS::Handle<JSString*> aSearch, JS::Handle<JSString*> aHash);

} 

END_WORKERS_NAMESPACE

#endif 
