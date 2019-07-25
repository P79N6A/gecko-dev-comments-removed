





































#ifndef mozilla_dom_workers_xmlhttprequest_h__
#define mozilla_dom_workers_xmlhttprequest_h__

#include "Workers.h"

#include "jsapi.h"

BEGIN_WORKERS_NAMESPACE

namespace xhr {

bool
InitClasses(JSContext* aCx, JSObject* aGlobal, JSObject* aProto);

struct StateData
{
  jsval mResponseText;
  jsval mStatus;
  jsval mStatusText;
  jsval mReadyState;
  bool mResponseTextException;
  bool mStatusException;
  bool mStatusTextException;
  bool mReadyStateException;
};

bool
UpdateXHRState(JSContext* aCx, JSObject* aObj, const StateData& aNewState);

} 

END_WORKERS_NAMESPACE

#endif 
