









































#include <LaunchServices.h>

#include "nsMIMEInfoMac.h"
#include "nsILocalFileMac.h"
#include "nsIFileURL.h"
#include "nsIInternetConfigService.h"

NS_IMETHODIMP
nsMIMEInfoMac::LaunchWithFile(nsIFile *aFile)
{
  nsCOMPtr<nsIFile> application;
  nsresult rv;

  NS_ASSERTION(mClass == eMIMEInfo, "only MIME infos are currently allowed"
               "to pass content by value");
  
  if (mPreferredAction == useHelperApp) {

    
    
    nsCOMPtr<nsILocalHandlerApp> localHandlerApp =
        do_QueryInterface(mPreferredApplication, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = localHandlerApp->GetExecutable(getter_AddRefs(application));
    NS_ENSURE_SUCCESS(rv, rv);
    
  } else if (mPreferredAction == useSystemDefault) {
    application = mDefaultApplication;
  }
  else
    return NS_ERROR_INVALID_ARG;

  
  nsCOMPtr<nsILocalFileMac> app;
  if (application) {
    app = do_QueryInterface(application, &rv);
    if (NS_FAILED(rv)) return rv;
  } else {
    
    nsCOMPtr<nsILocalFileMac> tempFile = do_QueryInterface(aFile, &rv);
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
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(aFile);
  return app->LaunchWithDoc(localFile, PR_FALSE); 
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
