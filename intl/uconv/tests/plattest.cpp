



#include "nsIPlatformCharset.h"
#include "nsILocaleService.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsLocaleCID.h"
#include "nsIComponentManager.h"
#include <stdio.h>

int
main(int argc, const char** argv)
{

    nsCOMPtr<nsIPlatformCharset> platform_charset = 
        do_CreateInstance(NS_PLATFORMCHARSET_CONTRACTID);
    if (!platform_charset) return -1;

    nsCOMPtr<nsILocaleService>      locale_service = 
        do_CreateInstance(NS_LOCALESERVICE_CONTRACTID);
    if (!locale_service) return -1;

    nsCOMPtr<nsILocale>             locale;
    nsAutoCString                   charset;
    nsAutoString                    category;

    nsresult rv = locale_service->GetSystemLocale(getter_AddRefs(locale));
    if (NS_FAILED(rv)) return -1;

    rv = locale->GetCategory(NS_LITERAL_STRING("NSILOCALE_MESSAGES"), category);
    if (NS_FAILED(rv)) return -1;

    rv = platform_charset->GetDefaultCharsetForLocale(category, charset);
    if (NS_FAILED(rv)) return -1;

    printf("DefaultCharset for %s is %s\n", NS_LossyConvertUTF16toASCII(category).get(), charset.get());

    category.AssignLiteral("en-US");
    rv = platform_charset->GetDefaultCharsetForLocale(category, charset);
    if (NS_FAILED(rv)) return -1;

    printf("DefaultCharset for %s is %s\n", NS_LossyConvertUTF16toASCII(category).get(), charset.get());

    return 0;
}
