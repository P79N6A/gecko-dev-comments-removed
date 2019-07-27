




#include "nsXBLSerialize.h"

#include "jsfriendapi.h"
#include "js/OldDebugAPI.h"
#include "nsXBLPrototypeBinding.h"
#include "nsIXPConnect.h"
#include "nsContentUtils.h"

using namespace mozilla;

nsresult
XBL_SerializeFunction(nsIObjectOutputStream* aStream,
                      JS::Handle<JSObject*> aFunction)
{
  AssertInCompilationScope();
  AutoJSContext cx;
  MOZ_ASSERT(js::GetContextCompartment(cx) == js::GetObjectCompartment(aFunction));
  return nsContentUtils::XPConnect()->WriteFunction(aStream, cx, aFunction);
}

nsresult
XBL_DeserializeFunction(nsIObjectInputStream* aStream,
                        JS::MutableHandle<JSObject*> aFunctionObjectp)
{
  AssertInCompilationScope();
  AutoJSContext cx;
  return nsContentUtils::XPConnect()->ReadFunction(aStream, cx,
                                                   aFunctionObjectp.address());
}
