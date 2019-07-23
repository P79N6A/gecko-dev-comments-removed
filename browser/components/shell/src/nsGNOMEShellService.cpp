



































#include "nsCOMPtr.h"
#include "nsGNOMEShellService.h"
#include "nsShellService.h"
#include "nsIServiceManager.h"
#include "nsILocalFile.h"
#include "nsIProperties.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIPrefService.h"
#include "prenv.h"
#include "nsStringAPI.h"
#include "nsIGConfService.h"
#include "nsIGIOService.h"
#include "nsIStringBundle.h"
#include "nsIOutputStream.h"
#include "nsIProcess.h"
#include "nsNetUtil.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIImageLoadingContent.h"
#include "imgIRequest.h"
#include "imgIContainer.h"
#include "prprf.h"
#ifdef MOZ_WIDGET_GTK2
#include "nsIImageToPixbuf.h"
#endif

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <limits.h>
#include <stdlib.h>

struct ProtocolAssociation
{
  const char *name;
  PRBool essential;
};

struct MimeTypeAssociation
{
  const char *mimeType;
  const char *extensions;
};

static const ProtocolAssociation appProtocols[] = {
  { "http",   PR_TRUE  },
  { "https",  PR_TRUE  },
  { "ftp",    PR_FALSE },
  { "chrome", PR_FALSE }
};

static const MimeTypeAssociation appTypes[] = {
  { "text/html",             "htm html shtml" },
  { "application/xhtml+xml", "xhtml xht"      }
};

static const char kDocumentIconPath[] = "firefox-document.png";


#define DG_BACKGROUND "/desktop/gnome/background"

static const char kDesktopImageKey[] = DG_BACKGROUND "/picture_filename";
static const char kDesktopOptionsKey[] = DG_BACKGROUND "/picture_options";
static const char kDesktopDrawBGKey[] = DG_BACKGROUND "/draw_background";
static const char kDesktopColorKey[] = DG_BACKGROUND "/primary_color";

