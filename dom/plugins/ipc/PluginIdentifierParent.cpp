






































#include "PluginIdentifierParent.h"

#include "nsServiceManagerUtils.h"
#include "nsNPAPIPlugin.h"
#include "nsIJSContextStack.h"
#include "PluginScriptableObjectUtils.h"
#include "mozilla/unused.h"

using namespace mozilla::plugins::parent;

namespace mozilla {
namespace plugins {

bool
PluginIdentifierParent::RecvRetain()
{
  mTemporaryRefs = 0;

  
  jsid id = NPIdentifierToJSId(mIdentifier);
  if (JSID_IS_INT(id)) {
    return true;
  }

  
  
  nsCOMPtr<nsIThreadJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1");
  if (!stack) {
    return false;
  }

  JSContext *cx = nsnull;
  stack->GetSafeJSContext(&cx);
  if (!cx) {
    return false;
  }

  JSAutoRequest ar(cx);
  JSString* str = JSID_TO_STRING(id);
  JSString* str2 = JS_InternJSString(cx, str);
  if (!str2) {
    return false;
  }

  NS_ASSERTION(str == str2, "Interning a string in a JSID should always return the same string.");

  return true;
}

PluginIdentifierParent::StackIdentifier::StackIdentifier
    (PluginInstanceParent* inst, NPIdentifier aIdentifier)
  : mIdentifier(inst->Module()->GetIdentifierForNPIdentifier(inst->GetNPP(), aIdentifier))
{
}

PluginIdentifierParent::StackIdentifier::StackIdentifier
    (NPObject* aObject, NPIdentifier aIdentifier)
  : mIdentifier(NULL)
{
  PluginInstanceParent* inst = GetInstance(aObject);
  mIdentifier = inst->Module()->GetIdentifierForNPIdentifier(inst->GetNPP(), aIdentifier);
}

PluginIdentifierParent::StackIdentifier::~StackIdentifier()
{
  if (!mIdentifier) {
    return;
  }

  if (!mIdentifier->IsTemporary()) {
    return;
  }

  if (mIdentifier->RemoveTemporaryRef()) {
    unused << PPluginIdentifierParent::Send__delete__(mIdentifier);
  }
}

} 
} 
