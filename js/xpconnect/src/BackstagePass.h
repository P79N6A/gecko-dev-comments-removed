




#ifndef BackstagePass_h__
#define BackstagePass_h__

#include "nsISupports.h"
#include "nsWeakReference.h"
#include "nsIGlobalObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIXPCScriptable.h"

class BackstagePass : public nsIGlobalObject,
                      public nsIScriptObjectPrincipal,
                      public nsIXPCScriptable,
                      public nsIClassInfo,
                      public nsSupportsWeakReference
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE
  NS_DECL_NSICLASSINFO

  virtual nsIPrincipal* GetPrincipal() {
    return mPrincipal;
  }

  virtual JSObject* GetGlobalJSObject() {
    return mGlobal;
  }

  virtual void ForgetGlobalObject() {
    mGlobal = NULL;
  }

  virtual void SetGlobalObject(JSObject* global) {
    mGlobal = global;
  }

  BackstagePass(nsIPrincipal *prin) :
    mPrincipal(prin)
  {
  }

  virtual ~BackstagePass() { }

private:
  nsCOMPtr<nsIPrincipal> mPrincipal;
  JSObject *mGlobal;
};

NS_EXPORT nsresult
NS_NewBackstagePass(BackstagePass** ret);

#endif 
