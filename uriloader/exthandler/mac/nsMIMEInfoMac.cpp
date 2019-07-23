








































#include <LaunchServices.h>

#include "nsMIMEInfoMac.h"
#include "nsILocalFileMac.h"
#include "nsIFileURL.h"
#include "nsIInternetConfigService.h"

NS_IMETHODIMP
nsMIMEInfoMac::LaunchWithURI(nsIURI* aURI)
{
  nsCOMPtr<nsIFile> application;
  nsresult rv;
  
  if (mPreferredAction == useHelperApp) {

    
    nsCOMPtr<nsIWebHandlerApp> webHandlerApp =
      do_QueryInterface(mPreferredApplication, &rv);
    if (NS_SUCCEEDED(rv)) {
      return LaunchWithWebHandler(webHandlerApp, aURI);         
    }

    
    nsCOMPtr<nsILocalHandlerApp> localHandlerApp =
        do_QueryInterface(mPreferredApplication, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = localHandlerApp->GetExecutable(getter_AddRefs(application));
    NS_ENSURE_SUCCESS(rv, rv);
    
  } else if (mPreferredAction == useSystemDefault) {

    
    
    
    
    if (mClass == eProtocolInfo) {
      return LoadUriInternal(aURI);      
    }

    application = mDefaultApplication;
  }
  else
    return NS_ERROR_INVALID_ARG;


  
  nsCOMPtr<nsILocalFile> docToLoad;
  rv = GetLocalFileFromURI(aURI, getter_AddRefs(docToLoad));
  if (NS_FAILED(rv)) {

    
    NS_ASSERTION(mClass == eProtocolInfo, 
                 "nsMIMEInfoMac should be a protocol handler");

    
    nsCAutoString spec;
    aURI->GetSpec(spec);
    return OpenApplicationWithURI(application, spec);
  }

  
  
  
  
  nsCOMPtr<nsILocalFileMac> app;
  if (application) {
    app = do_QueryInterface(application, &rv);
    if (NS_FAILED(rv)) return rv;
  } else {
    
    nsCOMPtr<nsILocalFileMac> tempFile = do_QueryInterface(docToLoad, &rv);
    if (NS_FAILED(rv)) return rv;

    FSRef tempFileRef;
    tempFile->GetFSRef(&tempFileRef);

    FSRef appFSRef;
    if (::LSGetApplicationForItem(&tempFileRef, kLSRolesAll, &appFSRef, nsnull) == noErr)
    {
      app = (do_CreateInstance("@mozilla.org/file/local;1"));
      if (!app) return NS_ERROR_FAILURE;
      app->InitWithFSRef(&appFSRef);
    } else {
      return NS_ERROR_FAILURE;
    }
  }
  
  return app->LaunchWithDoc(docToLoad, PR_FALSE); 
}

nsresult 
nsMIMEInfoMac::LoadUriInternal(nsIURI *aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  nsresult rv = NS_ERROR_FAILURE;
  
  nsCAutoString uri;
  aURI->GetSpec(uri);
  if (!uri.IsEmpty()) {
    nsCOMPtr<nsIInternetConfigService> icService = 
      do_GetService(NS_INTERNETCONFIGSERVICE_CONTRACTID);
    if (icService)
      rv = icService->LaunchURL(uri.get());
  }
  return rv;
}

NS_IMETHODIMP
nsMIMEInfoMac::GetHasDefaultHandler(PRBool *_retval)
{
  
  *_retval = !mDefaultAppDescription.IsEmpty();
  return NS_OK;
}






nsresult
nsMIMEInfoMac::OpenApplicationWithURI(nsIFile* aApplication, 
                                      const nsCString& aURI)
{
  nsresult rv;
  nsCOMPtr<nsILocalFileMac> lfm(do_QueryInterface(aApplication, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  
  CFURLRef appURL;
  rv = lfm->GetCFURL(&appURL);
  if (NS_FAILED(rv))
    return rv;
  
  const UInt8* uriString = (const UInt8*)aURI.get();
  CFURLRef uri = ::CFURLCreateWithBytes(NULL, uriString, aURI.Length(),
                                        kCFStringEncodingUTF8, NULL);
  if (!uri) {
    ::CFRelease(appURL);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  CFArrayRef uris = ::CFArrayCreate(NULL, (const void**)&uri, 1, NULL);
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
