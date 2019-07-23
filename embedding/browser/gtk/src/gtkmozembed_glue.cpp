










































#include "gtkmozembed.h"
#include "gtkmozembed_internal.h"
#include "gtkmozembed_common.h"
#include "gtkmozembed_download.h"
#include "nsXPCOMGlue.h"

#ifndef XPCOM_GLUE
#error This file only makes sense when XPCOM_GLUE is defined.
#endif

#define GTKMOZEMBED2_FUNCTIONS \
  GTKF(gtk_moz_embed_download_get_type) \
  GTKF(gtk_moz_embed_download_new) \
  GTKF(gtk_moz_embed_common_get_type) \
  GTKF(gtk_moz_embed_common_new) \
  GTKF(gtk_moz_embed_common_set_pref) \
  GTKF(gtk_moz_embed_common_get_pref) \
  GTKF(gtk_moz_embed_common_save_prefs) \
  GTKF(gtk_moz_embed_common_remove_passwords) \
  GTKF(gtk_moz_embed_common_get_history_list) \
  GTKF(gtk_moz_embed_get_zoom_level) \
  GTKF(gtk_moz_embed_set_zoom_level) \
  GTKF(gtk_moz_embed_find_text) \
  GTKF(gtk_moz_embed_clipboard) \
  GTKF(gtk_moz_embed_notify_plugins) \
  GTKF(gtk_moz_embed_get_context_info) \
  GTKF(gtk_moz_embed_get_selection) \
  GTKF(gtk_moz_embed_get_doc_info) \
  GTKF(gtk_moz_embed_insert_text) \
  GTKF(gtk_moz_embed_common_nsx509_to_raw) \
  GTKF(gtk_moz_embed_common_observe) \
  GTKF(gtk_moz_embed_get_shistory_list) \
  GTKF(gtk_moz_embed_get_shistory_index) \
  GTKF(gtk_moz_embed_shistory_goto_index) \
  GTKF(gtk_moz_embed_get_server_cert) \
  GTKF(gtk_moz_embed_get_nsIWebBrowser)

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
  GTKF(gtk_moz_embed_single_get) \
  GTKMOZEMBED2_FUNCTIONS

#define GTKF(fname) fname##Type fname;

GTKMOZEMBED_FUNCTIONS

#undef GTKF

#define GTKF(fname) { #fname, (NSFuncPtr*) &fname },

static const nsDynamicFunctionLoad GtkSymbols[] = {
GTKMOZEMBED_FUNCTIONS
  { nsnull, nsnull }
};

#undef GTKF

static nsresult
GTKEmbedGlueStartup()
{
  return XPCOMGlueLoadXULFunctions(GtkSymbols);
}
