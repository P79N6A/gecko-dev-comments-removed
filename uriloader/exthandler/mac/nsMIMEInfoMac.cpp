







































#include <LaunchServices.h>

#include "nsMIMEInfoMac.h"
#include "nsILocalFileMac.h"

NS_IMETHODIMP
nsMIMEInfoMac::LaunchWithFile(nsIFile* aFile)
{
  nsCOMPtr<nsIFile> application;

  if (mPreferredAction == useHelperApp) {
    nsresult rv;
    nsCOMPtr<nsILocalHandlerApp> localHandlerApp =
      do_QueryInterface(mPreferredApplication, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
          
    rv = localHandlerApp->GetExecutable(getter_AddRefs(application));
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (mPreferredAction == useSystemDefault)
    application = mDefaultApplication;
  else
    return NS_ERROR_INVALID_ARG;

  if (application) {
    nsresult rv;
    nsCOMPtr<nsILocalFileMac> app = do_QueryInterface(application, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsILocalFile> docToLoad = do_QueryInterface(aFile, &rv);
    if (NS_FAILED(rv)) return rv;

    return app->LaunchWithDoc(docToLoad, PR_FALSE); 
  }
#ifdef XP_MACOSX
  
  nsresult rv;
  nsCOMPtr <nsILocalFileMac> tempFile = do_QueryInterface(aFile, &rv);
  if (NS_FAILED(rv)) return rv;
  
  FSRef tempFileRef;
  tempFile->GetFSRef(&tempFileRef);

  FSRef appFSRef;
  if (::LSGetApplicationForItem(&tempFileRef, kLSRolesAll, &appFSRef, nsnull) == noErr)
  {
    nsCOMPtr<nsILocalFileMac> app(do_CreateInstance("@mozilla.org/file/local;1"));
    if (!app) return NS_ERROR_FAILURE;
    app->InitWithFSRef(&appFSRef);
    
    nsCOMPtr <nsILocalFile> docToLoad = do_QueryInterface(aFile, &rv);
    if (NS_FAILED(rv)) return rv;
    
    rv = app->LaunchWithDoc(docToLoad, PR_FALSE); 
  }
  return rv;
#endif
}
