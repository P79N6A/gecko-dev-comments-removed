











#ifndef mozilla_dom_BindingDeclarations_h__
#define mozilla_dom_BindingDeclarations_h__

#include "nsStringGlue.h"
#include "jsapi.h"
#include "mozilla/Util.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {

struct MainThreadDictionaryBase
{
protected:
  JSContext* ParseJSON(const nsAString& aJSON,
                       mozilla::Maybe<JSAutoRequest>& aAr,
                       mozilla::Maybe<JSAutoCompartment>& aAc,
                       JS::Value& aVal);
};

struct EnumEntry {
  const char* value;
  size_t length;
};

class NS_STACK_CLASS GlobalObject
{
public:
  GlobalObject(JSContext* aCx, JSObject* aObject);

  nsISupports* Get() const
  {
    return mGlobalObject;
  }

  bool Failed() const
  {
    return !Get();
  }

private:
  js::RootedObject mGlobalJSObject;
  nsISupports* mGlobalObject;
  nsCOMPtr<nsISupports> mGlobalObjectRef;
};

class NS_STACK_CLASS WorkerGlobalObject
{
public:
  WorkerGlobalObject(JSContext* aCx, JSObject* aObject);

  JSObject* Get() const
  {
    return mGlobalJSObject;
  }
  
  
  
  JSContext* GetContext() const
  {
    return mCx;
  }

  bool Failed() const
  {
    return !Get();
  }

private:
  js::RootedObject mGlobalJSObject;
  JSContext* mCx;
};

} 
} 

#endif 
