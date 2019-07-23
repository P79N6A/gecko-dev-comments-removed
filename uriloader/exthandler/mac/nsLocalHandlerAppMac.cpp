




































#include <LaunchServices.h>

#include "nsLocalHandlerAppMac.h"
#include "nsILocalFileMac.h"
#include "nsIURI.h"






NS_IMETHODIMP
nsLocalHandlerAppMac::LaunchWithURI(nsIURI *aURI,
                                    nsIInterfaceRequestor *aWindowContext)
{
  nsresult rv;
  nsCOMPtr<nsILocalFileMac> lfm(do_QueryInterface(mExecutable, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  
  CFURLRef appURL;
  rv = lfm->GetCFURL(&appURL);
  if (NS_FAILED(rv))
    return rv;
  
  nsCAutoString uriSpec;
  aURI->GetSpec(uriSpec);
  
  const UInt8* uriString = reinterpret_cast<const UInt8*>(uriSpec.get());
  CFURLRef uri = ::CFURLCreateWithBytes(NULL, uriString, uriSpec.Length(),
                                        kCFStringEncodingUTF8, NULL);
  if (!uri) {
    ::CFRelease(appURL);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  CFArrayRef uris = ::CFArrayCreate(NULL, reinterpret_cast<const void**>(&uri),
                                    1, NULL);
  if (!uris) {
    ::CFRelease(uri);
    ::CFRelease(appURL);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  LSLaunchURLSpec launchSpec;
  launchSpec.appURL = appURL;
  launchSpec.itemURLs = uris;
  launchSpec.passThruParams = NULL;
  launchSpec.launchFlags = kLSLaunchDefaults;
  launchSpec.asyncRefCon = NULL;
  
  OSErr err = ::LSOpenFromURLSpec(&launchSpec, NULL);
  
  ::CFRelease(uris);
  ::CFRelease(uri);
  ::CFRelease(appURL);
  
  return err != noErr ? NS_ERROR_FAILURE : NS_OK;
}
