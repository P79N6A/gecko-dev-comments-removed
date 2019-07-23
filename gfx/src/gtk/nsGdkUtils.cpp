





































#include "nsGdkUtils.h"
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>

void
my_gdk_draw_text(GdkDrawable *drawable,
                 GdkFont     *font,
                 GdkGC       *gc,
                 gint         x,
                 gint         y,
                 const gchar *text,
                 gint         text_length)
{
#ifdef MOZ_WIDGET_GTK
  GdkWindowPrivate *drawable_private;
  GdkFontPrivate *font_private;
  GdkGCPrivate *gc_private;
#endif 

  g_return_if_fail (drawable != NULL);
  g_return_if_fail (font != NULL);
  g_return_if_fail (gc != NULL);
  g_return_if_fail (text != NULL);

#ifdef MOZ_WIDGET_GTK
  drawable_private = (GdkWindowPrivate*) drawable;
  if (drawable_private->destroyed)
    return;

  gc_private = (GdkGCPrivate*) gc;
  font_private = (GdkFontPrivate*) font;
#endif 
#ifdef MOZ_WIDGET_GTK2
  if (GDK_IS_WINDOW(drawable) && GDK_WINDOW_OBJECT(drawable)->destroyed)
    return;
#endif 

  if (font->type == GDK_FONT_FONT)
  {
#ifdef MOZ_WIDGET_GTK
    XFontStruct *xfont = (XFontStruct *) font_private->xfont;
#endif 
#ifdef MOZ_WIDGET_GTK2
    XFontStruct *xfont = (XFontStruct *) GDK_FONT_XFONT(font);
#endif 

    
    

    
    
    

    if ((xfont->min_byte1 == 0) && (xfont->max_byte1 == 0))
    {
#ifdef MOZ_WIDGET_GTK
      XDrawString (drawable_private->xdisplay, drawable_private->xwindow,
                   gc_private->xgc, x, y, text, MIN(text_length, 32768));
#endif 
#ifdef MOZ_WIDGET_GTK2
      XDrawString (GDK_WINDOW_XDISPLAY(drawable), GDK_DRAWABLE_XID(drawable),
                   GDK_GC_XGC(gc), x, y, text, MIN(text_length, 32768));
#endif 
    }
    else
    {
#ifdef MOZ_WIDGET_GTK
      XDrawString16 (drawable_private->xdisplay, drawable_private->xwindow,
                     gc_private->xgc, x, y, (XChar2b *) text, 
                     MIN((text_length / 2), 32768));
#endif 
#ifdef MOZ_WIDGET_GTK2
      XDrawString16 (GDK_WINDOW_XDISPLAY(drawable), GDK_DRAWABLE_XID(drawable),
                     GDK_GC_XGC(gc), x, y, (XChar2b *) text, 
                     MIN((text_length / 2), 32768));
#endif 
    }
  }
  else if (font->type == GDK_FONT_FONTSET)
  {
#ifdef MOZ_WIDGET_GTK
    XFontSet fontset = (XFontSet) font_private->xfont;
    XmbDrawString (drawable_private->xdisplay, drawable_private->xwindow,
                   fontset, gc_private->xgc, x, y, text, text_length);
#endif 
#ifdef MOZ_WIDGET_GTK2
    XFontSet fontset = (XFontSet) GDK_FONT_XFONT(font);
    XmbDrawString (GDK_WINDOW_XDISPLAY(drawable), GDK_DRAWABLE_XID(drawable),
                   fontset, GDK_GC_XGC(gc), x, y, text, text_length);
#endif 
  }
  else
    g_error("undefined font type\n");
}
