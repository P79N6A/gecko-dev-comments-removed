




































#ifndef gtkmozembedprivate_h
#define gtkmozembedprivate_h

#ifdef __cplusplus
extern "C" {
#endif

#include "gtkmozembed.h"



enum {
  LINK_MESSAGE,
  JS_STATUS,
  LOCATION,
  TITLE,
  PROGRESS,
  PROGRESS_ALL,
  NET_STATE,
  NET_STATE_ALL,
  NET_START,
  NET_STOP,
  NEW_WINDOW,
  VISIBILITY,
  DESTROY_BROWSER,
  OPEN_URI,
  SIZE_TO,
  DOM_KEY_DOWN,
  DOM_KEY_PRESS,
  DOM_KEY_UP,
  DOM_MOUSE_DOWN,
  DOM_MOUSE_UP,
  DOM_MOUSE_CLICK,
  DOM_MOUSE_DBL_CLICK,
  DOM_MOUSE_OVER,
  DOM_MOUSE_OUT,
  SECURITY_CHANGE,
  STATUS_CHANGE,
  DOM_ACTIVATE,
  DOM_FOCUS_IN,
  DOM_FOCUS_OUT,
  EMBED_LAST_SIGNAL
};

extern guint moz_embed_signals[EMBED_LAST_SIGNAL];

extern void gtk_moz_embed_single_create_window(GtkMozEmbed **aNewEmbed,
					       guint aChromeFlags);

#ifdef __cplusplus
}
#endif 

#endif
