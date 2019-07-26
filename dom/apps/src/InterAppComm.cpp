



#include "InterAppComm.h"
#include "nsContentUtils.h"
#include "nsPIDOMWindow.h"
#include "nsJSPrincipals.h"
#include "mozilla/Preferences.h"
#include "AccessCheck.h"

using namespace mozilla::dom;

 bool
InterAppComm::EnabledForScope(JSContext* , JS::Handle<JSObject*> aObj)
{
  
  if (!Preferences::GetBool("dom.inter-app-communication-api.enabled", false)) {
  	return false;
  }

  
  
  return xpc::AccessCheck::isChrome(aObj);
}
