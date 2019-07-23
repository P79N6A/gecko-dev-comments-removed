




































#include "nsAboutBlank.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsStringStream.h"
#include "nsNetUtil.h"

NS_IMPL_ISUPPORTS1(nsAboutBlank, nsIAboutModule)

static const char kBlankPage[] = "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">"
"<html><head><title></title></head><body></body></html>";

NS_IMETHODIMP
nsAboutBlank::NewChannel(nsIURI *aURI, nsIChannel **result)
{
    NS_ENSURE_ARG_POINTER(aURI);
    nsresult rv;
    nsIChannel* channel;

    nsCOMPtr<nsIInputStream> in;
    rv = NS_NewCStringInputStream(getter_AddRefs(in), NS_LITERAL_CSTRING(kBlankPage));
    if (NS_FAILED(rv)) return rv;

    rv = NS_NewInputStreamChannel(&channel, aURI, in,
                                  NS_LITERAL_CSTRING("text/html"),
                                  NS_LITERAL_CSTRING("utf-8"));
    if (NS_FAILED(rv)) return rv;

    *result = channel;
    return rv;
}

NS_IMETHODIMP
nsAboutBlank::GetURIFlags(nsIURI *aURI, PRUint32 *result)
{
    *result = nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT;
    return NS_OK;
}

NS_METHOD
nsAboutBlank::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsAboutBlank* about = new nsAboutBlank();
    if (about == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(about);
    nsresult rv = about->QueryInterface(aIID, aResult);
    NS_RELEASE(about);
    return rv;
}


