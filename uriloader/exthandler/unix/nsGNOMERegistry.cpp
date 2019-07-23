





































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

#include <glib.h>
#include <glib-object.h>

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

  
  

  nsCAutoString command;
  handlerApp->GetCommand(command);
  gchar *nativeCommand = g_filename_from_utf8(command.get(),
                                              -1, NULL, NULL, NULL);
  if (!nativeCommand) {
    NS_ERROR("Could not convert helper app command to filesystem encoding");
    return nsnull;
  }

  gchar *commandPath = g_find_program_in_path(nativeCommand);

  g_free(nativeCommand);

  if (!commandPath) {
    return nsnull;
  }

  nsCOMPtr<nsILocalFile> appFile;
  NS_NewNativeLocalFile(nsDependentCString(commandPath), PR_TRUE,
                        getter_AddRefs(appFile));
  if (appFile) {
    mimeInfo->SetDefaultApplication(appFile);
    nsCAutoString name;
    handlerApp->GetName(name);
    mimeInfo->SetDefaultDescription(NS_ConvertUTF8toUTF16(name));
    mimeInfo->SetPreferredAction(nsIMIMEInfo::useSystemDefault);
  }

  g_free(commandPath);

  nsMIMEInfoBase* retval;
  NS_ADDREF((retval = mimeInfo));
  return retval;
}
