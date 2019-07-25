





































#ifndef nsIScriptGlobalObject_h__
#define nsIScriptGlobalObject_h__

#include "nsISupports.h"
#include "nsEvent.h"
#include "nsIProgrammingLanguage.h"

class nsIScriptContext;
class nsIDOMDocument;
class nsIDOMEvent;
class nsIScriptGlobalObjectOwner;
class nsIArray;
class nsScriptErrorEvent;
class nsIScriptGlobalObject;
struct JSObject; 














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
{ 0x8f19a761, 0x0717, 0x4b3f, \
  { 0x80, 0xc5, 0xed, 0x7e, 0x9c, 0xbc, 0x40, 0xb1 } }






class nsIScriptGlobalObject : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTGLOBALOBJECT_IID)

  








  virtual nsresult EnsureScriptEnvironment(PRUint32 aLangID) = 0;
  


  virtual nsIScriptContext *GetScriptContext(PRUint32 lang) = 0;
  
  virtual JSObject* GetGlobalJSObject() = 0;

  virtual nsIScriptContext *GetContext() {
        return GetScriptContext(nsIProgrammingLanguage::JAVASCRIPT);
  }

  




  virtual nsresult SetScriptContext(PRUint32 lang, nsIScriptContext *aContext) = 0;

  






  virtual void OnFinalize(JSObject* aObject) = 0;

  


  virtual void SetScriptsEnabled(bool aEnabled, bool aFireTimeouts) = 0;

  


  virtual nsresult HandleScriptError(nsScriptErrorEvent *aErrorEvent,
                                     nsEventStatus *aEventStatus) {
    return NS_HandleScriptError(this, aErrorEvent, aEventStatus);
  }

  virtual bool IsBlackForCC() { return false; }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptGlobalObject,
                              NS_ISCRIPTGLOBALOBJECT_IID)

#endif
