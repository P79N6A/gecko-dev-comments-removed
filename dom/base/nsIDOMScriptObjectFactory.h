




































#ifndef nsIDOMScriptObjectFactory_h__
#define nsIDOMScriptObjectFactory_h__

#include "nsISupports.h"
#include "nsIDOMClassInfo.h"
#include "nsStringGlue.h"

#define NS_IDOM_SCRIPT_OBJECT_FACTORY_IID   \
{ 0x8c0eb687, 0xa859, 0x4a62, \
 { 0x99, 0x82, 0xea, 0xbf, 0x9e, 0xf5, 0x59, 0x5f } }

class nsIScriptContext;
class nsIScriptGlobalObject;
class nsIScriptRuntime;
class nsIDOMEventListener;

typedef nsXPCClassInfo* (*nsDOMClassInfoExternalConstructorFnc)
  (const char* aName);

class nsIDOMScriptObjectFactory : public nsISupports {
public:  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOM_SCRIPT_OBJECT_FACTORY_IID)

  
  
  
  
  
  NS_IMETHOD GetScriptRuntime(const nsAString &aLanguageName,
                              nsIScriptRuntime **aLanguage) = 0;

  
  NS_IMETHOD GetScriptRuntimeByID(PRUint32 aScriptTypeID, 
                                  nsIScriptRuntime **aLanguage) = 0;

  
  
  NS_IMETHOD GetIDForScriptType(const nsAString &aLanguageName,
                                PRUint32 *aScriptTypeID) = 0;

  NS_IMETHOD NewScriptGlobalObject(bool aIsChrome,
                                   bool aIsModalContentWindow,
                                   nsIScriptGlobalObject **aGlobal) = 0;

  NS_IMETHOD_(nsISupports *) GetClassInfoInstance(nsDOMClassInfoID aID) = 0;
  NS_IMETHOD_(nsISupports *) GetExternalClassInfoInstance(const nsAString& aName) = 0;

  
  
  
  
  
  NS_IMETHOD RegisterDOMClassInfo(const char *aName,
                                  nsDOMClassInfoExternalConstructorFnc aConstructorFptr,
                                  const nsIID *aProtoChainInterface,
                                  const nsIID **aInterfaces,
                                  PRUint32 aScriptableFlags,
                                  bool aHasClassInterface,
                                  const nsCID *aConstructorCID) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMScriptObjectFactory,
                              NS_IDOM_SCRIPT_OBJECT_FACTORY_IID)

#endif 
