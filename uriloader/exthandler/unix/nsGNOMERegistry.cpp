




#include "nsGNOMERegistry.h"
#include "nsString.h"
#include "nsIComponentManager.h"
#include "nsIFile.h"
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

 bool
nsGNOMERegistry::HandlerExists(const char *aProtocolScheme)
{
  nsCOMPtr<nsIGIOService> giovfs = do_GetService(NS_GIOSERVICE_CONTRACTID);
  nsCOMPtr<nsIGConfService> gconf = do_GetService(NS_GCONFSERVICE_CONTRACTID);
  if (giovfs) {
    nsCOMPtr<nsIGIOMimeApp> app;
    if (NS_FAILED(giovfs->GetAppForURIScheme(nsDependentCString(aProtocolScheme),
                                             getter_AddRefs(app))))
      return false;
    else
      return true;
  } else if (gconf) {
    bool isEnabled;
    nsAutoCString handler;
    if (NS_FAILED(gconf->GetAppForProtocol(nsDependentCString(aProtocolScheme), &isEnabled, handler)))
      return false;

    return isEnabled;
  }

  return false;
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

  nsAutoCString name;
  if (giovfs) {
    nsCOMPtr<nsIGIOMimeApp> app;
    if (NS_FAILED(giovfs->GetAppForURIScheme(aScheme, getter_AddRefs(app))))
      return;

    app->GetName(name);
  } else {
    bool isEnabled;
    if (NS_FAILED(gconf->GetAppForProtocol(aScheme, &isEnabled, name)))
      return;

    if (!name.IsEmpty()) {
      
      int32_t firstSpace = name.FindChar(' ');
      if (firstSpace != kNotFound) {
        name.Truncate(firstSpace);
        int32_t lastSlash = name.RFindChar('/');
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
  nsAutoCString mimeType;
  nsCOMPtr<nsIGIOService> giovfs = do_GetService(NS_GIOSERVICE_CONTRACTID);

  if (giovfs) {
    
    
    if (NS_FAILED(giovfs->GetMimeTypeFromExtension(aFileExt, mimeType)) ||
        mimeType.EqualsLiteral("application/octet-stream")) {
      return nullptr;
    }
  } else {
    
    nsCOMPtr<nsIGnomeVFSService> gnomevfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);
    if (!gnomevfs)
      return nullptr;

    if (NS_FAILED(gnomevfs->GetMimeTypeFromExtension(aFileExt, mimeType)) ||
        mimeType.EqualsLiteral("application/octet-stream"))
      return nullptr;
  }

  nsRefPtr<nsMIMEInfoBase> mi = GetFromType(mimeType);
  if (mi) {
    mi->AppendExtension(aFileExt);
  }

  return mi.forget();
}

 already_AddRefed<nsMIMEInfoBase>
nsGNOMERegistry::GetFromType(const nsACString& aMIMEType)
{
  nsRefPtr<nsMIMEInfoUnix> mimeInfo = new nsMIMEInfoUnix(aMIMEType);
  NS_ENSURE_TRUE(mimeInfo, nullptr);

  nsAutoCString name;
  nsAutoCString description;

  nsCOMPtr<nsIGIOService> giovfs = do_GetService(NS_GIOSERVICE_CONTRACTID);
  if (giovfs) {
    nsCOMPtr<nsIGIOMimeApp> gioHandlerApp;
    if (NS_FAILED(giovfs->GetAppForMimeType(aMIMEType, getter_AddRefs(gioHandlerApp))) ||
        !gioHandlerApp) {
      return nullptr;
    }
    gioHandlerApp->GetName(name);
    giovfs->GetDescriptionForMimeType(aMIMEType, description);
  } else {
    
    nsCOMPtr<nsIGnomeVFSService> gnomevfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);
    if (!gnomevfs)
      return nullptr;

    nsCOMPtr<nsIGnomeVFSMimeApp> gnomeHandlerApp;
    if (NS_FAILED(gnomevfs->GetAppForMimeType(aMIMEType, getter_AddRefs(gnomeHandlerApp))) ||
        !gnomeHandlerApp) {
      return nullptr;
    }
    gnomeHandlerApp->GetName(name);
    gnomevfs->GetDescriptionForMimeType(aMIMEType, description);
  }

#ifdef MOZ_PLATFORM_MAEMO
  
  
  
  
  
  const char kDefaultTextDomain [] = "maemo-af-desktop";
  nsAutoCString realName (dgettext(kDefaultTextDomain, PromiseFlatCString(name).get()));
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
