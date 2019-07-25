





































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
  jsval mResponse;
  bool mResponseTextException;
  bool mStatusException;
  bool mStatusTextException;
  bool mReadyStateException;
  bool mResponseException;
};

bool
UpdateXHRState(JSContext* aCx, JSObject* aObj, bool aIsUpload,
               const StateData& aNewState);

} 

bool
ClassIsXMLHttpRequest(JSClass* aClass);

END_WORKERS_NAMESPACE

#endif 
