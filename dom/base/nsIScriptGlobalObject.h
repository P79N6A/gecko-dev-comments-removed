





#ifndef nsIScriptGlobalObject_h__
#define nsIScriptGlobalObject_h__

#include "nsISupports.h"
#include "nsIGlobalObject.h"
#include "js/TypeDecls.h"
#include "mozilla/EventForwards.h"

class nsIScriptContext;
class nsIScriptGlobalObject;

namespace mozilla {
namespace dom {
class ErrorEventInit;
} 
} 






bool
NS_HandleScriptError(nsIScriptGlobalObject *aScriptGlobal,
                     const mozilla::dom::ErrorEventInit &aErrorEvent,
                     nsEventStatus *aStatus);


#define NS_ISCRIPTGLOBALOBJECT_IID \
{ 0x6995e1ff, 0x9fc5, 0x44a7, \
 { 0xbd, 0x7c, 0xe7, 0xcd, 0x44, 0x47, 0x22, 0x87 } }








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
                     const mozilla::dom::ErrorEventInit &aErrorEventInit,
                     nsEventStatus *aEventStatus) {
    NS_ENSURE_STATE(NS_HandleScriptError(this, aErrorEventInit, aEventStatus));
    return NS_OK;
  }

  virtual bool IsBlackForCC(bool aTracingNeeded = true) { return false; }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptGlobalObject,
                              NS_ISCRIPTGLOBALOBJECT_IID)

#endif
