






































#ifndef gtkmozembed_internal_h
#define gtkmozembed_internal_h

#include "nsIWebBrowser.h"
#include "nsXPCOM.h"
#include <stdlib.h>

struct nsModuleComponentInfo;

#ifdef __cplusplus
extern "C" {
#endif

GTKMOZEMBED_API(void,
  gtk_moz_embed_get_nsIWebBrowser, (GtkMozEmbed *embed,
                                    nsIWebBrowser **retval))

GTKMOZEMBED_API(PRUnichar*,
  gtk_moz_embed_get_title_unichar, (GtkMozEmbed *embed))

GTKMOZEMBED_API(PRUnichar*,
  gtk_moz_embed_get_js_status_unichar, (GtkMozEmbed *embed))

GTKMOZEMBED_API(PRUnichar*,
  gtk_moz_embed_get_link_message_unichar, (GtkMozEmbed *embed))

GTKMOZEMBED_API(void,
  gtk_moz_embed_set_directory_service_provider, (nsIDirectoryServiceProvider *appFileLocProvider))

GTKMOZEMBED_API(void,
  gtk_moz_embed_set_app_components, (const nsModuleComponentInfo *aComps,
                                     int aNumComps))

#ifdef __cplusplus
}
#endif 
#endif
