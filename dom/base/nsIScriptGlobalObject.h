





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
struct ErrorEventInit;
} 
} 






bool
NS_HandleScriptError(nsIScriptGlobalObject *aScriptGlobal,
                     const mozilla::dom::ErrorEventInit &aErrorEvent,
                     nsEventStatus *aStatus);


#define NS_ISCRIPTGLOBALOBJECT_IID \
{ 0x876f83bd, 0x6314, 0x460a, \
  { 0xa0, 0x45, 0x1c, 0x8f, 0x46, 0x2f, 0xb8, 0xe1 } }








class nsIScriptGlobalObject : public nsIGlobalObject
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTGLOBALOBJECT_IID)

  








  virtual nsresult EnsureScriptEnvironment() = 0;
  


  virtual nsIScriptContext *GetScriptContext() = 0;

  nsIScriptContext* GetContext() {
    return GetScriptContext();
  }

  


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
