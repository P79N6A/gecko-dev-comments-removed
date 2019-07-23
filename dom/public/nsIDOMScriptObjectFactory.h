




































#ifndef nsIDOMScriptObjectFactory_h__
#define nsIDOMScriptObjectFactory_h__

#include "nsISupports.h"
#include "nsIDOMClassInfo.h"
#include "nsStringGlue.h"

#define NS_IDOM_SCRIPT_OBJECT_FACTORY_IID   \
{ 0xd5a4f935, 0xe428, 0x47ec, \
  { 0x8f, 0x36, 0x44, 0x23, 0xfa, 0xa2, 0x21, 0x90 } }

class nsIScriptContext;
class nsIScriptGlobalObject;
class nsIScriptRuntime;
class nsIDOMEventListener;

class nsIDOMScriptObjectFactory : public nsISupports {
public:  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOM_SCRIPT_OBJECT_FACTORY_IID)

  
  
  
  
  
  NS_IMETHOD GetScriptRuntime(const nsAString &aLanguageName,
                              nsIScriptRuntime **aLanguage) = 0;

  
  NS_IMETHOD GetScriptRuntimeByID(PRUint32 aScriptTypeID, 
                                  nsIScriptRuntime **aLanguage) = 0;

  
  
  NS_IMETHOD GetIDForScriptType(const nsAString &aLanguageName,
                                PRUint32 *aScriptTypeID) = 0;

  NS_IMETHOD NewScriptGlobalObject(PRBool aIsChrome,
                                   PRBool aIsModalContentWindow,
                                   nsIScriptGlobalObject **aGlobal) = 0;

  NS_IMETHOD_(nsISupports *) GetClassInfoInstance(nsDOMClassInfoID aID) = 0;
  NS_IMETHOD_(nsISupports *) GetExternalClassInfoInstance(const nsAString& aName) = 0;

  
  
  
  
  
  NS_IMETHOD RegisterDOMClassInfo(const char *aName,
                                  nsDOMClassInfoExternalConstructorFnc aConstructorFptr,
                                  const nsIID *aProtoChainInterface,
                                  const nsIID **aInterfaces,
                                  PRUint32 aScriptableFlags,
                                  PRBool aHasClassInterface,
                                  const nsCID *aConstructorCID) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMScriptObjectFactory,
                              NS_IDOM_SCRIPT_OBJECT_FACTORY_IID)

#endif 
