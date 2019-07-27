





#ifndef mozilla_JSObjectHolder_h
#define mozilla_JSObjectHolder_h

#include "js/RootingAPI.h"
#include "nsISupportsImpl.h"

namespace mozilla {











class JSObjectHolder final : public nsISupports
{
public:
  JSObjectHolder(JSContext* aCx, JSObject* aObject) : mJSObject(aCx, aObject) {}

  NS_DECL_ISUPPORTS

  JSObject* GetJSObject() { return mJSObject; }

private:
  ~JSObjectHolder() {}

  JS::PersistentRooted<JSObject*> mJSObject;
};

} 

#endif 