nsresult
nsGNOMEShellService::Init()
{
  nsresult rv;

  
  

  nsCOMPtr<nsIGConfService> gconf = do_GetService(NS_GCONFSERVICE_CONTRACTID);
  nsCOMPtr<nsIGIOService> giovfs =
    do_GetService(NS_GIOSERVICE_CONTRACTID);

  if (!gconf)
    return NS_ERROR_NOT_AVAILABLE;

  
  
  mUseLocaleFilenames = PR_GetEnv("G_BROKEN_FILENAMES") != nsnull;

  nsCOMPtr<nsIProperties> dirSvc
    (do_GetService("@mozilla.org/file/directory_service;1"));
  NS_ENSURE_TRUE(dirSvc, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsILocalFile> appPath;
  rv = dirSvc->Get(NS_XPCOM_CURRENT_PROCESS_DIR, NS_GET_IID(nsILocalFile),
                   getter_AddRefs(appPath));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = appPath->AppendNative(NS_LITERAL_CSTRING(MOZ_APP_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  return appPath->GetNativePath(mAppPath);
}

NS_IMPL_ISUPPORTS1(nsGNOMEShellService, nsIShellService)

PRBool
nsGNOMEShellService::KeyMatchesAppName(const char *aKeyValue) const
{

  gchar *commandPath;
  if (mUseLocaleFilenames) {
    gchar *nativePath = g_filename_from_utf8(aKeyValue, -1, NULL, NULL, NULL);
    if (!nativePath) {
      NS_ERROR("Error converting path to filesystem encoding");
      return PR_FALSE;
    }

    commandPath = g_find_program_in_path(nativePath);
    g_free(nativePath);
  } else {
    commandPath = g_find_program_in_path(aKeyValue);
  }

  if (!commandPath)
    return PR_FALSE;

  PRBool matches = mAppPath.Equals(commandPath);
  g_free(commandPath);
  return matches;
}

NS_IMETHODIMP
nsGNOMEShellService::IsDefaultBrowser(PRBool aStartupCheck,
                                      PRBool* aIsDefaultBrowser)
{
  *aIsDefaultBrowser = PR_FALSE;
  if (aStartupCheck)
    mCheckedThisSession = PR_TRUE;

  nsCOMPtr<nsIGConfService> gconf = do_GetService(NS_GCONFSERVICE_CONTRACTID);

  PRBool enabled;
  nsCAutoString handler;

  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(appProtocols); ++i) {
    if (!appProtocols[i].essential)
      continue;

    handler.Truncate();
    gconf->GetAppForProtocol(nsDependentCString(appProtocols[i].name),
                             &enabled, handler);

    
    

    gint argc;
    gchar **argv;

    if (g_shell_parse_argv(handler.get(), &argc, &argv, NULL) && argc > 0) {
      handler.Assign(argv[0]);
      g_strfreev(argv);
    }

    if (!KeyMatchesAppName(handler.get()) || !enabled)
      return NS_OK; 
  }

  *aIsDefaultBrowser = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
nsGNOMEShellService::SetDefaultBrowser(PRBool aClaimAllTypes,
                                       PRBool aForAllUsers)
{
#ifdef DEBUG
  if (aForAllUsers)
    NS_WARNING("Setting the default browser for all users is not yet supported");
#endif

  nsCOMPtr<nsIGConfService> gconf = do_GetService(NS_GCONFSERVICE_CONTRACTID);

  nsCAutoString appKeyValue(mAppPath);
  appKeyValue.Append(" \"%s\"");
  unsigned int i;

  for (i = 0; i < NS_ARRAY_LENGTH(appProtocols); ++i) {
    if (appProtocols[i].essential || aClaimAllTypes) {
      gconf->SetAppForProtocol(nsDependentCString(appProtocols[i].name),
                               appKeyValue);
    }
  }

  
  if (aClaimAllTypes) {
    nsCOMPtr<nsIGIOService> giovfs =
      do_GetService(NS_GIOSERVICE_CONTRACTID);

    nsCOMPtr<nsIStringBundleService> bundleService =
      do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    NS_ENSURE_TRUE(bundleService, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<nsIStringBundle> brandBundle;
    bundleService->CreateBundle(BRAND_PROPERTIES, getter_AddRefs(brandBundle));
    NS_ENSURE_TRUE(brandBundle, NS_ERROR_FAILURE);

    nsString brandShortName, brandFullName;
    brandBundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(),
                                   getter_Copies(brandShortName));
    brandBundle->GetStringFromName(NS_LITERAL_STRING("brandFullName").get(),
                                   getter_Copies(brandFullName));

    
    NS_ConvertUTF16toUTF8 id(brandShortName);
    if (giovfs) {
      nsCOMPtr<nsIGIOMimeApp> appInfo;
      giovfs->CreateAppFromCommand(mAppPath,
                                id,
                                getter_AddRefs(appInfo));

      
      for (i = 0; i < NS_ARRAY_LENGTH(appTypes); ++i) {
        appInfo->SetAsDefaultForMimeType(nsDependentCString(appTypes[i].mimeType));
        appInfo->SetAsDefaultForFileExtensions(nsDependentCString(appTypes[i].extensions));
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGNOMEShellService::GetShouldCheckDefaultBrowser(PRBool* aResult)
{
  
  
  if (mCheckedThisSession) {
    *aResult = PR_FALSE;
    return NS_OK;
  }

  nsCOMPtr<nsIPrefBranch> prefs;
  nsCOMPtr<nsIPrefService> pserve(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (pserve)
    pserve->GetBranch("", getter_AddRefs(prefs));

  if (prefs)
    prefs->GetBoolPref(PREF_CHECKDEFAULTBROWSER, aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsGNOMEShellService::SetShouldCheckDefaultBrowser(PRBool aShouldCheck)
{
  nsCOMPtr<nsIPrefBranch> prefs;
  nsCOMPtr<nsIPrefService> pserve(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (pserve)
    pserve->GetBranch("", getter_AddRefs(prefs));

  if (prefs)
    prefs->SetBoolPref(PREF_CHECKDEFAULTBROWSER, aShouldCheck);

  return NS_OK;
}

static nsresult
WriteImage(const nsCString& aPath, imgIContainer* aImage)
{
#ifndef MOZ_WIDGET_GTK2
  return NS_ERROR_NOT_AVAILABLE;
#else
  nsCOMPtr<nsIImageToPixbuf> imgToPixbuf =
    do_GetService("@mozilla.org/widget/image-to-gdk-pixbuf;1");
  if (!imgToPixbuf)
      return NS_ERROR_NOT_AVAILABLE;

  GdkPixbuf* pixbuf = imgToPixbuf->ConvertImageToPixbuf(aImage);
  if (!pixbuf)
      return NS_ERROR_NOT_AVAILABLE;

  gboolean res = gdk_pixbuf_save(pixbuf, aPath.get(), "png", NULL, NULL);

  g_object_unref(pixbuf);
  return res ? NS_OK : NS_ERROR_FAILURE;
#endif
}
                 
NS_IMETHODIMP
nsGNOMEShellService::SetDesktopBackground(nsIDOMElement* aElement, 
                                          PRInt32 aPosition)
{
  nsresult rv;
  nsCOMPtr<nsIImageLoadingContent> imageContent = do_QueryInterface(aElement, &rv);
  if (!imageContent) return rv;

  
  nsCOMPtr<imgIRequest> request;
  rv = imageContent->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                                getter_AddRefs(request));
  if (!request) return rv;
  nsCOMPtr<imgIContainer> container;
  rv = request->GetImage(getter_AddRefs(container));
  if (!container) return rv;

  
  nsCAutoString filePath(PR_GetEnv("HOME"));

  
  nsString brandName;
  nsCID bundleCID = NS_STRINGBUNDLESERVICE_CID;
  nsCOMPtr<nsIStringBundleService> bundleService(do_GetService(bundleCID));
  if (bundleService) {
    nsCOMPtr<nsIStringBundle> brandBundle;
    rv = bundleService->CreateBundle(BRAND_PROPERTIES,
                                     getter_AddRefs(brandBundle));
    if (NS_SUCCEEDED(rv) && brandBundle) {
      rv = brandBundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(),
                                          getter_Copies(brandName));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  filePath.Append('/');
  filePath.Append(NS_ConvertUTF16toUTF8(brandName));
  filePath.Append("_wallpaper.png");

  
  rv = WriteImage(filePath, container);

  
  nsCOMPtr<nsIGConfService> gconf = do_GetService(NS_GCONFSERVICE_CONTRACTID);

  nsCAutoString options;
  if (aPosition == BACKGROUND_TILE)
    options.Assign("wallpaper");
  else if (aPosition == BACKGROUND_STRETCH)
    options.Assign("stretched");
  else
    options.Assign("centered");

  gconf->SetString(NS_LITERAL_CSTRING(kDesktopOptionsKey), options);

  
  
  
  gconf->SetString(NS_LITERAL_CSTRING(kDesktopImageKey),
                   EmptyCString());

  gconf->SetString(NS_LITERAL_CSTRING(kDesktopImageKey), filePath);
  gconf->SetBool(NS_LITERAL_CSTRING(kDesktopDrawBGKey), PR_TRUE);

  return rv;
}

#define COLOR_16_TO_8_BIT(_c) ((_c) >> 8)
#define COLOR_8_TO_16_BIT(_c) ((_c) << 8 | (_c))

NS_IMETHODIMP
nsGNOMEShellService::GetDesktopBackgroundColor(PRUint32 *aColor)
{
  nsCOMPtr<nsIGConfService> gconf = do_GetService(NS_GCONFSERVICE_CONTRACTID);

  nsCAutoString background;
  gconf->GetString(NS_LITERAL_CSTRING(kDesktopColorKey), background);

  if (background.IsEmpty()) {
    *aColor = 0;
    return NS_OK;
  }

  GdkColor color;
  gboolean success = gdk_color_parse(background.get(), &color);

  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  *aColor = COLOR_16_TO_8_BIT(color.red) << 16 |
            COLOR_16_TO_8_BIT(color.green) << 8 |
            COLOR_16_TO_8_BIT(color.blue);
  return NS_OK;
}

static void
ColorToCString(PRUint32 aColor, nsCString& aResult)
{
  
  char *buf = aResult.BeginWriting(13);
  if (!buf)
    return;

  PRUint16 red = COLOR_8_TO_16_BIT((aColor >> 16) & 0xff);
  PRUint16 green = COLOR_8_TO_16_BIT((aColor >> 8) & 0xff);
  PRUint16 blue = COLOR_8_TO_16_BIT(aColor & 0xff);

  PR_snprintf(buf, 14, "#%04x%04x%04x", red, green, blue);
}

NS_IMETHODIMP
nsGNOMEShellService::SetDesktopBackgroundColor(PRUint32 aColor)
{
  NS_ASSERTION(aColor <= 0xffffff, "aColor has extra bits");
  nsCOMPtr<nsIGConfService> gconf = do_GetService(NS_GCONFSERVICE_CONTRACTID);

  nsCAutoString colorString;
  ColorToCString(aColor, colorString);

  gconf->SetString(NS_LITERAL_CSTRING(kDesktopColorKey), colorString);

  return NS_OK;
}

NS_IMETHODIMP
nsGNOMEShellService::OpenApplication(PRInt32 aApplication)
{
  nsCAutoString scheme;
  if (aApplication == APPLICATION_MAIL)
    scheme.Assign("mailto");
  else if (aApplication == APPLICATION_NEWS)
    scheme.Assign("news");
  else
    return NS_ERROR_NOT_AVAILABLE;

  nsCOMPtr<nsIGConfService> gconf = do_GetService(NS_GCONFSERVICE_CONTRACTID);

  PRBool enabled;
  nsCAutoString appCommand;
  gconf->GetAppForProtocol(scheme, &enabled, appCommand);

  if (!enabled)
    return NS_ERROR_FAILURE;

  
  
  PRBool requiresTerminal;
  gconf->HandlerRequiresTerminal(scheme, &requiresTerminal);
  if (requiresTerminal)
    return NS_ERROR_FAILURE;

  
  int argc;
  char **argv;
  if (!g_shell_parse_argv(appCommand.get(), &argc, &argv, NULL))
    return NS_ERROR_FAILURE;

  char **newArgv = new char*[argc + 1];
  int newArgc = 0;

  
  
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "%s") != 0)
      newArgv[newArgc++] = argv[i];
  }

  newArgv[newArgc] = nsnull;

  gboolean err = g_spawn_async(NULL, newArgv, NULL, G_SPAWN_SEARCH_PATH,
                               NULL, NULL, NULL, NULL);

  g_strfreev(argv);
  delete[] newArgv;

  return err ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsGNOMEShellService::OpenApplicationWithURI(nsILocalFile* aApplication, const nsACString& aURI)
{
  nsresult rv;
  nsCOMPtr<nsIProcess> process = 
    do_CreateInstance("@mozilla.org/process/util;1", &rv);
  if (NS_FAILED(rv))
    return rv;
  
  rv = process->Init(aApplication);
  if (NS_FAILED(rv))
    return rv;

  const nsCString spec(aURI);
  const char* specStr = spec.get();
  return process->Run(PR_FALSE, &specStr, 1);
}

NS_IMETHODIMP
nsGNOMEShellService::GetDefaultFeedReader(nsILocalFile** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
