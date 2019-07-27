





#ifndef BackstagePass_h__
#define BackstagePass_h__

#include "nsISupports.h"
#include "nsWeakReference.h"
#include "nsIGlobalObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIXPCScriptable.h"

#include "js/HeapAPI.h"

class XPCWrappedNative;

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

  virtual nsIPrincipal* GetPrincipal() override {
    return mPrincipal;
  }

  virtual JSObject* GetGlobalJSObject() override;

  void ForgetGlobalObject() {
    mWrapper = nullptr;
  }

  void SetGlobalObject(JSObject* global);

  explicit BackstagePass(nsIPrincipal* prin) :
    mPrincipal(prin)
  {
  }

private:
  virtual ~BackstagePass() { }

  nsCOMPtr<nsIPrincipal> mPrincipal;
  XPCWrappedNative* mWrapper;
};

nsresult
NS_NewBackstagePass(BackstagePass** ret);

#endif 
