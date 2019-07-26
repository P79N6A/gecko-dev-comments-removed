





#include "PluginIdentifierParent.h"

#include "nsContentUtils.h"
#include "nsNPAPIPlugin.h"
#include "nsServiceManagerUtils.h"
#include "PluginScriptableObjectUtils.h"
#include "nsCxPusher.h"
#include "mozilla/unused.h"

using namespace mozilla::plugins::parent;

namespace mozilla {
namespace plugins {

bool
PluginIdentifierParent::RecvRetain()
{
  mTemporaryRefs = 0;

  
  AutoSafeJSContext cx;
  JS::Rooted<jsid> id(cx, NPIdentifierToJSId(mIdentifier));
  if (JSID_IS_INT(id)) {
    return true;
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
