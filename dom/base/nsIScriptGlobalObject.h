





#ifndef nsIScriptGlobalObject_h__
#define nsIScriptGlobalObject_h__

#include "nsISupports.h"
#include "nsIGlobalObject.h"
#include "js/TypeDecls.h"
#include "mozilla/EventForwards.h"

class nsIScriptContext;
class nsIScriptGlobalObject;






bool
NS_HandleScriptError(nsIScriptGlobalObject *aScriptGlobal,
                     mozilla::InternalScriptErrorEvent *aErrorEvent,
                     nsEventStatus *aStatus);


#define NS_ISCRIPTGLOBALOBJECT_IID \
{ 0x214fa2f6, 0xcc0c, 0x42cf, \
  { 0x98, 0x4b, 0x45, 0xf5, 0x73, 0x9c, 0x6b, 0x73 } }








class nsIScriptGlobalObject : public nsIGlobalObject
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTGLOBALOBJECT_IID)

  








  virtual nsresult EnsureScriptEnvironment() = 0;
  


  virtual nsIScriptContext *GetScriptContext() = 0;

  nsIScriptContext* GetContext() {
    return GetScriptContext();
  }

  






  virtual void OnFinalize(JSObject* aObject) = 0;

  


  virtual void SetScriptsEnabled(bool aEnabled, bool aFireTimeouts) = 0;

  


  virtual nsresult HandleScriptError(
                     mozilla::InternalScriptErrorEvent *aErrorEvent,
                     nsEventStatus *aEventStatus) {
    NS_ENSURE_STATE(NS_HandleScriptError(this, aErrorEvent, aEventStatus));
    return NS_OK;
  }

  virtual bool IsBlackForCC() { return false; }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptGlobalObject,
                              NS_ISCRIPTGLOBALOBJECT_IID)

#endif
