





#include "mozilla/dom/ScriptSettings.h"

#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {

AutoEntryScript::AutoEntryScript(nsIGlobalObject* aGlobalObject,
                                 bool aIsMainThread,
                                 JSContext* aCx)
{
  MOZ_ASSERT(aGlobalObject);
  if (!aCx) {
    
    
    
    
    MOZ_ASSERT(aIsMainThread, "cx is mandatory off-main-thread");
    nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(aGlobalObject);
    if (sgo && sgo->GetScriptContext()) {
      aCx = sgo->GetScriptContext()->GetNativeContext();
    }
    if (!aCx) {
      aCx = nsContentUtils::GetSafeJSContext();
    }
  }
  if (aIsMainThread) {
    mCxPusher.Push(aCx);
  }
  mAc.construct(aCx, aGlobalObject->GetGlobalJSObject());
}

AutoIncumbentScript::AutoIncumbentScript(nsIGlobalObject* aGlobalObject)
{
  MOZ_ASSERT(aGlobalObject);
}

AutoSystemCaller::AutoSystemCaller(bool aIsMainThread)
{
  if (aIsMainThread) {
    mCxPusher.PushNull();
  }
}

} 
} 
