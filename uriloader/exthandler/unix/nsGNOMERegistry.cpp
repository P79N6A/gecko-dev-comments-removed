





































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
  nsCOMPtr<nsIGnomeVFSService> vfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);
  if (!vfs)
    return NS_ERROR_FAILURE;

  return vfs->ShowURI(aURL);
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
  nsCOMPtr<nsIGnomeVFSService> vfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);
  if (!vfs)
    return nsnull;

  
  
  nsCAutoString mimeType;
  if (NS_FAILED(vfs->GetMimeTypeFromExtension(aFileExt, mimeType)) ||
      mimeType.EqualsLiteral("application/octet-stream"))
    return nsnull;

  return GetFromType(mimeType);
}

 already_AddRefed<nsMIMEInfoBase>
nsGNOMERegistry::GetFromType(const nsACString& aMIMEType)
{
  nsCOMPtr<nsIGnomeVFSService> vfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);
  if (!vfs)
    return nsnull;

  nsCOMPtr<nsIGnomeVFSMimeApp> handlerApp;
  if (NS_FAILED(vfs->GetAppForMimeType(aMIMEType, getter_AddRefs(handlerApp))) ||
      !handlerApp)
    return nsnull;

  nsRefPtr<nsMIMEInfoUnix> mimeInfo = new nsMIMEInfoUnix(aMIMEType);
  NS_ENSURE_TRUE(mimeInfo, nsnull);

  nsCAutoString description;
  vfs->GetDescriptionForMimeType(aMIMEType, description);
  mimeInfo->SetDescription(NS_ConvertUTF8toUTF16(description));

  nsCAutoString name;
  handlerApp->GetName(name);
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
