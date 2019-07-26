





#ifndef nsIScriptGlobalObject_h__
#define nsIScriptGlobalObject_h__

#include "nsISupports.h"
#include "nsEvent.h"
#include "nsIProgrammingLanguage.h"
#include "nsIGlobalObject.h"

class nsIScriptContext;
class nsIDOMEvent;
class nsIScriptGlobalObjectOwner;
class nsIArray;
class nsScriptErrorEvent;
class nsIScriptGlobalObject;
class JSObject; 







#define NS_STID_FIRST nsIProgrammingLanguage::JAVASCRIPT

#define NS_STID_LAST (nsIProgrammingLanguage::MAX > 0x000FU ? \
                      0x000FU : nsIProgrammingLanguage::MAX)


#define NS_STID_ARRAY_UBOUND (NS_STID_LAST-NS_STID_FIRST+1)


#define NS_STID_VALID(langID) (langID >= NS_STID_FIRST && langID <= NS_STID_LAST)


#define NS_STID_INDEX(langID) (langID-NS_STID_FIRST)


#define NS_STID_FOR_ID(varName) \
          for (varName=NS_STID_FIRST;varName<=NS_STID_LAST;varName++)



#define NS_STID_FOR_INDEX(varName) \
          for (varName=0;varName<=NS_STID_INDEX(NS_STID_LAST);varName++)






bool
NS_HandleScriptError(nsIScriptGlobalObject *aScriptGlobal,
                     nsScriptErrorEvent *aErrorEvent,
                     nsEventStatus *aStatus);


#define NS_ISCRIPTGLOBALOBJECT_IID \
{ 0xde24b30a, 0x12c6, 0x4e5f, \
  { 0xa8, 0x5e, 0x90, 0xcd, 0xfb, 0x6c, 0x54, 0x51 } }








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

  


  virtual nsresult HandleScriptError(nsScriptErrorEvent *aErrorEvent,
                                     nsEventStatus *aEventStatus) {
    NS_ENSURE_STATE(NS_HandleScriptError(this, aErrorEvent, aEventStatus));
    return NS_OK;
  }

  virtual bool IsBlackForCC() { return false; }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptGlobalObject,
                              NS_ISCRIPTGLOBALOBJECT_IID)

#endif
