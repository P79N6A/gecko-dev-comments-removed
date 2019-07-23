




































#ifndef __nsGtkUtils_h
#define __nsGtkUtils_h

#include <gtk/gtk.h>

struct nsGtkUtils
{
  
  
  
#if 0
  static gint gdk_query_pointer(GdkWindow * window,
                                gint *      x_out,
                                gint *      y_out);
#endif

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static void gtk_widget_set_color(GtkWidget *  widget,
                                   GtkRcFlags   flags,
                                   GtkStateType state,
                                   GdkColor *   color);

  





  static GdkModifierType gdk_keyboard_get_modifiers();

  








  static void gdk_window_flash(GdkWindow *     aGdkWindow,
                               unsigned int    aTimes,
                               unsigned long   aInterval,
                               GdkRegion *     aArea);
};

#endif  
