




































#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsXPIDLString.h"
#include "nsParserMsgUtils.h"
#include "nsNetCID.h"

static nsresult GetBundle(const char * aPropFileName, nsIStringBundle **aBundle)
{
  NS_ENSURE_ARG_POINTER(aPropFileName);
  NS_ENSURE_ARG_POINTER(aBundle);

  
  nsresult rv;
  
  nsCOMPtr<nsIStringBundleService> stringService = 
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv))
    rv = stringService->CreateBundle(aPropFileName, aBundle);
  
  return rv;
}

nsresult
nsParserMsgUtils::GetLocalizedStringByName(const char * aPropFileName, const char* aKey, nsString& oVal)
{
  oVal.Truncate();

  NS_ENSURE_ARG_POINTER(aKey);

  nsCOMPtr<nsIStringBundle> bundle;
  nsresult rv = GetBundle(aPropFileName,getter_AddRefs(bundle));
  if (NS_SUCCEEDED(rv) && bundle) {
    nsXPIDLString valUni;
    nsAutoString key; key.AssignWithConversion(aKey);
    rv = bundle->GetStringFromName(key.get(), getter_Copies(valUni));
    if (NS_SUCCEEDED(rv) && valUni) {
      oVal.Assign(valUni);
    }  
  }

  return rv;
}

nsresult
nsParserMsgUtils::GetLocalizedStringByID(const char * aPropFileName, PRUint32 aID, nsString& oVal)
{
  oVal.Truncate();

  nsCOMPtr<nsIStringBundle> bundle;
  nsresult rv = GetBundle(aPropFileName,getter_AddRefs(bundle));
  if (NS_SUCCEEDED(rv) && bundle) {
    nsXPIDLString valUni;
    rv = bundle->GetStringFromID(aID, getter_Copies(valUni));
    if (NS_SUCCEEDED(rv) && valUni) {
      oVal.Assign(valUni);
    }  
  }

  return rv;
}
