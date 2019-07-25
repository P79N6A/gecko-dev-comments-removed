





































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
#endif

#ifdef MOZ_PLATFORM_MAEMO
#include <libintl.h>
#endif

 PRBool
nsGNOMERegistry::HandlerExists(const char *aProtocolScheme)
{
  nsCOMPtr<nsIGIOService> giovfs = do_GetService(NS_GIOSERVICE_CONTRACTID);
  nsCOMPtr<nsIGConfService> gconf = do_GetService(NS_GCONFSERVICE_CONTRACTID);
  if (giovfs) {
    nsCOMPtr<nsIGIOMimeApp> app;
    if (NS_FAILED(giovfs->GetAppForURIScheme(nsDependentCString(aProtocolScheme),
                                             getter_AddRefs(app))))
      return PR_FALSE;
    else
      return PR_TRUE;
  } else if (gconf) {
    PRBool isEnabled;
    nsCAutoString handler;
    if (NS_FAILED(gconf->GetAppForProtocol(nsDependentCString(aProtocolScheme), &isEnabled, handler)))
      return PR_FALSE;

    return isEnabled;
  }

  return PR_FALSE;
}









 nsresult
nsGNOMERegistry::LoadURL(nsIURI *aURL)
{
  nsCOMPtr<nsIGIOService> giovfs = do_GetService(NS_GIOSERVICE_CONTRACTID);
  if (giovfs)
    return giovfs->ShowURI(aURL);

  nsCOMPtr<nsIGnomeVFSService> gnomevfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);
  if (gnomevfs)
    return gnomevfs->ShowURI(aURL);

  return NS_ERROR_FAILURE;
}

 void
nsGNOMERegistry::GetAppDescForScheme(const nsACString& aScheme,
                                     nsAString& aDesc)
{
  nsCOMPtr<nsIGConfService> gconf = do_GetService(NS_GCONFSERVICE_CONTRACTID);
  nsCOMPtr<nsIGIOService> giovfs = do_GetService(NS_GIOSERVICE_CONTRACTID);
  if (!gconf && !giovfs)
    return;

  nsCAutoString name;
  if (giovfs) {
    nsCOMPtr<nsIGIOMimeApp> app;
    if (NS_FAILED(giovfs->GetAppForURIScheme(aScheme, getter_AddRefs(app))))
      return;

    app->GetName(name);
  } else {
    PRBool isEnabled;
    if (NS_FAILED(gconf->GetAppForProtocol(aScheme, &isEnabled, name)))
      return;

    if (!name.IsEmpty()) {
      
      PRInt32 firstSpace = name.FindChar(' ');
      if (firstSpace != kNotFound) {
        name.Truncate(firstSpace);
        PRInt32 lastSlash = name.RFindChar('/');
        if (lastSlash != kNotFound) {
          name.Cut(0, lastSlash + 1);
        }
      }
    }
  }

  CopyUTF8toUTF16(name, aDesc);
}


 already_AddRefed<nsMIMEInfoBase>
nsGNOMERegistry::GetFromExtension(const nsACString& aFileExt)
{
  nsCAutoString mimeType;
  nsCOMPtr<nsIGIOService> giovfs = do_GetService(NS_GIOSERVICE_CONTRACTID);

  if (giovfs) {
    
    
    if (NS_FAILED(giovfs->GetMimeTypeFromExtension(aFileExt, mimeType)) ||
        mimeType.EqualsLiteral("application/octet-stream")) {
      return nsnull;
    }
  } else {
    
    nsCOMPtr<nsIGnomeVFSService> gnomevfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);
    if (!gnomevfs)
      return nsnull;

    if (NS_FAILED(gnomevfs->GetMimeTypeFromExtension(aFileExt, mimeType)) ||
        mimeType.EqualsLiteral("application/octet-stream"))
      return nsnull;
  }

  return GetFromType(mimeType);
}

 already_AddRefed<nsMIMEInfoBase>
nsGNOMERegistry::GetFromType(const nsACString& aMIMEType)
{
  nsRefPtr<nsMIMEInfoUnix> mimeInfo = new nsMIMEInfoUnix(aMIMEType);
  NS_ENSURE_TRUE(mimeInfo, nsnull);

  nsCAutoString name;
  nsCAutoString description;

  nsCOMPtr<nsIGIOService> giovfs = do_GetService(NS_GIOSERVICE_CONTRACTID);
  if (giovfs) {
    nsCOMPtr<nsIGIOMimeApp> gioHandlerApp;
    if (NS_FAILED(giovfs->GetAppForMimeType(aMIMEType, getter_AddRefs(gioHandlerApp))) ||
        !gioHandlerApp) {
      return nsnull;
    }
    gioHandlerApp->GetName(name);
    giovfs->GetDescriptionForMimeType(aMIMEType, description);
  } else {
    
    nsCOMPtr<nsIGnomeVFSService> gnomevfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);
    if (!gnomevfs)
      return nsnull;

    nsCOMPtr<nsIGnomeVFSMimeApp> gnomeHandlerApp;
    if (NS_FAILED(gnomevfs->GetAppForMimeType(aMIMEType, getter_AddRefs(gnomeHandlerApp))) ||
        !gnomeHandlerApp) {
      return nsnull;
    }
    gnomeHandlerApp->GetName(name);
    gnomevfs->GetDescriptionForMimeType(aMIMEType, description);
  }

#ifdef MOZ_PLATFORM_MAEMO
  
  
  
  
  
  const char kDefaultTextDomain [] = "maemo-af-desktop";
  nsCAutoString realName (dgettext(kDefaultTextDomain, PromiseFlatCString(name).get()));
  mimeInfo->SetDefaultDescription(NS_ConvertUTF8toUTF16(realName));
#else
  mimeInfo->SetDefaultDescription(NS_ConvertUTF8toUTF16(name));
#endif
  mimeInfo->SetPreferredAction(nsIMIMEInfo::useSystemDefault);
  mimeInfo->SetDescription(NS_ConvertUTF8toUTF16(description));

  nsMIMEInfoBase* retval;
  NS_ADDREF((retval = mimeInfo));
  return retval;
}
