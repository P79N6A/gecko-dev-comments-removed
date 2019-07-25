



































#include "nsIURI.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIAboutModule.h"

inline nsresult
NS_GetAboutModuleName(nsIURI *aAboutURI, nsCString& aModule)
{
#ifdef DEBUG
    {
        PRBool isAbout;
        NS_ASSERTION(NS_SUCCEEDED(aAboutURI->SchemeIs("about", &isAbout)) &&
                     isAbout,
                     "should be used only on about: URIs");
    }
#endif

    nsresult rv = aAboutURI->GetPath(aModule);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 f = aModule.FindCharInSet(NS_LITERAL_CSTRING("#?"));
    if (f != kNotFound) {
        aModule.Truncate(f);
    }

    
    ToLowerCase(aModule);
    return NS_OK;
}

inline nsresult
NS_GetAboutModule(nsIURI *aAboutURI, nsIAboutModule** aModule)
{
  NS_PRECONDITION(aAboutURI, "Must have URI");

  nsCAutoString contractID;
  nsresult rv = NS_GetAboutModuleName(aAboutURI, contractID);
  if (NS_FAILED(rv)) return rv;

  
  contractID.Insert(NS_LITERAL_CSTRING(NS_ABOUT_MODULE_CONTRACTID_PREFIX), 0);

  return CallGetService(contractID.get(), aModule);
}
