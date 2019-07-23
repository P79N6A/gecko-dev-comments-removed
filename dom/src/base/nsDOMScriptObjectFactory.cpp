




















































#include "nsDOMScriptObjectFactory.h"
#include "xpcexception.h"
#include "nsScriptNameSpaceManager.h"
#include "nsIObserverService.h"
#include "nsJSEnvironment.h"
#include "nsJSEventListener.h"
#include "nsGlobalWindow.h"
#include "nsIJSContextStack.h"
#include "nsISupportsPrimitives.h"
#include "nsDOMException.h"
#include "nsCRT.h"
#ifdef MOZ_XUL
#include "nsXULPrototypeCache.h"
#endif

static NS_DEFINE_CID(kDOMScriptObjectFactoryCID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);

nsDOMScriptObjectFactory::nsDOMScriptObjectFactory() :
  mLoadedAllLanguages(PR_FALSE)
{
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1");

  if (observerService) {
    observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  }

  nsCOMPtr<nsIExceptionService> xs =
    do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID);

  if (xs) {
    xs->RegisterExceptionProvider(this, NS_ERROR_MODULE_DOM);
    xs->RegisterExceptionProvider(this, NS_ERROR_MODULE_DOM_RANGE);
#ifdef MOZ_SVG
    xs->RegisterExceptionProvider(this, NS_ERROR_MODULE_SVG);
#endif
    xs->RegisterExceptionProvider(this, NS_ERROR_MODULE_DOM_XPATH);
    xs->RegisterExceptionProvider(this, NS_ERROR_MODULE_XPCONNECT);
  }
  
  NS_CreateJSRuntime(getter_AddRefs(mLanguageArray[NS_STID_INDEX(nsIProgrammingLanguage::JAVASCRIPT)]));
}

NS_INTERFACE_MAP_BEGIN(nsDOMScriptObjectFactory)
  NS_INTERFACE_MAP_ENTRY(nsIDOMScriptObjectFactory)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsIExceptionProvider)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMScriptObjectFactory)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsDOMScriptObjectFactory)
NS_IMPL_RELEASE(nsDOMScriptObjectFactory)






















