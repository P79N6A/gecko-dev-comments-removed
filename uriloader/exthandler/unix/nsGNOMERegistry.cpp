





































#include "nsGNOMERegistry.h"
#include "prlink.h"
#include "prmem.h"
#include "nsString.h"
#include "nsIComponentManager.h"
#include "nsILocalFile.h"
#include "nsMIMEInfoUnix.h"
#include "nsAutoPtr.h"
#include "nsIGConfService.h"
#include "nsIGnomeVFSService.h"
#include "nsIGIOService.h"

#ifdef MOZ_WIDGET_GTK2
#include <glib.h>
#include <glib-object.h>

#ifdef MOZ_PLATFORM_HILDON
#include <libintl.h>
#endif
#endif

 PRBool
nsGNOMERegistry::HandlerExists(const char *aProtocolScheme)
{
  nsCOMPtr<nsIGConfService> gconf = do_GetService(NS_GCONFSERVICE_CONTRACTID);
  if (!gconf)
    return PR_FALSE;

  PRBool isEnabled;
  nsCAutoString handler;
  if (NS_FAILED(gconf->GetAppForProtocol(nsDependentCString(aProtocolScheme), &isEnabled, handler)))
    return PR_FALSE;

  return isEnabled;
}









 nsresult
nsGNOMERegistry::LoadURL(nsIURI *aURL)
{
  nsCOMPtr<nsIGIOService> giovfs = do_GetService(NS_GIOSERVICE_CONTRACTID);
  nsCOMPtr<nsIGnomeVFSService> gnomevfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);
  if (giovfs) {
    return giovfs->ShowURI(aURL);
  } else if (gnomevfs) {
    
    return gnomevfs->ShowURI(aURL);
  }
  return NS_ERROR_FAILURE;
}

 void
nsGNOMERegistry::GetAppDescForScheme(const nsACString& aScheme,
                                     nsAString& aDesc)
{
  nsCOMPtr<nsIGConfService> gconf = do_GetService(NS_GCONFSERVICE_CONTRACTID);
  if (!gconf)
    return;

  PRBool isEnabled;
  nsCAutoString app;
  if (NS_FAILED(gconf->GetAppForProtocol(aScheme, &isEnabled, app)))
    return;

  if (!app.IsEmpty()) {
    
    PRInt32 firstSpace = app.FindChar(' ');
    if (firstSpace != kNotFound) {
      app.Truncate(firstSpace);
      PRInt32 lastSlash = app.RFindChar('/');
      if (lastSlash != kNotFound) {
        app.Cut(0, lastSlash + 1);
      }
    }

    CopyUTF8toUTF16(app, aDesc);
  }
}


 already_AddRefed<nsMIMEInfoBase>
nsGNOMERegistry::GetFromExtension(const nsACString& aFileExt)
{
  NS_ASSERTION(aFileExt[0] != '.', "aFileExt shouldn't start with a dot");
  nsCAutoString mimeType;
  nsCOMPtr<nsIGnomeVFSService> gnomevfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);
  nsCOMPtr<nsIGIOService> giovfs = do_GetService(NS_GIOSERVICE_CONTRACTID);

  if (!gnomevfs && !giovfs)
    return nsnull;

  if (giovfs) {
    
    
    if (NS_FAILED(giovfs->GetMimeTypeFromExtension(aFileExt, mimeType)) ||
        mimeType.EqualsLiteral("application/octet-stream"))
      return nsnull;
  } else if (gnomevfs) {
    
    if (NS_FAILED(gnomevfs->GetMimeTypeFromExtension(aFileExt, mimeType)) ||
        mimeType.EqualsLiteral("application/octet-stream"))
      return nsnull;
    
  }


  return GetFromType(mimeType);
}

 already_AddRefed<nsMIMEInfoBase>
nsGNOMERegistry::GetFromType(const nsACString& aMIMEType)
{
  nsCOMPtr<nsIGIOService> giovfs = do_GetService(NS_GIOSERVICE_CONTRACTID);
  nsCOMPtr<nsIGnomeVFSService> gnomevfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);
  nsCOMPtr<nsIGIOMimeApp> gioHandlerApp;
  nsCOMPtr<nsIGnomeVFSMimeApp> gnomeHandlerApp;
  
  if (!giovfs && !gnomevfs)
    return nsnull;

  if (giovfs) {
    if (NS_FAILED(giovfs->GetAppForMimeType(aMIMEType, getter_AddRefs(gioHandlerApp))) ||
        !gioHandlerApp)
      return nsnull;

  } else {
    
    if (NS_FAILED(gnomevfs->GetAppForMimeType(aMIMEType, getter_AddRefs(gnomeHandlerApp))) ||
        !gnomeHandlerApp)
      return nsnull;
    
  }
  nsRefPtr<nsMIMEInfoUnix> mimeInfo = new nsMIMEInfoUnix(aMIMEType);
  NS_ENSURE_TRUE(mimeInfo, nsnull);

  nsCAutoString description;
  if (giovfs)
    giovfs->GetDescriptionForMimeType(aMIMEType, description);
  else
    gnomevfs->GetDescriptionForMimeType(aMIMEType, description);

  mimeInfo->SetDescription(NS_ConvertUTF8toUTF16(description));

  nsCAutoString name;
  if (giovfs)
    gioHandlerApp->GetName(name);
  else 
    gnomeHandlerApp->GetName(name);

#ifdef MOZ_PLATFORM_HILDON
  
  
  
  
  
  const char kDefaultTextDomain [] = "maemo-af-desktop";
  nsCAutoString realName (dgettext(kDefaultTextDomain, PromiseFlatCString(name).get()));
  mimeInfo->SetDefaultDescription(NS_ConvertUTF8toUTF16(realName));
#else
  mimeInfo->SetDefaultDescription(NS_ConvertUTF8toUTF16(name));
#endif
  mimeInfo->SetPreferredAction(nsIMIMEInfo::useSystemDefault);

  nsMIMEInfoBase* retval;
  NS_ADDREF((retval = mimeInfo));
  return retval;
}
