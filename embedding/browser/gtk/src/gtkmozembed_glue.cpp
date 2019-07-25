








































#include "gtkmozembed.h"
#include "gtkmozembed_internal.h"
#include "nsXPCOMGlue.h"

#ifndef XPCOM_GLUE
#error This file only makes sense when XPCOM_GLUE is defined.
#endif

#define GTKMOZEMBED_FUNCTIONS \
  GTKF(gtk_moz_embed_get_type) \
  GTKF(gtk_moz_embed_new) \
  GTKF(gtk_moz_embed_push_startup) \
  GTKF(gtk_moz_embed_pop_startup) \
  GTKF(gtk_moz_embed_set_path) \
  GTKF(gtk_moz_embed_set_comp_path) \
  GTKF(gtk_moz_embed_set_profile_path) \
  GTKF(gtk_moz_embed_load_url) \
  GTKF(gtk_moz_embed_stop_load) \
  GTKF(gtk_moz_embed_can_go_back) \
  GTKF(gtk_moz_embed_can_go_forward) \
  GTKF(gtk_moz_embed_go_back) \
  GTKF(gtk_moz_embed_go_forward) \
  GTKF(gtk_moz_embed_render_data) \
  GTKF(gtk_moz_embed_open_stream) \
  GTKF(gtk_moz_embed_append_data) \
  GTKF(gtk_moz_embed_close_stream) \
  GTKF(gtk_moz_embed_get_link_message) \
  GTKF(gtk_moz_embed_get_js_status) \
  GTKF(gtk_moz_embed_get_title) \
  GTKF(gtk_moz_embed_get_location) \
  GTKF(gtk_moz_embed_reload) \
  GTKF(gtk_moz_embed_set_chrome_mask) \
  GTKF(gtk_moz_embed_get_chrome_mask) \
  GTKF(gtk_moz_embed_single_get_type) \
  GTKF(gtk_moz_embed_single_get)

#define GTKMOZEMBED_FUNCTIONS_INTERNAL \
  GTKF(gtk_moz_embed_get_nsIWebBrowser) \
  GTKF(gtk_moz_embed_get_title_unichar) \
  GTKF(gtk_moz_embed_get_js_status_unichar) \
  GTKF(gtk_moz_embed_get_link_message_unichar) \
  GTKF(gtk_moz_embed_set_directory_service_provider)

#define GTKF(fname) fname##Type fname;

GTKMOZEMBED_FUNCTIONS
GTKMOZEMBED_FUNCTIONS_INTERNAL

#undef GTKF

#define GTKF(fname) { #fname, (NSFuncPtr*) &fname },

static const nsDynamicFunctionLoad GtkSymbols[] = {
GTKMOZEMBED_FUNCTIONS
  { nsnull, nsnull }
};

static const nsDynamicFunctionLoad GtkSymbolsInternal[] = {
GTKMOZEMBED_FUNCTIONS_INTERNAL
  { nsnull, nsnull }
};
#undef GTKF

static nsresult
GTKEmbedGlueStartup()
{
  return XPCOMGlueLoadXULFunctions(GtkSymbols);
}

static nsresult
GTKEmbedGlueStartupInternal()
{
  return XPCOMGlueLoadXULFunctions(GtkSymbolsInternal);
}