NS_IMETHODIMP
nsDOMScriptObjectFactory::GetScriptRuntime(const nsAString &aLanguageName,
                                           nsIScriptRuntime **aLanguage)
{
  
  
  
  
  
  
  nsCAutoString contractid(NS_LITERAL_CSTRING(
                          "@mozilla.org/script-language;1?script-type="));
  
  AppendUTF16toUTF8(aLanguageName, contractid);
  nsresult rv;
  nsCOMPtr<nsIScriptRuntime> lang =
        do_GetService(contractid.get(), &rv);

  if (NS_FAILED(rv)) {
    if (aLanguageName.Equals(NS_LITERAL_STRING("application/javascript")))
      return GetScriptRuntimeByID(nsIProgrammingLanguage::JAVASCRIPT, aLanguage);
    
    NS_WARNING("No script language registered for this mime-type");
    return NS_ERROR_FACTORY_NOT_REGISTERED;
  }
  
  PRUint32 lang_ndx = NS_STID_INDEX(lang->GetScriptTypeID());
  if (mLanguageArray[lang_ndx] == nsnull) {
    mLanguageArray[lang_ndx] = lang;
  } else {
    
    NS_ASSERTION(mLanguageArray[lang_ndx] == lang,
                 "Got a different language for this ID???");
  }
  *aLanguage = lang;
  NS_IF_ADDREF(*aLanguage);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::GetScriptRuntimeByID(PRUint32 aLanguageID, 
                                               nsIScriptRuntime **aLanguage)
{
  if (!NS_STID_VALID(aLanguageID)) {
    NS_WARNING("Unknown script language");
    return NS_ERROR_UNEXPECTED;
  }
  *aLanguage = mLanguageArray[NS_STID_INDEX(aLanguageID)];
  if (!*aLanguage) {
    nsCAutoString contractid(NS_LITERAL_CSTRING(
                        "@mozilla.org/script-language;1?id="));
    char langIdStr[25]; 
    sprintf(langIdStr, "%d", aLanguageID);
    contractid += langIdStr;
    nsresult rv;
    nsCOMPtr<nsIScriptRuntime> lang = do_GetService(contractid.get(), &rv);

    if (NS_FAILED(rv)) {
      NS_ERROR("Failed to get the script language");
      return rv;
    }

    
    mLanguageArray[NS_STID_INDEX(aLanguageID)] = lang;
    *aLanguage = lang;
  }
  NS_IF_ADDREF(*aLanguage);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::GetIDForScriptType(const nsAString &aLanguageName,
                                             PRUint32 *aScriptTypeID)
{
  nsCOMPtr<nsIScriptRuntime> languageRuntime;
  nsresult rv;
  rv = GetScriptRuntime(aLanguageName, getter_AddRefs(languageRuntime));
  if (NS_FAILED(rv))
    return rv;

  *aScriptTypeID = languageRuntime->GetScriptTypeID();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::NewScriptGlobalObject(PRBool aIsChrome,
                                                nsIScriptGlobalObject **aGlobal)
{
  return NS_NewScriptGlobalObject(aIsChrome, aGlobal);
}

NS_IMETHODIMP_(nsISupports *)
nsDOMScriptObjectFactory::GetClassInfoInstance(nsDOMClassInfoID aID)
{
  return NS_GetDOMClassInfoInstance(aID);
}

NS_IMETHODIMP_(nsISupports *)
nsDOMScriptObjectFactory::GetExternalClassInfoInstance(const nsAString& aName)
{
  extern nsScriptNameSpaceManager *gNameSpaceManager;

  NS_ENSURE_TRUE(gNameSpaceManager, nsnull);

  const nsGlobalNameStruct *globalStruct;
  gNameSpaceManager->LookupName(aName, &globalStruct);
  if (globalStruct) {
    if (globalStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfoCreator) {
      nsresult rv;
      nsCOMPtr<nsIDOMCIExtension> creator(do_CreateInstance(globalStruct->mCID, &rv));
      NS_ENSURE_SUCCESS(rv, nsnull);

      rv = creator->RegisterDOMCI(NS_ConvertUTF16toUTF8(aName).get(), this);
      NS_ENSURE_SUCCESS(rv, nsnull);

      rv = gNameSpaceManager->LookupName(aName, &globalStruct);
      NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && globalStruct, nsnull);

      NS_ASSERTION(globalStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfo,
                   "The classinfo data for this class didn't get registered.");
    }
    if (globalStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
      return nsDOMClassInfo::GetClassInfoInstance(globalStruct->mData);
    }
  }
  return nsnull;
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::Observe(nsISupports *aSubject,
                                  const char *aTopic,
                                  const PRUnichar *someData)
{
  if (!nsCRT::strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
#ifdef MOZ_XUL
    
    
    nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();

    if (cache)
      cache->Flush();
#endif

    nsCOMPtr<nsIThreadJSContextStack> stack =
      do_GetService("@mozilla.org/js/xpc/ContextStack;1");

    if (stack) {
      JSContext *cx = nsnull;

      stack->GetSafeJSContext(&cx);

      if (cx) {
        

        ::JS_GC(cx);
      }
    }

    nsGlobalWindow::ShutDown();
    nsDOMClassInfo::ShutDown();

    PRUint32 i;
    NS_STID_FOR_INDEX(i) {
      if (mLanguageArray[i] != nsnull) {
        mLanguageArray[i]->ShutDown();
        mLanguageArray[i] = nsnull;
      }
    }

    nsCOMPtr<nsIExceptionService> xs =
      do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID);

    if (xs) {
      xs->UnregisterExceptionProvider(this, NS_ERROR_MODULE_DOM);
      xs->UnregisterExceptionProvider(this, NS_ERROR_MODULE_DOM_RANGE);
#ifdef MOZ_SVG
      xs->UnregisterExceptionProvider(this, NS_ERROR_MODULE_SVG);
#endif
      xs->UnregisterExceptionProvider(this, NS_ERROR_MODULE_DOM_XPATH);
      xs->UnregisterExceptionProvider(this, NS_ERROR_MODULE_XPCONNECT);
    }
  }

  return NS_OK;
}

static nsresult
CreateXPConnectException(nsresult aResult, nsIException *aDefaultException,
                         nsIException **_retval)
{
  
  
  nsCOMPtr<nsIXPCException> exception(do_QueryInterface(aDefaultException));
  if (!exception) {
    nsresult rv = NS_OK;
    exception = do_CreateInstance("@mozilla.org/js/xpc/Exception;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = exception->Initialize(nsnull, aResult, nsnull, nsnull, nsnull,
                               nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*_retval = exception);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::GetException(nsresult result,
				       nsIException *aDefaultException,
				       nsIException **_retval)
{
  switch (NS_ERROR_GET_MODULE(result))
  {
    case NS_ERROR_MODULE_DOM_RANGE:
      return NS_NewRangeException(result, aDefaultException, _retval);
#ifdef MOZ_SVG
    case NS_ERROR_MODULE_SVG:
      return NS_NewSVGException(result, aDefaultException, _retval);
#endif
    case NS_ERROR_MODULE_DOM_XPATH:
      return NS_NewXPathException(result, aDefaultException, _retval);
    case NS_ERROR_MODULE_XPCONNECT:
      return CreateXPConnectException(result, aDefaultException, _retval);
    case NS_ERROR_MODULE_DOM_FILE:
      return NS_NewFileException(result, aDefaultException, _retval);
    default:
      return NS_NewDOMException(result, aDefaultException, _retval);
  }
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::RegisterDOMClassInfo(const char *aName,
					       nsDOMClassInfoExternalConstructorFnc aConstructorFptr,
					       const nsIID *aProtoChainInterface,
					       const nsIID **aInterfaces,
					       PRUint32 aScriptableFlags,
					       PRBool aHasClassInterface,
					       const nsCID *aConstructorCID)
{
  extern nsScriptNameSpaceManager *gNameSpaceManager;

  NS_ENSURE_TRUE(gNameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  return gNameSpaceManager->RegisterDOMCIData(aName,
                                              aConstructorFptr,
                                              aProtoChainInterface,
                                              aInterfaces,
                                              aScriptableFlags,
                                              aHasClassInterface,
                                              aConstructorCID);
}

 nsresult
nsDOMScriptObjectFactory::Startup()
{
  nsJSRuntime::Startup();
  
  
  
  return NS_OK;
}


nsresult NS_GetScriptRuntime(const nsAString &aLanguageName,
                             nsIScriptRuntime **aLanguage)
{
  nsresult rv;
  *aLanguage = nsnull;
  nsCOMPtr<nsIDOMScriptObjectFactory> factory = \
        do_GetService(kDOMScriptObjectFactoryCID, &rv);
  if (NS_FAILED(rv))
    return rv;
  return factory->GetScriptRuntime(aLanguageName, aLanguage);
}

nsresult NS_GetScriptRuntimeByID(PRUint32 aScriptTypeID,
                                 nsIScriptRuntime **aLanguage)
{
  nsresult rv;
  *aLanguage = nsnull;
  nsCOMPtr<nsIDOMScriptObjectFactory> factory = \
        do_GetService(kDOMScriptObjectFactoryCID, &rv);
  if (NS_FAILED(rv))
    return rv;
  return factory->GetScriptRuntimeByID(aScriptTypeID, aLanguage);
}
