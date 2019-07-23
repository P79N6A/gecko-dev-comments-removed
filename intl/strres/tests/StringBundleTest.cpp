




































#include "nsCOMPtr.h"
#include "nsIStringBundle.h"
#include <stdio.h>

#include "nsIURL.h"
#include "nsIServiceManager.h"
#include "nsIComponentRegistrar.h"
#include "nsNetCID.h"

#include "nsStringAPI.h"
#include "nsEmbedString.h"

#include "nsXPCOM.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"


#define TEST_URL "resource://gre/res/strres.properties"







#include "nsILocaleService.h"
#include "nsLocaleCID.h"




nsresult
getCountry(const nsAString &lc_name, nsAString &aCountry)
{
  const PRUnichar* data;
  PRUint32 length = NS_StringGetData(lc_name, &data);

  PRUint32 dash;
  for (dash = 0; dash < length; ++dash) {
    if (data[dash] == '-')
      break;
  }
  if (dash == length)
    return NS_ERROR_FAILURE;

  aCountry.Assign(lc_name);
  aCountry.Cut(dash, (lc_name.Length()-dash-1));

  return NS_OK;
}

nsresult
getUILangCountry(nsAString& aUILang, nsAString& aCountry)
{
	nsresult	 result;
	
	nsCOMPtr<nsILocaleService> localeService = do_GetService(NS_LOCALESERVICE_CONTRACTID, &result);
	NS_ASSERTION(NS_SUCCEEDED(result),"nsLocaleTest: get locale service failed");

  result = localeService->GetLocaleComponentForUserAgent(aUILang);
  NS_ASSERTION(NS_SUCCEEDED(result),"nsLocaleTest: get locale component failed");
  result = getCountry(aUILang, aCountry);
  return result;
}


int
main(int argc, char *argv[])
{
  nsresult ret;

  nsCOMPtr<nsIServiceManager> servMan;
  NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, nsnull);
  nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(servMan);
  NS_ASSERTION(registrar, "Null nsIComponentRegistrar");
  registrar->AutoRegister(nsnull);
  
  nsCOMPtr<nsIStringBundleService> service =
      do_GetService(NS_STRINGBUNDLE_CONTRACTID);
  if (!service) {
    printf("cannot create service\n");
    return 1;
  }

  nsAutoString uiLang;
  nsAutoString country;
  ret = getUILangCountry(uiLang, country);
#if DEBUG_tao
  printf("\n uiLang=%s, country=%s\n\n",
         NS_LossyConvertUTF16toASCII(uiLang).get(),
         NS_LossyConvertUTF16toASCII(country).get());
#endif

  nsIStringBundle* bundle = nsnull;

  ret = service->CreateBundle(TEST_URL, &bundle);

  if (NS_FAILED(ret)) {
    printf("cannot create instance\n");
    return 1;
  }

  PRUnichar *ptrv = nsnull;

  
  ret = bundle->GetStringFromID(123, &ptrv);
  if (NS_FAILED(ret)) {
    printf("cannot get string from ID 123, ret=%d\n", ret);
    return 1;
  }

  printf("123=\"%s\"\n", NS_ConvertUTF16toUTF8(ptrv).get());

  
  ret = bundle->GetStringFromName(NS_LITERAL_STRING("file").get(), &ptrv);
  if (NS_FAILED(ret)) {
    printf("cannot get string from name\n");
    return 1;
  }

  printf("file=\"%s\"\n", NS_ConvertUTF16toUTF8(ptrv).get());

  return 0;
}
