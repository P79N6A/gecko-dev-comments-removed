




































#ifndef nsGdkUtils_h___
#define nsGdkUtils_h___

#include <gtk/gtk.h>


void my_gdk_draw_text(GdkDrawable *drawable,
                      GdkFont     *font,
                      GdkGC       *gc,
                      gint         x,
                      gint         y,
                      const gchar *text,
                      gint         text_length);

#endif 
