



































#include "nsSimpleChromeRegistry.h"
#include "nsNetUtil.h"
#include "nsLayoutCID.h"


static NS_DEFINE_CID(kCSSLoaderCID, NS_CSS_LOADER_CID);

NS_IMPL_ISUPPORTS1(nsSimpleChromeRegistry, nsIChromeRegistry)

nsSimpleChromeRegistry::nsSimpleChromeRegistry()
{
}

nsSimpleChromeRegistry::~nsSimpleChromeRegistry()
{
}

NS_IMETHODIMP 
nsSimpleChromeRegistry::ConvertChromeURL(nsIURI *aChromeURL, nsIURI* *aResult)
{
  NS_WARNING("Who is calling ConvertChromeURL?");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsSimpleChromeRegistry::CheckForNewChrome()
{
    return NS_OK;
}

