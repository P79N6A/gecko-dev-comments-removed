








































#include <LaunchServices.h>

#include "nsMIMEInfoMac.h"
#include "nsILocalFileMac.h"
#include "nsIFileURL.h"

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
    
  } else if (mPreferredAction == useSystemDefault)
    application = mDefaultApplication;
  else
    return NS_ERROR_INVALID_ARG;

  
  nsCOMPtr<nsILocalFile> docToLoad;
  rv = GetLocalFileFromURI(aURI, getter_AddRefs(docToLoad));
  if (NS_FAILED(rv)) return rv;

  
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
