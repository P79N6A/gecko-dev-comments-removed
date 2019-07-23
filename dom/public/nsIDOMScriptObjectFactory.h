




































#ifndef nsIDOMScriptObjectFactory_h__
#define nsIDOMScriptObjectFactory_h__

#include "nsISupports.h"
#include "nsIDOMClassInfo.h"
#include "nsStringGlue.h"

#define NS_IDOM_SCRIPT_OBJECT_FACTORY_IID   \
  { /* {38EC7717-6CBE-44a8-B2BB-53F2BA998B31} */ \
  0x38ec7717, 0x6cbe, 0x44a8, \
  { 0xb2, 0xbb, 0x53, 0xf2, 0xba, 0x99, 0x8b, 0x31 } }

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
