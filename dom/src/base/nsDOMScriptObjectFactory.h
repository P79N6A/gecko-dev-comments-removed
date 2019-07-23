



















































#include "nsIDOMScriptObjectFactory.h"
#include "nsIObserver.h"
#include "nsIExceptionService.h"
#include "nsIScriptRuntime.h"
#include "nsIScriptGlobalObject.h" 

class nsDOMScriptObjectFactory : public nsIDOMScriptObjectFactory,
                                 public nsIObserver,
                                 public nsIExceptionProvider
{
public:
  nsDOMScriptObjectFactory();

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOBSERVER

  
  NS_DECL_NSIEXCEPTIONPROVIDER

  
  NS_IMETHOD GetScriptRuntime(const nsAString &aLanguageName,
                              nsIScriptRuntime **aLanguage);

  NS_IMETHOD GetScriptRuntimeByID(PRUint32 aLanguageID, 
                                  nsIScriptRuntime **aLanguage);

  NS_IMETHOD GetIDForScriptType(const nsAString &aLanguageName,
                                PRUint32 *aLanguageID);

  NS_IMETHOD NewScriptGlobalObject(PRBool aIsChrome,
                                   nsIScriptGlobalObject **aGlobal);

  NS_IMETHOD_(nsISupports *) GetClassInfoInstance(nsDOMClassInfoID aID);
  NS_IMETHOD_(nsISupports *) GetExternalClassInfoInstance(const nsAString& aName);

  NS_IMETHOD RegisterDOMClassInfo(const char *aName,
                                  nsDOMClassInfoExternalConstructorFnc aConstructorFptr,
                                  const nsIID *aProtoChainInterface,
                                  const nsIID **aInterfaces,
                                  PRUint32 aScriptableFlags,
                                  PRBool aHasClassInterface,
                                  const nsCID *aConstructorCID);

  static nsresult Startup();

protected:
  PRBool mLoadedAllLanguages;
  nsCOMPtr<nsIScriptRuntime> mLanguageArray[NS_STID_ARRAY_UBOUND];
};
