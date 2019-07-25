





































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






PRBool
NS_HandleScriptError(nsIScriptGlobalObject *aScriptGlobal,
                     nsScriptErrorEvent *aErrorEvent,
                     nsEventStatus *aStatus);


#define NS_ISCRIPTGLOBALOBJECT_IID \
{ 0x4eb16819, 0x4e81, 0x406e, \
  { 0x93, 0x05, 0x6f, 0x30, 0xfc, 0xd2, 0x62, 0x4a } }






class nsIScriptGlobalObject : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTGLOBALOBJECT_IID)

  








  virtual nsresult EnsureScriptEnvironment(PRUint32 aLangID) = 0;
  


  virtual nsIScriptContext *GetScriptContext(PRUint32 lang) = 0;
  
  


  virtual void *GetScriptGlobal(PRUint32 lang) = 0;

  
  virtual JSObject *GetGlobalJSObject() {
        return (JSObject *)GetScriptGlobal(nsIProgrammingLanguage::JAVASCRIPT);
  }

  virtual nsIScriptContext *GetContext() {
        return GetScriptContext(nsIProgrammingLanguage::JAVASCRIPT);
  }

  




  virtual nsresult SetScriptContext(PRUint32 lang, nsIScriptContext *aContext) = 0;

  






  virtual void OnFinalize(JSObject* aObject) = 0;

  


  virtual void SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts) = 0;

  


  virtual nsresult HandleScriptError(nsScriptErrorEvent *aErrorEvent,
                                     nsEventStatus *aEventStatus) {
    return NS_HandleScriptError(this, aErrorEvent, aEventStatus);
  }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptGlobalObject,
                              NS_ISCRIPTGLOBALOBJECT_IID)

#endif
