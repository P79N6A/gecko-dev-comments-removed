







#ifndef mozilla_dom_ScriptSettings_h
#define mozilla_dom_ScriptSettings_h

#include "nsCxPusher.h"
#include "MainThreadUtils.h"

#include "mozilla/Maybe.h"

class nsIGlobalObject;

namespace mozilla {
namespace dom {




class AutoEntryScript {
public:
  AutoEntryScript(nsIGlobalObject* aGlobalObject,
                  bool aIsMainThread = NS_IsMainThread(),
                  
                  JSContext* aCx = nullptr);

private:
  nsCxPusher mCxPusher;
  mozilla::Maybe<JSAutoCompartment> mAc; 
                                         
};




class AutoIncumbentScript {
public:
  AutoIncumbentScript(nsIGlobalObject* aGlobalObject);
};






class AutoSystemCaller {
public:
  AutoSystemCaller(bool aIsMainThread = NS_IsMainThread());
private:
  nsCxPusher mCxPusher;
};

} 
} 

#endif 
