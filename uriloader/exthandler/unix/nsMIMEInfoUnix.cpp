






































#include "nsMIMEInfoUnix.h"
#include "nsGNOMERegistry.h"
#include "nsIGnomeVFSService.h"

nsresult
nsMIMEInfoUnix::LoadUriInternal(nsIURI * aURI)
{
  return nsGNOMERegistry::LoadURL(aURI);
}

NS_IMETHODIMP
nsMIMEInfoUnix::GetHasDefaultHandler(PRBool *_retval)
{
  *_retval = PR_FALSE;
  nsCOMPtr<nsIGnomeVFSService> vfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);
  if (vfs) {
    nsCOMPtr<nsIGnomeVFSMimeApp> app;
    if (NS_SUCCEEDED(vfs->GetAppForMimeType(mType, getter_AddRefs(app))) && app)
      *_retval = PR_TRUE;
  }

  if (*_retval)
    return NS_OK;

  
  return nsMIMEInfoImpl::GetHasDefaultHandler(_retval);
}

nsresult
nsMIMEInfoUnix::LaunchDefaultWithFile(nsIFile *aFile)
{
  nsCAutoString nativePath;
  aFile->GetNativePath(nativePath);

  nsCOMPtr<nsIGnomeVFSService> vfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);

  if (vfs) {
    nsCOMPtr<nsIGnomeVFSMimeApp> app;
    if (NS_SUCCEEDED(vfs->GetAppForMimeType(mType, getter_AddRefs(app))) && app)
      return app->Launch(nativePath);
  }

  if (!mDefaultApplication)
    return NS_ERROR_FILE_NOT_FOUND;

  return LaunchWithIProcess(mDefaultApplication, nativePath);
}
