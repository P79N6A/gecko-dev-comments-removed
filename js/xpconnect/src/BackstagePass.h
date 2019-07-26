




#ifndef BackstagePass_h__
#define BackstagePass_h__

#include "nsISupports.h"
#include "nsIGlobalObject.h"

class BackstagePass : public nsIGlobalObject,
                      public nsIXPCScriptable,
                      public nsIClassInfo
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
