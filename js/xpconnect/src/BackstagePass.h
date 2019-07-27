





#ifndef BackstagePass_h__
#define BackstagePass_h__

#include "nsISupports.h"
#include "nsWeakReference.h"
#include "nsIGlobalObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIXPCScriptable.h"

#include "js/HeapAPI.h"

class BackstagePass : public nsIGlobalObject,
                      public nsIScriptObjectPrincipal,
                      public nsIXPCScriptable,
                      public nsIClassInfo,
                      public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE
  NS_DECL_NSICLASSINFO

  virtual nsIPrincipal* GetPrincipal() {
    return mPrincipal;
  }

  virtual JSObject* GetGlobalJSObject() {
    return mGlobal;
  }

  virtual void ForgetGlobalObject() {
    mGlobal = nullptr;
  }

  virtual void SetGlobalObject(JSObject* global) {
    mGlobal = global;
  }

  explicit BackstagePass(nsIPrincipal *prin) :
    mPrincipal(prin)
  {
  }

private:
  virtual ~BackstagePass() { }

  nsCOMPtr<nsIPrincipal> mPrincipal;
  JS::TenuredHeap<JSObject*> mGlobal;
};

nsresult
NS_NewBackstagePass(BackstagePass** ret);

#endif 
