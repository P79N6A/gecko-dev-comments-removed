





#include "nsIGlobalObject.h"
#include "nsContentUtils.h"

nsIPrincipal*
nsIGlobalObject::PrincipalOrNull()
{
  JSObject *global = GetGlobalJSObject();
  if (NS_WARN_IF(!global))
    return nullptr;

  return nsContentUtils::ObjectPrincipal(global);
}
