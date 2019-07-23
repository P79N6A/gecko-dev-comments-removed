







































#include <LaunchServices.h>

#include "nsMIMEInfoMac.h"
#include "nsILocalFileMac.h"

NS_IMETHODIMP
nsMIMEInfoMac::LaunchWithFile(nsIFile* aFile)
{
  nsIFile* application;

  if (mPreferredAction == useHelperApp)
    application = mPreferredApplication;
  else if (mPreferredAction == useSystemDefault)
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
