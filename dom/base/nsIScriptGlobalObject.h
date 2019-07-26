





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
{ 0x30c64680, 0x909a, 0x4435, \
  { 0x90, 0x3b, 0x29, 0x3e, 0xb5, 0x5d, 0xc7, 0xa0 } }








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

  


  virtual nsresult HandleScriptError(
                     mozilla::InternalScriptErrorEvent *aErrorEvent,
                     nsEventStatus *aEventStatus) {
    NS_ENSURE_STATE(NS_HandleScriptError(this, aErrorEvent, aEventStatus));
    return NS_OK;
  }

  virtual bool IsBlackForCC(bool aTracingNeeded = true) { return false; }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptGlobalObject,
                              NS_ISCRIPTGLOBALOBJECT_IID)

#endif
